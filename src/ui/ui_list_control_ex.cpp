#include "pch.h"
#include "ui_list_control_ex.h"
#include "win32_utils.h"

static const wchar_t* kBrowseButtonText = L"...";

CListControlEx::CListControlEx(IListControlOwnerDataSource* source, IListControlOwnerDataCells* cells)
    : CListControlOwnerDataCells(source, cells)
    , mCellsInterface(cells)
    , mEditItem(SIZE_MAX)
    , mEditSubItem(SIZE_MAX)
{
}

CListControlEx::~CListControlEx()
{
}

void CListControlEx::TableEdit_SetItemFocus(t_size item, t_size subItem)
{
    __super::TableEdit_SetItemFocus(item, subItem);
    UpdateWindow();
}

void CListControlEx::RequestEditItem(size_t item, size_t subItem)
{
    if (HWND editWnd = __super::TableEdit_Start(item, subItem))
        SetupBrowseEdit(editWnd, item, subItem);
}

void CListControlEx::TableEdit_Finished()
{
    if (mEditWithBrowse.IsWindow())
    {
        POINT pt = {};
        GetCursorPos(&pt);
        if (auto rc = mEditWithBrowse.RectOfButton(kBrowseButtonText); rc.PtInRect(pt))
            OnBrowseClicked();
    }
    
    __super::TableEdit_Finished();

    mEditItem = SIZE_MAX;
    mEditSubItem = SIZE_MAX;
}

void CListControlEx::SetupBrowseEdit(HWND editWnd, size_t item, size_t subItem)
{
    const auto cellType = GetCellType(item, subItem);
    if (cellType != cell_edit_file && cellType != cell_edit_dir)
        return;

    mEditItem = item;
    mEditSubItem = subItem;

    mEditWithBrowse.SubclassWindow(editWnd);

    static bool browseButtonAdded = false;
    if (!browseButtonAdded)
    {
        mEditWithBrowse.AddButton(kBrowseButtonText, []() { });
        browseButtonAdded = true;
    }

    // Set appropriate shell autocomplete based on browse type
    if (cellType == cell_edit_dir)
        mEditWithBrowse.SetShellFolderAutoComplete();
    else
        mEditWithBrowse.SetShellFileAutoComplete();
}

void CListControlEx::OnBrowseClicked()
{
    if (mEditItem == SIZE_MAX || mEditSubItem == SIZE_MAX)
        return;

    const auto cellType = GetCellType(mEditItem, mEditSubItem);
    if (cellType != cell_edit_file && cellType != cell_edit_dir)
        return;
    // Get initial path from current edit control text
    pfc::string8_fast initialPath;
    t_size lineCount = 0;
    TableEdit_GetField(mEditItem, mEditSubItem, initialPath, lineCount);
    const auto initialPathW = Utility::ToUTF16(initialPath.c_str());
    FileDialogOptions options;
    options.ParentWindow = GetParent();
    options.InitialPath = initialPathW.c_str();
    options.PickFolders = (cellType == cell_edit_dir);
    if (auto result = Utility::ShowFileDialog(options))
        TableEdit_SetField(mEditItem, mEditSubItem, Utility::ToUTF8(*result).c_str());
}