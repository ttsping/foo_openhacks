#include "pch.h"
#include "ui_pref_advanced.h"
#include "preferences_page_impl.h"
#include "hacks_core.h"
#include "hacks_vars.h"
#include "hacks_guids.h"
#include "advanced_settings.h"

DECLARE_PREFERENCES_PAGE("Open Hacks", UIPrefAdvancedDialog, 100.0, OpenHacksGuids::kAdvancedPageGuid, preferences_page::guid_advanced);

UIPrefAdvancedDialog::UIPrefAdvancedDialog(preferences_page_callback::ptr cb) : UIPrefDialogBase(cb), mListView(this, this)
{
}

void UIPrefAdvancedDialog::OnInitDialog()
{
    SetHeaderFont(IDC_PREF_HEADER1);

    mListView.CreateInDialog(*this, IDC_LIST_PROPERTIES);
    mDarkModeHooks.AddDialogWithControls(*this);

    CRect listRect;
    mListView.GetClientRect(listRect);
    int listWidth = listRect.Width();
    int scrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
    int availableWidth = listWidth - scrollbarWidth;
    if (availableWidth < 100)
    {
        availableWidth = listWidth;
    }

    int valueColWidth = MulDiv(availableWidth, 40, 100);
    int settingColWidth = availableWidth - valueColWidth;
    mListView.AddColumn("Setting", settingColWidth);
    mListView.AddColumn("Value", valueColWidth);

    LoadUIState();
    UpdateDescription();
}

void UIPrefAdvancedDialog::OnApply()
{
    SaveUIState();
    ApplySettings();
}

void UIPrefAdvancedDialog::listSelChanged(ctx_t)
{
    UpdateDescription();
}

void UIPrefAdvancedDialog::UpdateDescription()
{
    const size_t focusedItem = mListView.GetFocusItem();
    if (focusedItem != SIZE_MAX && focusedItem < mItems.size())
    {
        const auto& item = mItems[focusedItem];
        SetDlgItemText(IDC_DESC, Utility::ToUTF16(item.description).c_str());
    }
    else
    {
        SetDlgItemText(IDC_DESC, L"");
    }
}

void UIPrefAdvancedDialog::OnContextMenu(CWindow wnd, CPoint point)
{
    const size_t item = mListView.GetSingleSel();
    if (item == SIZE_MAX || item >= mItems.size())
        return;

    const auto& settingItem = mItems[item];

    // Menu command IDs
    enum
    {
        ID_RESET_TO_DEFAULT = 1
    };

    CMenu menu;
    WIN32_OP_D(menu.CreatePopupMenu());

    if (settingItem.value != settingItem.defaultValue)
    {
        menu.AppendMenu(MF_STRING, ID_RESET_TO_DEFAULT, L"Reset to default");
    }

    if (menu.GetMenuItemCount() == 0)
        return;

    const int cmd = menu.TrackPopupMenuEx(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, m_hWnd, nullptr);
    if (cmd == ID_RESET_TO_DEFAULT)
    {
        AdvancedSettings::ResetValue(settingItem.key);
        mItems[item].value = mItems[item].defaultValue;
        mItems[item].isModified = false;
        mListView.ReloadItem(item);
        NotifyStateChanges(true);
    }
}

void UIPrefAdvancedDialog::LoadUIState()
{
    mItems = AdvancedSettings::GetAllItems();
}

void UIPrefAdvancedDialog::SaveUIState()
{
    for (const auto& item : mItems)
    {
        if (item.isModified)
            AdvancedSettings::SetValue(item.key, item.value);
        else if (item.value == item.defaultValue)
            AdvancedSettings::ResetValue(item.key);
    }
}

void UIPrefAdvancedDialog::ApplySettings()
{
}

size_t UIPrefAdvancedDialog::listGetItemCount(ctx_t)
{
    return mItems.size();
}

pfc::string8 UIPrefAdvancedDialog::listGetSubItemText(ctx_t, size_t item, size_t subItem)
{
    if (item >= mItems.size())
    {
        return "";
    }

    const auto& settingItem = mItems[item];
    switch (subItem)
    {
    case 0:
        return settingItem.displayName.c_str();
    case 1:
        return settingItem.value.c_str();
    default:
        return "";
    }
}

void UIPrefAdvancedDialog::listItemAction(ctx_t, size_t item)
{
    mListView.TableEdit_Start(item, 1);
}

void UIPrefAdvancedDialog::listSubItemClicked(ctx_t, size_t item, size_t subItem)
{
}

bool UIPrefAdvancedDialog::listIsColumnEditable(ctx_t, size_t subItem)
{
    return subItem == 1;
}

void UIPrefAdvancedDialog::listSetEditField(ctx_t, size_t item, size_t subItem, const char* val)
{
    if (subItem == 1 && item < mItems.size())
    {
        mItems[item].value = val;
        mItems[item].isModified = (mItems[item].value != mItems[item].defaultValue);
        mListView.ReloadItem(item);
        NotifyStateChanges(true);
    }
}

IListControlOwnerDataSource::combo_t UIPrefAdvancedDialog::listGetCombo(ctx_t, size_t item, size_t subItem)
{
    combo_t result;
    if (subItem == 1 && item < mItems.size())
    {
        const auto& settingItem = mItems[item];
        if (settingItem.editStyle == SettingEditStyle::Combo)
        {
            unsigned defaultIndex = 0;
            for (size_t i = 0; i < settingItem.comboOptions.size(); ++i)
            {
                result.strings.add_item(settingItem.comboOptions[i].c_str());
                if (settingItem.comboOptions[i] == settingItem.defaultValue)
                {
                    defaultIndex = static_cast<unsigned>(i);
                }
            }
            result.iDefault = defaultIndex;
        }
    }
    return result;
}

bool UIPrefAdvancedDialog::listCanSelectItem(ctx_t, size_t item)
{
    return true;
}

CListControl::groupID_t UIPrefAdvancedDialog::listItemGroup(ctx_t, size_t item)
{
    if (item >= mItems.size())
    {
        return 0;
    }

    return static_cast<CListControl::groupID_t>(mItems[item].category) + 1;
}

pfc::string8 UIPrefAdvancedDialog::listGroupText(ctx_t, size_t baseItem)
{
    if (baseItem >= mItems.size())
    {
        return "";
    }
    switch (mItems[baseItem].category)
    {
    case SettingCategory::General:
        return "General";
    case SettingCategory::Appearance:
        return "Appearance";
    case SettingCategory::Behavior:
        return "Behavior";
    case SettingCategory::Portability:
        return "Portability";
    default:
        return "";
    }
}

CListControl::cellType_t UIPrefAdvancedDialog::listCellType(cellsCtx_t, size_t item, size_t subItem)
{
    if (subItem == 1 && item < mItems.size())
    {
        switch (mItems[item].editStyle)
        {
        case SettingEditStyle::Combo:
            return cell_combo;
        case SettingEditStyle::Dir:
            return cell_edit_dir;
        case SettingEditStyle::Text:
            return cell_edit;
        default:
            return cell_text;
        }
    }

    return cell_text;
}