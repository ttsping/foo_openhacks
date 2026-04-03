#include "pch.h"
#include "hacks_core.h"
#include "hacks_vars.h"
#include "win32_utils.h"

namespace
{
FORCEINLINE void UninstallWindowHook(HHOOK& hook)
{
    if (hook != nullptr)
    {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
    }
}

FORCEINLINE INT HitTestToWMSZ(INT hittest)
{
    switch (hittest)
    {
    case HTLEFT:
        return WMSZ_LEFT;
    case HTTOP:
        return WMSZ_TOP;
    case HTRIGHT:
        return WMSZ_RIGHT;
    case HTBOTTOM:
        return WMSZ_BOTTOM;
    case HTTOPLEFT:
        return WMSZ_TOPLEFT;
    case HTTOPRIGHT:
        return WMSZ_TOPRIGHT;
    case HTBOTTOMLEFT:
        return WMSZ_BOTTOMLEFT;
    case HTBOTTOMRIGHT:
        return WMSZ_BOTTOMRIGHT;
    default:
        break;
    }
    return 0;
}

} // namespace

bool OpenHacksCore::InstallWindowHooks()
{
    if (InstallWindowHooksInternal())
        return true;

    mInstallHooksWin32Error = GetLastError();
    mInitErrors |= HooksInstallError;
    UninstallWindowHooks();
    return false;
}

bool OpenHacksCore::InstallWindowHooksInternal()
{
    HINSTANCE hmod = core_api::get_my_instance();
    const DWORD threadId = GetCurrentThreadId();

    mCallWndHook = SetWindowsHookExW(WH_CALLWNDPROC, StaticOpenHacksCallWndProc, hmod, threadId);
    if (mCallWndHook == nullptr)
        return false;

    mGetMsgHook = SetWindowsHookExW(WH_GETMESSAGE, StaticOpenHacksGetMessageProc, hmod, threadId);
    if (mGetMsgHook == nullptr)
        return false;

    return true;
}

void OpenHacksCore::UninstallWindowHooks()
{
    UninstallWindowHook(mGetMsgHook);
    UninstallWindowHook(mCallWndHook);
}

LRESULT OpenHacksCore::OpenHacksCallWndProc(int code, WPARAM wp, LPARAM lp)
{
    if (code >= HC_ACTION)
    {
        const auto pcwps = reinterpret_cast<PCWPSTRUCT>(lp);
        switch (pcwps->message)
        {
        case WM_CREATE:
        {
            if (mMainWindow == nullptr)
            {
                wchar_t className[MAX_PATH] = {};
                GetClassNameW(pcwps->hwnd, className, ARRAYSIZE(className));
                if (className == kDUIMainWindowClassName)
                {
                    mMainWindow = pcwps->hwnd;
                    mMainWindowOriginProc = (WNDPROC)SetWindowLongPtr(pcwps->hwnd, GWLP_WNDPROC, (LONG_PTR)StaticOpenHacksMainWindowProc);
                    OpenHacksVars::DPI = Utility::GetDPI(mMainMenuWindow);
                }
            }

            break;
        }

        default:
            break;
        }
    }

    return CallNextHookEx(mCallWndHook, code, wp, lp);
}

LRESULT OpenHacksCore::OpenHacksGetMessageProc(int code, WPARAM wp, LPARAM lp)
{
    if (code >= HC_ACTION && (UINT)wp == PM_REMOVE)
    {
        auto msg = (LPMSG)(lp);
        if (IsMainOrChildWindow(msg->hwnd))
        {
            switch (msg->message)
            {
            case WM_MOUSEMOVE:
                OnHookMouseMove(msg);
                break;

            case WM_LBUTTONDOWN:
                OnHookLButtonDown(msg);
                break;

            default:
                break;
            }
        }
    }

    return CallNextHookEx(mGetMsgHook, code, wp, lp);
}

void OpenHacksCore::OnHookMouseMove(LPMSG msg)
{
    if (OpenHacksVars::MainWindowFrameStyle != WindowFrameStyleNoBorder)
        return;

    GUITHREADINFO threadInfo = {};
    threadInfo.cbSize = sizeof(threadInfo);
    if (GetGUIThreadInfo(GetCurrentThreadId(), &threadInfo))
    {
        if (threadInfo.flags & (GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE))
            return;

        const DWORD messagePos = GetMessagePos();
        const POINT pt = {GET_X_LPARAM(messagePos), GET_Y_LPARAM(messagePos)};
        const Rect rectForNonSizeing = GetRectForNonSizing();
        if (rectForNonSizeing.IsPointIn(pt))
        {
            if (mRequireRevertCursor)
            {
                mRequireRevertCursor = false;
                SendMessage(mMainWindow, WM_SETCURSOR, (WPARAM)mMainWindow, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
            }

            return;
        }

        const int32_t hittest = (int32_t)SendMessage(mMainWindow, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y));
        if (hittest != HTCLIENT)
        {
            mRequireRevertCursor = true;
            SendMessage(mMainWindow, WM_SETCURSOR, (WPARAM)mMainWindow, MAKELPARAM(hittest, WM_MOUSEMOVE));
            msg->message = WM_NULL;
        }
    }
}

void OpenHacksCore::OnHookLButtonDown(LPMSG msg)
{
    if (OpenHacksVars::MainWindowFrameStyle == WindowFrameStyleDefault)
        return;

    GUITHREADINFO threadInfo = {};
    threadInfo.cbSize = sizeof(threadInfo);
    if (GetGUIThreadInfo(GetCurrentThreadId(), &threadInfo))
    {
        if (threadInfo.flags & (GUI_INMENUMODE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE))
            return;

        const DWORD messagePos = GetMessagePos();
        const POINT pt = {GET_X_LPARAM(messagePos), GET_Y_LPARAM(messagePos)};

        // simulate move
        const auto& pseudoCaption = OpenHacksVars::PseudoCaptionSettings.get_value();
        Rect rectPseudoCaption = pseudoCaption.ToRect(mMainWindow);
        if (rectPseudoCaption.IsPointIn(pt))
        {
            SendMessage(mMainWindow, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
            msg->message = WM_NULL;
            return;
        }

        if (OpenHacksVars::MainWindowFrameStyle == WindowFrameStyleNoBorder)
        {
            // simulate resizing
            const Rect rectForNonSizeing = GetRectForNonSizing();
            if (!rectForNonSizeing.IsPointIn(pt))
            {
                if (threadInfo.flags & (GUI_INMOVESIZE))
                    return;

                const int32_t hittest = (int32_t)SendMessage(mMainWindow, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y));
                if (hittest != HTCLIENT)
                {
                    SendMessage(mMainWindow, WM_SETCURSOR, (WPARAM)mMainWindow, MAKELPARAM(hittest, WM_MOUSEMOVE));
                    SendMessage(mMainWindow, WM_SYSCOMMAND, SC_SIZE | HitTestToWMSZ(hittest), MAKELPARAM(pt.x, pt.y));
                    msg->message = WM_NULL;
                    return;
                }
            }
        }
    }
}