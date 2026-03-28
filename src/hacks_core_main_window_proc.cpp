#include "pch.h"
#include <sstream>
#include "hacks_menu.h"
#include "hacks_vars.h"
#include "hacks_core.h"
#include "win32_utils.h"

namespace
{
static bool PopupMainMenu(HWND wnd)
{
    if (HMENU menu = OpenHacksMenu::Get().GenerateMenu())
    {
        POINT point = {};
        ClientToScreen(wnd, &point);
        const int32_t cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, point.x, point.y, 0, wnd, nullptr);
        OpenHacksMenu::Get().ExecuteMenuCommand(cmd);
        DestroyMenu(menu);
        return true;
    }

    return false;
}
} // namespace

bool OpenHacksCore::OnSysCommand(HWND wnd, WPARAM wp, LPARAM lp)
{
    UNREFERENCED_PARAMETER(lp);
    const auto cmd = static_cast<UINT>(wp & 0xFFF0);
    switch (cmd)
    {
    case SC_MOUSEMENU:
        return PopupMainMenu(wnd);

    default:
        break;
    }

    return false;
}

LRESULT OpenHacksCore::OnNCHitTest(HWND wnd, WPARAM wp, LPARAM lp)
{
    const POINT cursor = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    const POINT border = GetBorderMetrics();
    RECT rect = {};
    GetWindowRect(mMainWindow, &rect);
    enum EdgeMask
    {
        Left = 0b0001,
        Right = 0b0010,
        Top = 0b0100,
        Bottom = 0b1000,
    };

    const auto result = Left * (cursor.x < (rect.left + border.x)) | Right * (cursor.x >= (rect.right - border.x)) | Top * (cursor.y < (rect.top + border.y)) |
                        Bottom * (cursor.y >= (rect.bottom - border.y));
    switch (result)
    {
    case Left:
        return HTLEFT;
    case Right:
        return HTRIGHT;
    case Top:
        return HTTOP;
    case Bottom:
        return HTBOTTOM;
    case Top | Left:
        return HTTOPLEFT;
    case Top | Right:
        return HTTOPRIGHT;
    case Bottom | Left:
        return HTBOTTOMLEFT;
    case Bottom | Right:
        return HTBOTTOMRIGHT;
    default:
        return HTNOWHERE;
    }
}

bool OpenHacksCore::OnSetCursor(HWND wnd, WPARAM wp, LPARAM lp)
{
    if (OpenHacksVars::MainWindowFrameStyle != WindowFrameStyleNoBorder)
        return false;

    const int32_t hittest = (int32_t)LOWORD(lp);
    if (hittest == HTCLIENT)
        return false;

    if (hittest == HTTOP || hittest == HTBOTTOM)
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
    else if (hittest == HTLEFT || hittest == HTRIGHT)
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
    else if (hittest == HTTOPLEFT || hittest == HTBOTTOMRIGHT)
        SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
    else if (hittest == HTTOPRIGHT || hittest == HTBOTTOMLEFT)
        SetCursor(LoadCursor(nullptr, IDC_SIZENESW));
    else
        return false;

    return true;
}

bool OpenHacksCore::OnSize(HWND wnd, WPARAM wp, LPARAM lp)
{
    return false;
}

LRESULT OpenHacksCore::OpenHacksMainWindowProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_SYSCOMMAND:
        if (OnSysCommand(wnd, wp, lp))
            return 0;
        break;

    case WM_NCHITTEST:
        return OnNCHitTest(wnd, wp, lp);

    case WM_SETCURSOR:
        if (OnSetCursor(wnd, wp, lp))
            return 1;
        break;

    case WM_NCACTIVATE:
        if (OpenHacksVars::MainWindowFrameStyle == WindowFrameStyleNoBorder)
            return CallWindowProc(mMainWindowOriginProc, wnd, msg, wp, -1);
        break;

    case WM_SIZE:
        if (OnSize(wnd, wp, lp))
            return 0;
        break;

    case WM_DPICHANGED: // fixme: won't receive currently(DPI System aware).
        OpenHacksVars::DPI = static_cast<uint32_t>(LOWORD(wp));
        break;

    default:
        break;
    }

    return CallWindowProc(mMainWindowOriginProc, wnd, msg, wp, lp);
}
