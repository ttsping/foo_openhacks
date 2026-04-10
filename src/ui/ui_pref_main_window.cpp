#include "pch.h"
#include "ui_pref_main_window.h"
#include "preferences_page_impl.h"
#include "hacks_core.h"
#include "hacks_vars.h"
#include "hacks_guids.h"

DECLARE_PREFERENCES_PAGE("Main window", UIPrefMainWindowDialog, 50.0, OpenHacksGuids::kMainWindowPageGuid, preferences_page::guid_display);

namespace
{
void ReadPseudoCaptionSettingFromControls(HWND dlg, PseudoCaptionParam& param)
{
    const auto ReadCtrlInt = [dlg](int32_t id) { return static_cast<int32_t>(GetDlgItemInt(dlg, id, nullptr, TRUE)); };
    auto& marginStates = param.marginStates;
    param.left = ReadCtrlInt(IDC_EDIT_LEFT);
    param.top = ReadCtrlInt(IDC_EDIT_TOP);
    param.right = ReadCtrlInt(IDC_EDIT_RIGHT);
    param.bottom = ReadCtrlInt(IDC_EDIT_BOTTOM);
    param.width = ReadCtrlInt(IDC_EDIT_WIDTH);
    param.height = ReadCtrlInt(IDC_EDIT_HEIGHT);
    marginStates.left = uButton_GetCheck(dlg, IDC_CHECK_LEFT);
    marginStates.top = uButton_GetCheck(dlg, IDC_CHECK_TOP);
    marginStates.right = uButton_GetCheck(dlg, IDC_CHECK_RIGHT);
    marginStates.bottom = uButton_GetCheck(dlg, IDC_CHECK_BOTTOM);
}
} // namespace

void UIPrefMainWindowDialog::OnInitDialog()
{
    SetHeaderFont(IDC_PREF_HEADER1);

    mComboFrameStyle.Attach(GetDlgItem(IDC_FRAME_STYLE));
    mComboFrameStyle.AddString(TEXT("Default"));
    mComboFrameStyle.AddString(TEXT("No border"));

    LoadUIState();
    UpdateCtrlState();
}

void UIPrefMainWindowDialog::OnApply()
{
    SaveUIState();
    ApplySettings();
}

void UIPrefMainWindowDialog::OnFinalMessage(HWND wnd)
{
    mPseudoCaptionOverlay.CleanUp();
}

void UIPrefMainWindowDialog::OnCommand(UINT code, int id, CWindow ctrl)
{
    switch (code)
    {
    case CBN_SELCHANGE:
        UpdateCtrlState();
        [[fallthrough]];
    case EN_CHANGE:
        NotifyStateChanges(true);
        break;

    default:
        break;
    }

    switch (id)
    {
    case IDC_CHECK_LEFT:
    case IDC_CHECK_TOP:
    case IDC_CHECK_BOTTOM:
    case IDC_CHECK_RIGHT:
        if (code == BN_SETFOCUS || code == BN_KILLFOCUS || code == BN_CLICKED)
            ShowOrHidePseudoCaptionOverlayAutomatically();

        UpdateCtrlState();
        NotifyStateChanges(true);
        break;

    case IDC_EDIT_LEFT:
    case IDC_EDIT_TOP:
    case IDC_EDIT_RIGHT:
    case IDC_EDIT_BOTTOM:
    case IDC_EDIT_WIDTH:
    case IDC_EDIT_HEIGHT:
        if (code == EN_SETFOCUS || code == EN_CHANGE || code == EN_KILLFOCUS)
            ShowOrHidePseudoCaptionOverlayAutomatically();
        break;

    default:
        break;
    }
}

void UIPrefMainWindowDialog::OnSetFocus(CWindow wndOld)
{
    const auto ctrlId = ::GetDlgCtrlID(GetFocus());
    console::formatter() << "focus: " << ctrlId;
}

