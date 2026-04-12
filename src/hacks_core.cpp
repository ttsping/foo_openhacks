#include "pch.h"
#include "hacks_core.h"
#include "hacks_vars.h"
#include "win32_utils.h"

OpenHacksCore& OpenHacksCore::Get()
{
    static OpenHacksCore core;
    return core;
}

void OpenHacksCore::Initialize()
{
    // check init errors.
    if (mInitErrors != NoError)
    {
        pfc::string8_fast errorMessage;
        if (mInitErrors & IncompatibleComponentInstalled)
        {
            errorMessage << "\nOpenHacks is not compatible with UIHacks.";
        }

        if (mInitErrors & HooksInstallError)
        {
            errorMessage << "\nfailed to install windows hook: " << format_win32_error(mInstallHooksWin32Error) << "(0x"
                         << pfc::format_hex(mInstallHooksWin32Error, 8) << ")";
        }

        popup_message_v2::g_complain(core_api::get_main_window(), "OpenHacks init failed", errorMessage);
        return;
    }

    if (HWND window = core_api::get_main_window())
    {
        // Restore saved window state from persistent storage
        auto& savedWindowData = OpenHacksVars::SavedWindowStateRef();
        if (savedWindowData.wp.rcNormalPosition.right > savedWindowData.wp.rcNormalPosition.left &&
            savedWindowData.wp.rcNormalPosition.bottom > savedWindowData.wp.rcNormalPosition.top)
        {
            mSavedWindowState = savedWindowData.ToWindowState();
        }

        // Migrate old config values: old NoCaption(1) or NoBorder(2) -> new NoBorder(1)
        int32_t styleValue = OpenHacksVars::MainWindowFrameStyle;
        if (styleValue > WindowFrameStyleNoBorder)
        {
            styleValue = WindowFrameStyleNoBorder;
            OpenHacksVars::MainWindowFrameStyle = styleValue;
        }

        auto newStyle = static_cast<WindowFrameStyle>(styleValue);

        if (mSavedWindowState.has_value() && mSavedWindowState->fullscreen)
        {
            // Restore fullscreen state
            WindowState state = {};
            Utility::EnterFullscreen(window, state);
        }
        else if (mSavedWindowState.has_value())
        {
            ApplyMainWindowFrameStyle(newStyle);
            Utility::Maximize(window, mSavedWindowState.value());
        }
        else
        {
            ApplyMainWindowFrameStyle(newStyle);
        }

        if (HWND rebarWindow = FindWindowExW(window, nullptr, kDUIRebarWindowClassName.data(), nullptr))
        {
            mRebarWindow = rebarWindow;
            mReBarOriginProc = (WNDPROC)SetWindowLongPtr(mRebarWindow, GWLP_WNDPROC, (LONG_PTR)StaticOpenHacksReBarProc);
            mMainMenuWindow = FindWindowExW(mRebarWindow, nullptr, kDUIMainMenuBandClassName.data(), nullptr);

            if (OpenHacksVars::ShowMainMenu == false)
            {
                ShowOrHideMenuBar(false);
            }
        }

        if (HWND statusBar = FindWindowExW(window, nullptr, kDUIStatusBarClassName.data(), nullptr))
        {
            mStatusBar = statusBar;
            mStatusBarOriginProc = (WNDPROC)SetWindowLongPtr(statusBar, GWLP_WNDPROC, (LONG_PTR)StaticOpenHacksStatusBarProc);
        }

        // always send WM_SIZE in order to update rectangle stat internal.
        SendMessage(window, WM_SIZE, 0, 0);
    }
}

void OpenHacksCore::Finalize()
{
    UninstallWindowHooks();
}

bool OpenHacksCore::IsMainOrChildWindow(HWND wnd)
{
    return wnd == mMainWindow || IsChild(mMainWindow, wnd);
}

POINT OpenHacksCore::GetBorderMetrics()
{
    const int32_t cx = Utility::GetSystemMetricsForDpi(SM_CXFRAME, OpenHacksVars::DPI) + Utility::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, OpenHacksVars::DPI);
    const int32_t cy = Utility::GetSystemMetricsForDpi(SM_CYFRAME, OpenHacksVars::DPI) + Utility::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, OpenHacksVars::DPI);
    return POINT{cx, cy};
}

Rect OpenHacksCore::GetRectForSizing()
{
    Rect rect;
    GetWindowRect(mMainWindow, &rect);
    const auto border = GetBorderMetrics();
    // Return top border area only (for manual resize handling)
    rect.bottom = rect.top + border.y;
    return rect;
}

void OpenHacksCore::ToggleStatusBar()
{
    if (mStatusBar != nullptr)
    {
        OpenHacksVars::ToggleShowStatusBar();
        ShowOrHideStatusBar(OpenHacksVars::ShowStatusBar);
    }
}

void OpenHacksCore::ToggleMenuBar()
{
    OpenHacksVars::ToggleShowMainMenu();
    if (!ShowOrHideMenuBar(OpenHacksVars::ShowMainMenu))
        OpenHacksVars::ToggleShowMainMenu();
}

void OpenHacksCore::ShowOrHideStatusBar(bool value)
{
    if (mStatusBar == nullptr)
        return;
    SendMessage(core_api::get_main_window(), WM_SIZE, 0, 0);
}

