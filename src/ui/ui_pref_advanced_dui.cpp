#include "pch.h"
#include "ui_pref_advanced_dui.h"
#include "preferences_page_impl.h"
#include "hacks_core.h"
#include "hacks_vars.h"
#include "hacks_guids.h"

DECLARE_PREFERENCES_PAGE("Advanced", UIPrefAdvancedDUIDialog, 100.0, OpenHacksGuids::kAdvancedDUIPageGuid, OpenHacksGuids::kDUIPageGuid);

void UIPrefAdvancedDUIDialog::OnInitDialog()
{
    SetHeaderFont(IDC_PREF_HEADER1);

    mComboMenuBar.Attach(GetDlgItem(IDC_MENUBAR));
    mComboMenuBar.AddString(TEXT("Show"));
    mComboMenuBar.AddString(TEXT("Hide"));

    mComboStatusBar.Attach(GetDlgItem(IDC_STATUSBAR));
    mComboStatusBar.AddString(TEXT("Show"));
    mComboStatusBar.AddString(TEXT("Hide"));

    LoadUIState();
}

void UIPrefAdvancedDUIDialog::OnApply()
{
    SaveUIState();
    ApplySettings();
}

void UIPrefAdvancedDUIDialog::OnCommand(UINT code, int id, CWindow ctrl)
{
    if (code == CBN_SELCHANGE)
    {
        NotifyStateChanges(true);
    }
}

void UIPrefAdvancedDUIDialog::LoadUIState()
{
    mComboMenuBar.SetCurSel(OpenHacksVars::ShowMainMenu ? 0 : 1);
    mComboStatusBar.SetCurSel(OpenHacksVars::ShowStatusBar ? 0 : 1);
}

void UIPrefAdvancedDUIDialog::SaveUIState()
{
    OpenHacksVars::ShowMainMenu = mComboMenuBar.GetCurSel() == 1 ? false : true;
    OpenHacksVars::ShowStatusBar = mComboStatusBar.GetCurSel() == 1 ? false : true;
}

void UIPrefAdvancedDUIDialog::ApplySettings()
{
    auto& api = OpenHacksCore::Get();
    api.ShowOrHideMenuBar(OpenHacksVars::ShowMainMenu);
    api.ShowOrHideStatusBar(OpenHacksVars::ShowStatusBar);
}
