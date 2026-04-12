#include "pch.h"
#include "hacks_com_impl.h"

#include "hacks_core.h"
#include "hacks_vars.h"
#include "win32_utils.h"

STDMETHODIMP OpenHacksCOM::get_DPI(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = static_cast<LONG>(Utility::GetDPI(core_api::get_main_window()));
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_MenuBarVisible(VARIANT_BOOL* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = TO_VARIANT_BOOL(OpenHacksVars::ShowMainMenu);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_MenuBarVisible(VARIANT_BOOL value)
{
    const bool visible = TO_BOOLEAN(value);
    if (OpenHacksCore::Get().ShowOrHideMenuBar(visible))
        OpenHacksVars::ShowMainMenu = (visible);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_StatusBarVisible(VARIANT_BOOL* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = TO_VARIANT_BOOL(OpenHacksVars::ShowStatusBar);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_StatusBarVisible(VARIANT_BOOL value)
{
    OpenHacksVars::ShowStatusBar = TO_BOOLEAN(value);
    OpenHacksCore::Get().ShowOrHideStatusBar(OpenHacksVars::ShowStatusBar);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_Fullscreen(VARIANT_BOOL* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = TO_VARIANT_BOOL(Utility::IsFullscreen(core_api::get_main_window()));
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_Fullscreen(VARIANT_BOOL value)
{
    bool setFullscreen = TO_BOOLEAN(value);
    bool isFullscreen = Utility::IsFullscreen(core_api::get_main_window());
    if (setFullscreen && !isFullscreen)
        OpenHacksCore::Get().EnterFullscreen();
    else if (!setFullscreen && isFullscreen)
        OpenHacksCore::Get().ExitFullscreen();
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_WindowState(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    HWND mainWindow = core_api::get_main_window();
    if (Utility::IsFullscreen(mainWindow))
        *pValue = WindowStateFullscreen;
    else if (Utility::IsMaximized(mainWindow))
        *pValue = WindowStateMaximized;
    else if (Utility::IsMinimized(mainWindow))
        *pValue = WindowStateMinimized;
    else
        *pValue = WindowStateNormal;
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_WindowState(LONG value)
{
    HWND mainWindow = core_api::get_main_window();
    auto& core = OpenHacksCore::Get();
    bool isFullscreen = Utility::IsFullscreen(mainWindow);
    bool isMaximized = Utility::IsMaximized(mainWindow);
    bool isMinimized = Utility::IsMinimized(mainWindow);

    switch (value)
    {
    case WindowStateNormal:
        if (isFullscreen)
            core.ExitFullscreen();
        else if (isMaximized || isMinimized)
            core.Restore();
        break;

    case WindowStateMinimized:
        if (isFullscreen)
            core.ExitFullscreen();
        ShowWindow(mainWindow, SW_MINIMIZE);
        break;

    case WindowStateMaximized:
        if (isFullscreen)
            core.ExitFullscreen();
        core.Maximize();
        break;

    case WindowStateFullscreen:
        if (!isFullscreen)
            core.EnterFullscreen();
        break;

    default:
        return E_INVALIDARG;
    }

    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_WindowFrameStyle(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = static_cast<LONG>(OpenHacksVars::MainWindowFrameStyle.get_value());
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_WindowFrameStyle(LONG value)
{
    if (value < WindowFrameStyleDefault || value > WindowFrameStyleNoBorder)
        return E_INVALIDARG;

    OpenHacksVars::MainWindowFrameStyle = value;
    OpenHacksCore::Get().ApplyMainWindowFrameStyle(static_cast<WindowFrameStyle>(value));
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::ToggleMenuBar()
{
    OpenHacksCore::Get().ToggleMenuBar();
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::ToggleStatusBar()
{
    OpenHacksCore::Get().ToggleStatusBar();
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::ToggleFullscreen()
{
    OpenHacksCore::Get().ToggleFullscreen();
    return S_OK;
}

// PseudoCaptionSettings Properties
STDMETHODIMP OpenHacksCOM::get_PseudoCaptionLeft(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = static_cast<LONG>(OpenHacksVars::PseudoCaption().left);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionLeft(LONG value)
{
    OpenHacksVars::PseudoCaption().left = value;
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionTop(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = static_cast<LONG>(OpenHacksVars::PseudoCaption().top);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionTop(LONG value)
{
    OpenHacksVars::PseudoCaption().top = value;
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionRight(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = static_cast<LONG>(OpenHacksVars::PseudoCaption().right);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionRight(LONG value)
{
    OpenHacksVars::PseudoCaption().right = value;
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionBottom(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = static_cast<LONG>(OpenHacksVars::PseudoCaption().bottom);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionBottom(LONG value)
{
    OpenHacksVars::PseudoCaption().bottom = value;
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionWidth(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = static_cast<LONG>(OpenHacksVars::PseudoCaption().width);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionWidth(LONG value)
{
    OpenHacksVars::PseudoCaption().width = value;
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionHeight(LONG* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = static_cast<LONG>(OpenHacksVars::PseudoCaption().height);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionHeight(LONG value)
{
    OpenHacksVars::PseudoCaption().height = value;
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionLeftEnabled(VARIANT_BOOL* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = TO_VARIANT_BOOL(OpenHacksVars::PseudoCaption().marginStates.left);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionLeftEnabled(VARIANT_BOOL value)
{
    OpenHacksVars::PseudoCaption().marginStates.left = TO_BOOLEAN(value);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionTopEnabled(VARIANT_BOOL* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = TO_VARIANT_BOOL(OpenHacksVars::PseudoCaption().marginStates.top);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionTopEnabled(VARIANT_BOOL value)
{
    OpenHacksVars::PseudoCaption().marginStates.top = TO_BOOLEAN(value);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionRightEnabled(VARIANT_BOOL* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = TO_VARIANT_BOOL(OpenHacksVars::PseudoCaption().marginStates.right);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionRightEnabled(VARIANT_BOOL value)
{
    OpenHacksVars::PseudoCaption().marginStates.right = TO_BOOLEAN(value);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::get_PseudoCaptionBottomEnabled(VARIANT_BOOL* pValue)
{
    RETURN_HR_IF(pValue == nullptr, E_POINTER);
    *pValue = TO_VARIANT_BOOL(OpenHacksVars::PseudoCaption().marginStates.bottom);
    return S_OK;
}

STDMETHODIMP OpenHacksCOM::put_PseudoCaptionBottomEnabled(VARIANT_BOOL value)
{
    OpenHacksVars::PseudoCaption().marginStates.bottom = TO_BOOLEAN(value);
    return S_OK;
}