bool OpenHacksCore::ShowOrHideMenuBar(bool value)
{
    if (mRebarWindow == nullptr || mMainMenuWindow == nullptr)
        return false;

    if (IsMenuBarVisible() == value)
        return true;

    const UINT bandCount = (UINT)SendMessage(mRebarWindow, RB_GETBANDCOUNT, 0, 0);
    for (UINT i = 0; i < bandCount; ++i)
    {
        REBARBANDINFO rebarInfo = {};
        rebarInfo.cbSize = sizeof(rebarInfo);
        rebarInfo.fMask = RBBIM_CHILD;
        SendMessage(mRebarWindow, RB_GETBANDINFO, (WPARAM)i, (LPARAM)&rebarInfo);
        if (mMainMenuWindow == rebarInfo.hwndChild)
        {
            SendMessage(mRebarWindow, RB_SHOWBAND, (WPARAM)i, (LPARAM)value);
            return true;
        }
    }

    return false;
}

bool OpenHacksCore::CheckIncompatibleComponents()
{
    static wstring_view_t incompatibleComponents[] = {
        L"foo_ui_hacks.dll",
    };

    for (const auto& fileName : incompatibleComponents)
    {
        if (GetModuleHandleW(fileName.data()))
        {
            mInitErrors |= IncompatibleComponentInstalled;
            break;
        }
    }

    return mInitErrors == NoError;
}

void OpenHacksCore::ApplyMainWindowFrameStyle(WindowFrameStyle newStyle)
{
    HWND mainWindow = core_api::get_main_window();
    Utility::ApplyWindowFrameStyle(mainWindow, newStyle);

    // Calculate mBorderThickness for WM_NCCALCSIZE
    if (newStyle == WindowFrameStyleNoBorder)
    {
        LONG currentStyle = GetWindowLongPtr(mainWindow, GWL_STYLE);
        RECT border = {0};
        AdjustWindowRectEx(&border, currentStyle, FALSE, 0);
        mBorderThickness = border;
        // Force WM_NCCALCSIZE to apply the new border thickness
        SetWindowPos(mainWindow, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    else
    {
        mBorderThickness = {};
    }
}

void OpenHacksCore::ApplyWindowSizeConstraints()
{
    HWND mainWindow = core_api::get_main_window();
    // Force the window to respect new constraints by triggering WM_GETMINMAXINFO
    SetWindowPos(mainWindow, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void OpenHacksCore::Maximize()
{
    HWND mainWindow = core_api::get_main_window();
    // Initialize saved state if not already
    if (!mSavedWindowState.has_value())
        mSavedWindowState.emplace();
    Utility::Maximize(mainWindow, mSavedWindowState.value());
    // Save to persistent storage
    OpenHacksVars::SavedWindowStateRef().FromWindowState(mSavedWindowState.value());
}

void OpenHacksCore::Restore()
{
    HWND mainWindow = core_api::get_main_window();
    // Check if minimized - use standard restore
    if (Utility::IsMinimized(mainWindow))
    {
        ShowWindow(mainWindow, SW_RESTORE);
    }
    else if (mSavedWindowState.has_value())
    {
        Utility::Restore(mainWindow, mSavedWindowState.value());
        mSavedWindowState.reset();
        // Clear persistent storage
        OpenHacksVars::SavedWindowStateRef() = WindowStateData();
    }
    else
    {
        // No saved state - use standard restore
        ShowWindow(mainWindow, SW_RESTORE);
    }
}

bool OpenHacksCore::IsMaximized()
{
    return mSavedWindowState.has_value() || Utility::IsMaximized(core_api::get_main_window());
}

bool OpenHacksCore::IsMinimized()
{
    return Utility::IsMinimized(core_api::get_main_window());
}

void OpenHacksCore::EnterFullscreen()
{
    HWND mainWindow = core_api::get_main_window();
    // Save current window state
    auto& state = mSavedWindowState.emplace();
    state.fullscreen = true;
    state.style = static_cast<DWORD>(GetWindowLongPtr(mainWindow, GWL_STYLE));
    GetWindowPlacement(mainWindow, &state.wp);

    Utility::EnterFullscreen(mainWindow, mSavedWindowState.value());

    // Mark fullscreen state in persistent storage
    OpenHacksVars::SavedWindowStateRef().FromWindowState(state);
}

void OpenHacksCore::ExitFullscreen()
{
    HWND mainWindow = core_api::get_main_window();
    // Exit fullscreen
    if (mSavedWindowState.has_value())
    {
        Utility::ExitFullscreen(mainWindow, mSavedWindowState.value());
        mSavedWindowState.reset();

        // Clear fullscreen state in persistent storage
        OpenHacksVars::SavedWindowStateRef() = WindowStateData();
    }
    else
    {
        const auto newStyle = static_cast<WindowFrameStyle>((int32_t)OpenHacksVars::MainWindowFrameStyle);
        ApplyMainWindowFrameStyle(newStyle);

        RECT rect = {};
        GetWindowRect(mainWindow, &rect);
        OffsetRect(&rect, 10, 10);
        SetWindowPos(mainWindow, nullptr, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    }
}

void OpenHacksCore::ToggleFullscreen()
{
    HWND mainWindow = core_api::get_main_window();
    if (!Utility::IsFullscreen(mainWindow))
        EnterFullscreen();
    else
        ExitFullscreen();
}
