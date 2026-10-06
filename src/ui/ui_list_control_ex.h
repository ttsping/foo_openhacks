#pragma once

#include <memory>
#include <libPPUI/CListControlOwnerData.h>
#include <libPPUI/CListControl-Cells-Compat.h>
#include <libPPUI/CEditWithButtons.h>
#include "win32_utils.h"

// Custom cell types for edit with file/folder browse button
// These inherit from CListCell_Edit to maintain ClickToEdit behavior
// CListControlEx will detect these types and add browse functionality
class CListCell_EditBrowseFile : public CListCell_Edit
{
};

class CListCell_EditBrowseFolder : public CListCell_Edit
{
};

#define cell_combo         &PFC_SINGLETON(CListCell_Combo)
#define cell_edit          &PFC_SINGLETON(CListCell_Edit)
#define cell_edit_file     &PFC_SINGLETON(CListCell_EditBrowseFile)
#define cell_edit_dir      &PFC_SINGLETON(CListCell_EditBrowseFolder)


class CListControlEx : public CListControlOwnerDataCells
{
public:
    CListControlEx(IListControlOwnerDataSource* source, IListControlOwnerDataCells* cells);
    ~CListControlEx();

protected:
    void TableEdit_SetItemFocus(t_size item, t_size subItem) override;
    void RequestEditItem(size_t item, size_t subItem) override;
    void TableEdit_Finished() override;

private:
    void SetupBrowseEdit(HWND editWnd, size_t item, size_t subItem);
    void OnBrowseClicked();

    IListControlOwnerDataCells* mCellsInterface = nullptr;
    CEditWithButtons mEditWithBrowse;
    size_t mEditItem = SIZE_MAX;
    size_t mEditSubItem = SIZE_MAX;
};