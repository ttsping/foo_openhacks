#include "pch.h"
#include "win32_utils.h"
#include "hacks_vars.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace Utility
{
int(WINAPI* pfnGetSystemMetricsForDpi)(int, UINT) = nullptr;
UINT(WINAPI* pfnGetDpiForSystem)() = nullptr;
UINT(WINAPI* pfnGetDpiForWindow)(HWND) = nullptr;
DPI_AWARENESS_CONTEXT(WINAPI* pfnGetThreadDpiAwarenessContext)() = nullptr;
DPI_AWARENESS(WINAPI* pfnGetAwarenessFromDpiAwarenessContext)(DPI_AWARENESS_CONTEXT value) = nullptr;

static std::once_flag staticLoadFlag;

static void LoadUtilityProc()
{
    // clang-format off
    std::call_once(staticLoadFlag, [&]()
    {
        if (HMODULE user32 = GetModuleHandleW(L"user32.dll"))
        {
            std::ignore = GetProcAddress(user32, "GetSystemMetricsForDpi", pfnGetSystemMetricsForDpi);
            std::ignore = GetProcAddress(user32, "GetDpiForSystem", pfnGetDpiForSystem);
            std::ignore = GetProcAddress(user32, "GetDpiForWindow", pfnGetDpiForWindow);
            std::ignore = GetProcAddress(user32, "GetThreadDpiAwarenessContext", pfnGetThreadDpiAwarenessContext);
            std::ignore = GetProcAddress(user32, "GetAwarenessFromDpiAwarenessContext", pfnGetAwarenessFromDpiAwarenessContext);
        }
    });
    // clang-format on
}

RECT& ClientToScreen(HWND wnd, RECT& rc)
{
    LPPOINT pt = (LPPOINT)&rc;
    ::ClientToScreen(wnd, pt);
    ::ClientToScreen(wnd, pt + 1);
    return rc;
}

RECT& ScreenToClient(HWND wnd, RECT& rc)
{
    LPPOINT pt = (LPPOINT)&rc;
    ::ScreenToClient(wnd, pt);
    ::ScreenToClient(wnd, pt + 1);
    return rc;
}

bool IsCompositionEnabled()
{
    BOOL composition_enabled = FALSE;
    return SUCCEEDED(DwmIsCompositionEnabled(&composition_enabled)) && composition_enabled;
}

bool IsWindows11OrGreater()
{
    // Use RtlGetVersion to get the real Windows version
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtDll)
        return false;

    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    auto pRtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hNtDll, "RtlGetVersion"));
    if (!pRtlGetVersion)
        return false;

    RTL_OSVERSIONINFOW osvi = { sizeof(osvi) };
    if (pRtlGetVersion(&osvi) != 0)
        return false;

    // Windows 11 is version 10.0.22000 or higher
    return osvi.dwMajorVersion > 10 ||
           (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion > 0) ||
           (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber >= 22000);
}

bool EnableWindowShadow(HWND window, bool enable)
{
    // Only apply shadow on Windows 11 or later
    if (!IsWindows11OrGreater())
        return false;

    if (IsCompositionEnabled())
    {
        static const MARGINS shadow_state[2]{{0, 0, 0, 0}, {1, 1, 1, 1}};
        const bool result = SUCCEEDED(DwmExtendFrameIntoClientArea(window, &shadow_state[enable ? 1 : 0]));
        const DWORD policy = DWMNCRP_ENABLED;
        std::ignore = DwmSetWindowAttribute(window, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        return result;
    }

    return false;
}

uint32_t GetDPI(HWND window)
{
    LoadUtilityProc();

    //if (window != nullptr && pfnGetDpiForWindow != nullptr)
    //    return pfnGetDpiForWindow(window);

    //if (pfnGetDpiForSystem != nullptr)
    //    return pfnGetDpiForSystem();

    if (HDC dc = GetDC(HWND_DESKTOP))
    {
        auto dpi = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(window, dc);
        return static_cast<uint32_t>(dpi);
    }

    return USER_DEFAULT_SCREEN_DPI;
}

int32_t GetSystemMetricsForDpi(int32_t index, uint32_t dpi)
{
    LoadUtilityProc();

    if (pfnGetSystemMetricsForDpi != nullptr)
        return pfnGetSystemMetricsForDpi(index, dpi);

    const int32_t rc = GetSystemMetrics(index);
    if (dpi == USER_DEFAULT_SCREEN_DPI)
        return rc;

    if (pfnGetThreadDpiAwarenessContext != nullptr && pfnGetAwarenessFromDpiAwarenessContext != nullptr)
    {
        const auto awareness = pfnGetAwarenessFromDpiAwarenessContext(pfnGetThreadDpiAwarenessContext());
        if (awareness == DPI_AWARENESS_UNAWARE || awareness == DPI_AWARENESS_INVALID)
        {
            const double scaleFactor = static_cast<double>(dpi) / 96.0;
            return static_cast<int>(std::round(rc * scaleFactor));
        }
    }

    return rc;
}

void Maximize(HWND wnd, WindowState& state)
{
    // Save current window state
    state.style = static_cast<DWORD>(GetWindowLongPtr(wnd, GWL_STYLE));
    GetWindowPlacement(wnd, &state.wp);

    // Check if window has caption (default style)
    bool hasCaption = (state.style & WS_CAPTION) != 0;

    if (hasCaption)
    {
        // Default style: use Windows standard maximize
        ShowWindow(wnd, SW_MAXIMIZE);
    }
    else
    {
        // NoCaption/NoBorder: switch to NoBorder style and disable shadow
        // This simplifies maximize - no need to calculate border
        LONG newStyle = state.style & ~(WS_CAPTION | WS_THICKFRAME);
        SetWindowLongPtr(wnd, GWL_STYLE, newStyle);

        // Get work area from the monitor where window is located
        RECT workArea;
        if (HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST))
        {
            MONITORINFO mi = { sizeof(MONITORINFO) };
            if (GetMonitorInfo(monitor, &mi))
                workArea = mi.rcWork;
            else
                SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); // Fallback
        }
        else
        {
            SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); // Fallback
        }

        // Position window to fill work area (no border calculation needed)
        SetWindowPos(wnd, HWND_TOP,
            workArea.left, workArea.top,
            workArea.right - workArea.left,
            workArea.bottom - workArea.top,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

void Restore(HWND wnd, WindowState& state)
{
    // Check if state is initialized
    if (state.wp.length == 0)
        return;

    // Check if window had caption (default style)
    bool hadCaption = (state.style & WS_CAPTION) != 0;

    if (hadCaption)
    {
        // Default style: use Windows standard restore
        ShowWindow(wnd, SW_RESTORE);
    }
    else
    {
        // NoCaption/NoBorder: restore original style first
        SetWindowLongPtr(wnd, GWL_STYLE, state.style);
        
        // Restore from saved WINDOWPLACEMENT
        state.wp.showCmd = SW_SHOWNORMAL;
        SetWindowPlacement(wnd, &state.wp);
        
        // Shadow is rendered automatically by THICKFRAME
        
        // Notify frame changes
        SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

bool IsMaximized(HWND wnd)
{
    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(wnd, &wp);

    // Windows standard maximized state
    if (wp.showCmd == SW_SHOWMAXIMIZED)
        return true;

    // Check if window has caption (default style)
    DWORD style = static_cast<DWORD>(GetWindowLongPtr(wnd, GWL_STYLE));
    bool hasCaption = (style & WS_CAPTION) != 0;

    if (!hasCaption)
    {
        // For custom maximized (NoCaption/NoBorder), check if window matches work area
        RECT workArea;
        if (HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST))
        {
            MONITORINFO mi = { sizeof(MONITORINFO) };
            if (GetMonitorInfo(monitor, &mi))
                workArea = mi.rcWork;
            else
                SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); // Fallback
        }
        else
        {
            SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); // Fallback
        }

        RECT windowRect;
        GetWindowRect(wnd, &windowRect);

        // Check if window rect matches work area
        return windowRect.left == workArea.left &&
               windowRect.top == workArea.top &&
               windowRect.right == workArea.right &&
               windowRect.bottom == workArea.bottom;
    }

    return false;
}

