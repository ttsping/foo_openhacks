#pragma once
#include "ui_dialog_base.h"
#include "ui_list_control_ex.h"
#include "advanced_settings.h"

class UIPrefAdvancedDialog : public UIPrefDialogBase<UIPrefAdvancedDialog, IDD_PREF_ADVANCED>,
                             private IListControlOwnerDataSource,
                             private IListControlOwnerDataCells
{
public:
    UIPrefAdvancedDialog(preferences_page_callback::ptr cb);

    BEGIN_MSG_MAP(UIPrefAdvancedDialog)
        CHAIN_MSG_MAP(UIPrefDialogBase)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        REFLECT_NOTIFICATIONS_EX()
    END_MSG_MAP()

protected:
    void OnInitDialog() override;
    void OnApply() override;

private:
    void OnContextMenu(CWindow wnd, CPoint point);
    void UpdateDescription();

    void LoadUIState();
    void SaveUIState();
    void ApplySettings();

    // IListControlOwnerDataSource
    size_t listGetItemCount(ctx_t) override;
    pfc::string8 listGetSubItemText(ctx_t, size_t item, size_t subItem) override;
    void listItemAction(ctx_t, size_t item) override;
    void listSubItemClicked(ctx_t, size_t item, size_t subItem) override;
    bool listIsColumnEditable(ctx_t, size_t subItem) override;
    void listSetEditField(ctx_t, size_t item, size_t subItem, const char* val) override;
    IListControlOwnerDataSource::combo_t listGetCombo(ctx_t, size_t item, size_t subItem) override;
    bool listCanSelectItem(ctx_t, size_t item) override;
    CListControl::groupID_t listItemGroup(ctx_t, size_t item) override;
    pfc::string8 listGroupText(ctx_t, size_t baseItem) override;
    void listSelChanged(ctx_t) override;

    // IListControlOwnerDataCells
    CListControl::cellType_t listCellType(cellsCtx_t, size_t item, size_t subItem) override;

private:
    CListControlEx mListView;
    std::vector<AdvancedSettingItem> mItems;
};