void UIPrefMainWindowDialog::LoadUIState()
{
    // Migrate old config values: old NoCaption(1) or NoBorder(2) -> new NoBorder(1)
    int32_t styleValue = OpenHacksVars::MainWindowFrameStyle;
    if (styleValue > WindowFrameStyleNoBorder)
        styleValue = WindowFrameStyleNoBorder;
    mComboFrameStyle.SetCurSel(styleValue);

    const auto SetupControl = [dlg = m_hWnd](int32_t checkId, int32_t editId, int32_t spinId, bool state, int32_t value)
    {
        if (checkId != 0)
            uButton_SetCheck(dlg, checkId, state);

        CUpDownCtrl spinCtrl(::GetDlgItem(dlg, spinId));
        spinCtrl.SetBuddy(::GetDlgItem(dlg, editId));
        spinCtrl.SetRange(0, UD_MAXVAL);
        ::SetDlgItemInt(dlg, editId, (UINT)value, TRUE);
    };

    const auto& pseudoCaption = OpenHacksVars::PseudoCaptionSettings.get_value();
    const auto& marginStates = pseudoCaption.marginStates;
    SetupControl(IDC_CHECK_LEFT, IDC_EDIT_LEFT, IDC_SPIN_LEFT, marginStates.left, pseudoCaption.left);
    SetupControl(IDC_CHECK_TOP, IDC_EDIT_TOP, IDC_SPIN_TOP, marginStates.top, pseudoCaption.top);
    SetupControl(IDC_CHECK_RIGHT, IDC_EDIT_RIGHT, IDC_SPIN_RIGHT, marginStates.right, pseudoCaption.right);
    SetupControl(IDC_CHECK_BOTTOM, IDC_EDIT_BOTTOM, IDC_SPIN_BOTTOM, marginStates.bottom, pseudoCaption.bottom);
    SetupControl(0, IDC_EDIT_WIDTH, IDC_SPIN_WIDTH, false, pseudoCaption.width);
    SetupControl(0, IDC_EDIT_HEIGHT, IDC_SPIN_HEIGHT, false, pseudoCaption.height);
}

void UIPrefMainWindowDialog::SaveUIState()
{
    if (const int curSel = mComboFrameStyle.GetCurSel(); curSel != CB_ERR)
    {
        OpenHacksVars::MainWindowFrameStyle = curSel;
    }

    auto& captionSettings = OpenHacksVars::PseudoCaptionSettings.get_value();
    ReadPseudoCaptionSettingFromControls(m_hWnd, captionSettings);
}

void UIPrefMainWindowDialog::UpdateCtrlState()
{
    const bool allowState = mComboFrameStyle.GetCurSel() > 0;
    ::EnableWindow(GetDlgItem(IDC_CHECK_LEFT), allowState);
    ::EnableWindow(GetDlgItem(IDC_CHECK_TOP), allowState);
    ::EnableWindow(GetDlgItem(IDC_CHECK_RIGHT), allowState);
    ::EnableWindow(GetDlgItem(IDC_CHECK_BOTTOM), allowState);
    
    const bool leftState = allowState && uButton_GetCheck(m_hWnd, IDC_CHECK_LEFT);
    ::EnableWindow(GetDlgItem(IDC_EDIT_LEFT), leftState);

    const bool topState = allowState && uButton_GetCheck(m_hWnd, IDC_CHECK_TOP);
    ::EnableWindow(GetDlgItem(IDC_EDIT_TOP), topState);

    const bool rightState = allowState && uButton_GetCheck(m_hWnd, IDC_CHECK_RIGHT);
    ::EnableWindow(GetDlgItem(IDC_EDIT_RIGHT), rightState);

    const bool bottomState = allowState && uButton_GetCheck(m_hWnd, IDC_CHECK_BOTTOM);
    ::EnableWindow(GetDlgItem(IDC_EDIT_BOTTOM), bottomState);

    const bool widthState = allowState && (leftState == false || rightState == false);
    ::EnableWindow(GetDlgItem(IDC_STATIC_WIDTH), widthState);
    ::EnableWindow(GetDlgItem(IDC_EDIT_WIDTH), widthState);

    const bool heightState = allowState && (topState == false || bottomState == false);
    ::EnableWindow(GetDlgItem(IDC_STATIC_HEIGHT), heightState);
    ::EnableWindow(GetDlgItem(IDC_EDIT_HEIGHT), heightState);
}

void UIPrefMainWindowDialog::ApplySettings()
{
    auto& api = OpenHacksCore::Get();
    api.ApplyMainWindowFrameStyle(static_cast<WindowFrameStyle>((int32_t)OpenHacksVars::MainWindowFrameStyle));
}

void UIPrefMainWindowDialog::ShowOrHidePseudoCaptionOverlayAutomatically()
{
    bool targetFocused = false;
    if (auto focusWnd = GetFocus(); focusWnd != nullptr && IsChild(focusWnd))
    {
        switch (::GetDlgCtrlID(focusWnd))
        {
        case IDC_CHECK_LEFT:
        case IDC_CHECK_TOP:
        case IDC_CHECK_BOTTOM:
        case IDC_CHECK_RIGHT:
        case IDC_EDIT_LEFT:
        case IDC_EDIT_TOP:
        case IDC_EDIT_RIGHT:
        case IDC_EDIT_BOTTOM:
        case IDC_EDIT_WIDTH:
        case IDC_EDIT_HEIGHT:
            targetFocused = true;
            break;

        default:
            break;
        }
    }

    if (targetFocused)
    {
        PseudoCaptionParam param = {};
        ReadPseudoCaptionSettingFromControls(m_hWnd, param);
        mPseudoCaptionOverlay.Activate(param.ToRect(core_api::get_main_window()));
    }
    else
    {
        mPseudoCaptionOverlay.Deactivate();
    }
}