bool IsMinimized(HWND wnd)
{
    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(wnd, &wp);
    return wp.showCmd == SW_SHOWMINIMIZED || wp.showCmd == SW_MINIMIZE;
}

bool IsFullscreenOrMaximized(HWND wnd)
{
    return IsFullscreen(wnd) || IsMaximized(wnd);
}

void ApplyWindowFrameStyle(HWND wnd, WindowFrameStyle style)
{
    const LONG currentStyle = static_cast<LONG>(GetWindowLongPtr(wnd, GWL_STYLE));
    LONG newStyle = currentStyle;

    switch (style)
    {
    case WindowFrameStyleDefault:
        newStyle |= (WS_CAPTION | WS_THICKFRAME);
        break;

    case WindowFrameStyleNoBorder:
        newStyle |= WS_THICKFRAME;
        newStyle &= ~(WS_CAPTION);
        break;

    default:
        break;
    }

    if (currentStyle == newStyle)
        return;

    SetWindowLongPtr(wnd, GWL_STYLE, newStyle);

    // Notify frame changes
    SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void EnterFullscreen(HWND wnd, WindowState& state)
{
    // Save current window state
    state.style = static_cast<DWORD>(GetWindowLongPtr(wnd, GWL_STYLE));
    GetWindowPlacement(wnd, &state.wp);

    // Switch to NoBorder style (remove caption and thick frame)
    LONG newStyle = state.style & ~(WS_CAPTION | WS_THICKFRAME);
    SetWindowLongPtr(wnd, GWL_STYLE, newStyle);

    // Get full screen area from the monitor where window is located
    RECT screenArea;
    if (HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST))
    {
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(monitor, &mi))
            screenArea = mi.rcMonitor; // Use full monitor area (including taskbar)
        else
            SystemParametersInfo(SPI_GETWORKAREA, 0, &screenArea, 0); // Fallback
    }
    else
    {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &screenArea, 0); // Fallback
    }

    // Position window to fill entire screen
    SetWindowPos(wnd, HWND_TOP,
        screenArea.left, screenArea.top,
        screenArea.right - screenArea.left,
        screenArea.bottom - screenArea.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void ExitFullscreen(HWND wnd, WindowState& state)
{
    // Check if state is initialized
    if (state.wp.length == 0 || state.fullscreen == false)
        return;

    // Restore original style
    SetWindowLongPtr(wnd, GWL_STYLE, state.style);

    // Restore from saved WINDOWPLACEMENT
    SetWindowPlacement(wnd, &state.wp);

    // Shadow is rendered automatically by THICKFRAME

    // Notify frame changes
    SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool IsFullscreen(HWND wnd)
{
    // Get current window rect
    RECT windowRect;
    GetWindowRect(wnd, &windowRect);

    // Get full monitor area
    RECT monitorRect;
    if (HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST))
    {
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(monitor, &mi))
            monitorRect = mi.rcMonitor;
        else
            return false;
    }
    else
    {
        return false;
    }

    // Compare window rect with monitor rect
    return windowRect.left == monitorRect.left &&
           windowRect.top == monitorRect.top &&
           windowRect.right == monitorRect.right &&
           windowRect.bottom == monitorRect.bottom;
}

} // namespace Utility
