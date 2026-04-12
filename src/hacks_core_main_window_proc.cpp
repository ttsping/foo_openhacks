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
    // Disable sizing border hit tests if configured
    if (OpenHacksVars::WindowSizeConstraintsSettings.get_value().disableSizing)
    {
        LRESULT result = CallWindowProc(mMainWindowOriginProc, wnd, WM_NCHITTEST, wp, lp);
        // Convert sizing hit tests to border/client
        switch (result)
        {
        case HTLEFT:
        case HTRIGHT:
        case HTTOP:
        case HTBOTTOM:
        case HTTOPLEFT:
        case HTTOPRIGHT:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
            return HTBORDER;
        default:
            return result;
        }
    }

    // Only handle top border, other edges are handled by system THICKFRAME
    const POINT cursor = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    const POINT border = GetBorderMetrics();
    RECT rect = {};
    GetWindowRect(mMainWindow, &rect);

    // Check if cursor is in top border area
    if (cursor.y < (rect.top + border.y))
    {
        // Distinguish top-left, top, and top-right based on X coordinate
        if (cursor.x < (rect.left + border.x))
            return HTTOPLEFT;
        else if (cursor.x >= (rect.right - border.x))
            return HTTOPRIGHT;
        else
            return HTTOP;
    }

    // Let original window proc handle other areas (THICKFRAME will handle left/right/bottom)
    return CallWindowProc(mMainWindowOriginProc, wnd, WM_NCHITTEST, wp, lp);
}

bool OpenHacksCore::OnSetCursor(HWND wnd, WPARAM wp, LPARAM lp)
{
    if (OpenHacksVars::MainWindowFrameStyle != WindowFrameStyleNoBorder)
        return false;

    const int32_t hittest = (int32_t)LOWORD(lp);
    if (hittest == HTCLIENT)
        return false;

    // Handle top border hit tests (other edges handled by system THICKFRAME)
    if (hittest == HTTOP)
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
    else if (hittest == HTTOPLEFT)
        SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
    else if (hittest == HTTOPRIGHT)
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
        // Disable sizing if configured
        if (OpenHacksVars::WindowSizeConstraintsSettings.get_value().disableSizing)
        {
            const auto cmd = static_cast<UINT>(wp & 0xFFF0);
            if (cmd == SC_SIZE || cmd == SC_MAXIMIZE)
                return 0;
        }
        break;

    case WM_NCHITTEST:
        if (OpenHacksVars::WindowSizeConstraintsSettings.get_value().disableSizing ||
            OpenHacksVars::MainWindowFrameStyle == WindowFrameStyleNoBorder)
            return OnNCHitTest(wnd, wp, lp);
        break;

    case WM_SETCURSOR:
        if (OnSetCursor(wnd, wp, lp))
            return 1;
        break;

    case WM_NCACTIVATE:
        if (OpenHacksVars::MainWindowFrameStyle == WindowFrameStyleNoBorder)
            return CallWindowProc(mMainWindowOriginProc, wnd, msg, wp, -1);
        break;

    case WM_NCCALCSIZE:
        if (wp && (OpenHacksVars::MainWindowFrameStyle == WindowFrameStyleNoBorder))
        {
            DWORD style = GetWindowLongPtr(wnd, GWL_STYLE);
            if (style & WS_THICKFRAME)
            {
                auto res = CallWindowProc(mMainWindowOriginProc, wnd, msg, wp, lp);
                auto sz = (NCCALCSIZE_PARAMS*)(lp);
                sz->rgrc[0].top += mBorderThickness.top;
                return res;
            }
        }
        break;

    case WM_SIZE:
        if (OnSize(wnd, wp, lp))
            return 0;
        break;

    case WM_GETMINMAXINFO:
        {
            const auto& constraints = OpenHacksVars::WindowSizeConstraintsSettings.get_value();
            auto* minmaxInfo = (MINMAXINFO*)lp;
            // Apply minimum size constraints
            if (constraints.enableMinSize)
            {
                if (constraints.minWidth > 0)
                    minmaxInfo->ptMinTrackSize.x = constraints.minWidth;
                if (constraints.minHeight > 0)
                    minmaxInfo->ptMinTrackSize.y = constraints.minHeight;
            }
            // Apply maximum size constraints
            if (constraints.enableMaxSize)
            {
                if (constraints.maxWidth > 0)
                    minmaxInfo->ptMaxTrackSize.x = constraints.maxWidth;
                if (constraints.maxHeight > 0)
                    minmaxInfo->ptMaxTrackSize.y = constraints.maxHeight;
            }
        }
        break;

    case WM_DPICHANGED: // fixme: won't receive currently(DPI System aware).
        OpenHacksVars::DPI = static_cast<uint32_t>(LOWORD(wp));
        break;

    default:
        break;
    }

    return CallWindowProc(mMainWindowOriginProc, wnd, msg, wp, lp);
}
