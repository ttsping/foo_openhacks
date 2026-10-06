#include "pch.h"
#include "win32_utils.h"
#include "hacks_vars.h"
#include <dwmapi.h>
#include <shlobj.h>

#pragma comment(lib, "dwmapi.lib")

namespace Utility
{
int(WINAPI* pfnGetSystemMetricsForDpi)(int, UINT) = nullptr;
UINT(WINAPI* pfnGetDpiForSystem)() = nullptr;
UINT(WINAPI* pfnGetDpiForWindow)(HWND) = nullptr;
DPI_AWARENESS_CONTEXT(WINAPI* pfnGetThreadDpiAwarenessContext)() = nullptr;
DPI_AWARENESS(WINAPI* pfnGetAwarenessFromDpiAwarenessContext)(DPI_AWARENESS_CONTEXT value) = nullptr;

static std::once_flag staticLoadFlag;

static void LoadUtilityProc()
{
    // clang-format off
    std::call_once(staticLoadFlag, [&]()
    {
        if (HMODULE user32 = GetModuleHandleW(L"user32.dll"))
        {
            std::ignore = GetProcAddress(user32, "GetSystemMetricsForDpi", pfnGetSystemMetricsForDpi);
            std::ignore = GetProcAddress(user32, "GetDpiForSystem", pfnGetDpiForSystem);
            std::ignore = GetProcAddress(user32, "GetDpiForWindow", pfnGetDpiForWindow);
            std::ignore = GetProcAddress(user32, "GetThreadDpiAwarenessContext", pfnGetThreadDpiAwarenessContext);
            std::ignore = GetProcAddress(user32, "GetAwarenessFromDpiAwarenessContext", pfnGetAwarenessFromDpiAwarenessContext);
        }
    });
    // clang-format on
}

RECT& ClientToScreen(HWND wnd, RECT& rc)
{
    LPPOINT pt = (LPPOINT)&rc;
    ::ClientToScreen(wnd, pt);
    ::ClientToScreen(wnd, pt + 1);
    return rc;
}

RECT& ScreenToClient(HWND wnd, RECT& rc)
{
    LPPOINT pt = (LPPOINT)&rc;
    ::ScreenToClient(wnd, pt);
    ::ScreenToClient(wnd, pt + 1);
    return rc;
}

bool IsCompositionEnabled()
{
    BOOL composition_enabled = FALSE;
    return SUCCEEDED(DwmIsCompositionEnabled(&composition_enabled)) && composition_enabled;
}

bool IsWindows11OrGreater()
{
    // Use RtlGetVersion to get the real Windows version
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtDll)
        return false;

    typedef LONG(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    auto pRtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hNtDll, "RtlGetVersion"));
    if (!pRtlGetVersion)
        return false;

    RTL_OSVERSIONINFOW osvi = {sizeof(osvi)};
    if (pRtlGetVersion(&osvi) != 0)
        return false;

    // Windows 11 is version 10.0.22000 or higher
    return osvi.dwMajorVersion > 10 || (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion > 0) ||
           (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber >= 22000);
}

bool EnableWindowShadow(HWND window, bool enable)
{
    // Only apply shadow on Windows 11 or later
    if (!IsWindows11OrGreater())
        return false;

    if (IsCompositionEnabled())
    {
        static const MARGINS shadow_state[2]{{0, 0, 0, 0}, {1, 1, 1, 1}};
        const bool result = SUCCEEDED(DwmExtendFrameIntoClientArea(window, &shadow_state[enable ? 1 : 0]));
        const DWORD policy = DWMNCRP_ENABLED;
        std::ignore = DwmSetWindowAttribute(window, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        return result;
    }

    return false;
}

uint32_t GetDPI(HWND window)
{
    LoadUtilityProc();

    // if (window != nullptr && pfnGetDpiForWindow != nullptr)
    //     return pfnGetDpiForWindow(window);

    // if (pfnGetDpiForSystem != nullptr)
    //     return pfnGetDpiForSystem();

    if (HDC dc = GetDC(HWND_DESKTOP))
    {
        auto dpi = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(window, dc);
        return static_cast<uint32_t>(dpi);
    }

    return USER_DEFAULT_SCREEN_DPI;
}

int32_t GetSystemMetricsForDpi(int32_t index, uint32_t dpi)
{
    LoadUtilityProc();

    if (pfnGetSystemMetricsForDpi != nullptr)
        return pfnGetSystemMetricsForDpi(index, dpi);

    const int32_t rc = GetSystemMetrics(index);
    if (dpi == USER_DEFAULT_SCREEN_DPI)
        return rc;

    if (pfnGetThreadDpiAwarenessContext != nullptr && pfnGetAwarenessFromDpiAwarenessContext != nullptr)
    {
        const auto awareness = pfnGetAwarenessFromDpiAwarenessContext(pfnGetThreadDpiAwarenessContext());
        if (awareness == DPI_AWARENESS_UNAWARE || awareness == DPI_AWARENESS_INVALID)
        {
            const double scaleFactor = static_cast<double>(dpi) / 96.0;
            return static_cast<int>(std::round(rc * scaleFactor));
        }
    }

    return rc;
}

void Maximize(HWND wnd, WindowState& state)
{
    state.style = static_cast<DWORD>(GetWindowLongPtr(wnd, GWL_STYLE));
    GetWindowPlacement(wnd, &state.wp);

    const bool hasCaption = (state.style & WS_CAPTION) != 0;
    if (hasCaption)
    {
        ShowWindow(wnd, SW_MAXIMIZE);
    }
    else
    {
        // NoCaption/NoBorder: switch to NoBorder style and disable shadow
        LONG newStyle = state.style & ~(WS_CAPTION | WS_THICKFRAME);
        SetWindowLongPtr(wnd, GWL_STYLE, newStyle);

        RECT workArea = {};
        if (HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST))
        {
            MONITORINFO mi = {sizeof(MONITORINFO)};
            if (GetMonitorInfo(monitor, &mi))
                workArea = mi.rcWork;
            else
                SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); // Fallback
        }
        else
        {
            SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); // Fallback
        }

        SetWindowPos(wnd, HWND_TOP, workArea.left, workArea.top, workArea.right - workArea.left, workArea.bottom - workArea.top,
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

void Restore(HWND wnd, WindowState& state)
{
    if (state.wp.length == 0)
        return;

    const bool hadCaption = (state.style & WS_CAPTION) != 0;
    if (hadCaption)
    {
        ShowWindow(wnd, SW_RESTORE);
    }
    else
    {
        SetWindowLongPtr(wnd, GWL_STYLE, state.style);

        // Restore from saved WINDOWPLACEMENT
        state.wp.showCmd = SW_SHOWNORMAL;
        SetWindowPlacement(wnd, &state.wp);

        // Shadow is rendered automatically by THICKFRAME

        // Notify frame changes
        SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

bool IsMaximized(HWND wnd)
{
    WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
    GetWindowPlacement(wnd, &wp);

    if (wp.showCmd == SW_SHOWMAXIMIZED)
        return true;

    const DWORD style = static_cast<DWORD>(GetWindowLongPtr(wnd, GWL_STYLE));
    const bool hasCaption = (style & WS_CAPTION) != 0;
    if (!hasCaption)
    {
        // For custom maximized (NoCaption/NoBorder), check if window matches work area
        RECT workArea;
        if (HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST))
        {
            MONITORINFO mi = {sizeof(MONITORINFO)};
            if (GetMonitorInfo(monitor, &mi))
                workArea = mi.rcWork;
            else
                SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); // Fallback
        }
        else
        {
            SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); // Fallback
        }

        RECT windowRect;
        GetWindowRect(wnd, &windowRect);

        // Check if window rect matches work area
        return windowRect.left == workArea.left && windowRect.top == workArea.top && windowRect.right == workArea.right && windowRect.bottom == workArea.bottom;
    }

    return false;
}

bool IsMinimized(HWND wnd)
{
    WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
    GetWindowPlacement(wnd, &wp);
    return wp.showCmd == SW_SHOWMINIMIZED || wp.showCmd == SW_MINIMIZE;
}

bool IsFullscreenOrMaximized(HWND wnd)
{
    return IsFullscreen(wnd) || IsMaximized(wnd);
}

void ApplyWindowFrameStyle(HWND wnd, WindowFrameStyle style)
{
    const LONG currentStyle = static_cast<LONG>(GetWindowLongPtr(wnd, GWL_STYLE));
    LONG newStyle = currentStyle;

    switch (style)
    {
    case WindowFrameStyleDefault:
        newStyle |= (WS_CAPTION | WS_THICKFRAME);
        break;

    case WindowFrameStyleNoBorder:
        newStyle |= WS_THICKFRAME;
        newStyle &= ~(WS_CAPTION);
        break;

    default:
        break;
    }

    if (currentStyle == newStyle)
        return;

    SetWindowLongPtr(wnd, GWL_STYLE, newStyle);

    // Notify frame changes
    SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void EnterFullscreen(HWND wnd, WindowState& state)
{
    state.style = static_cast<DWORD>(GetWindowLongPtr(wnd, GWL_STYLE));
    GetWindowPlacement(wnd, &state.wp);

    LONG newStyle = state.style & ~(WS_CAPTION | WS_THICKFRAME);
    SetWindowLongPtr(wnd, GWL_STYLE, newStyle);

    RECT screenArea = {};
    if (HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST))
    {
        MONITORINFO mi = {sizeof(MONITORINFO)};
        if (GetMonitorInfo(monitor, &mi))
            screenArea = mi.rcMonitor;
        else
            SystemParametersInfo(SPI_GETWORKAREA, 0, &screenArea, 0); // Fallback
    }
    else
    {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &screenArea, 0); // Fallback
    }

    SetWindowPos(wnd, HWND_TOP, screenArea.left, screenArea.top, screenArea.right - screenArea.left, screenArea.bottom - screenArea.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void ExitFullscreen(HWND wnd, WindowState& state)
{
    if (state.wp.length == 0 || state.fullscreen == false)
        return;

    // Restore original style
    SetWindowLongPtr(wnd, GWL_STYLE, state.style);

    // Restore from saved WINDOWPLACEMENT
    SetWindowPlacement(wnd, &state.wp);

    // Shadow is rendered automatically by THICKFRAME

    // Notify frame changes
    SetWindowPos(wnd, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool IsFullscreen(HWND wnd)
{
    RECT windowRect;
    GetWindowRect(wnd, &windowRect);

    RECT monitorRect;
    if (HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST))
    {
        MONITORINFO mi = {sizeof(MONITORINFO)};
        if (GetMonitorInfo(monitor, &mi))
            monitorRect = mi.rcMonitor;
        else
            return false;
    }
    else
    {
        return false;
    }

    return windowRect.left == monitorRect.left && windowRect.top == monitorRect.top && windowRect.right == monitorRect.right &&
           windowRect.bottom == monitorRect.bottom;
}

std::optional<std::wstring> ShowFileDialog(const FileDialogOptions& options)
{
    ComPtr<IFileDialog> dialog;
    HRESULT hr;
    if (options.SaveMode)
    {
        hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    }
    else
    {
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    }

    if (FAILED(hr) || !dialog)
        return std::nullopt;

    if (options.Title)
        dialog->SetTitle(options.Title);

    DWORD dwOptions = 0;
    dialog->GetOptions(&dwOptions);

    if (options.PickFolders)
        dialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
    else if (!options.SaveMode)
        dialog->SetOptions(dwOptions | FOS_FILEMUSTEXIST);
    else
        dialog->SetOptions(dwOptions | FOS_OVERWRITEPROMPT);

    if (!options.PickFolders && options.Filters && options.FilterCount > 0)
    {
        std::vector<COMDLG_FILTERSPEC> filterSpecs;
        filterSpecs.reserve(options.FilterCount);

        for (size_t i = 0; i < options.FilterCount; ++i)
        {
            filterSpecs.push_back({options.Filters[i].Name, options.Filters[i].Pattern});
        }

        dialog->SetFileTypes(static_cast<UINT>(filterSpecs.size()), filterSpecs.data());
        dialog->SetFileTypeIndex(1);
    }

    if (options.SaveMode && options.DefaultFileName)
    {
        dialog->SetFileName(options.DefaultFileName);
    }

    if (options.InitialPath && options.InitialPath[0] != 0)
    {
        ComPtr<IShellItem> folderItem;
        hr = SHCreateItemFromParsingName(options.InitialPath, nullptr, IID_PPV_ARGS(&folderItem));
        if (SUCCEEDED(hr) && folderItem)
        {
            dialog->SetDefaultFolder(folderItem.Get());
        }
    }

    hr = dialog->Show(options.ParentWindow);
    if (FAILED(hr))
        return std::nullopt;

    ComPtr<IShellItem> resultItem;
    hr = dialog->GetResult(&resultItem);
    if (FAILED(hr) || !resultItem)
        return std::nullopt;

    LPWSTR pathStr = nullptr;
    hr = resultItem->GetDisplayName(SIGDN_FILESYSPATH, &pathStr);
    if (FAILED(hr) || !pathStr)
        return std::nullopt;

    std::wstring result(pathStr);
    CoTaskMemFree(pathStr);

    return result;
}

std::optional<std::wstring> PickFolder(HWND parent, const wchar_t* initialPath, const wchar_t* title)
{
    FileDialogOptions options;
    options.ParentWindow = parent;
    options.Title = title;
    options.InitialPath = initialPath;
    options.PickFolders = true;
    return ShowFileDialog(options);
}

std::optional<std::wstring> PickFile(HWND parent, const wchar_t* initialPath, const wchar_t* title, const FileTypeFilter* filters, size_t filterCount)
{
    FileDialogOptions options;
    options.ParentWindow = parent;
    options.Title = title;
    options.InitialPath = initialPath;
    options.Filters = filters;
    options.FilterCount = filterCount;
    return ShowFileDialog(options);
}

std::optional<std::wstring> PickFileForSave(HWND parent, const wchar_t* initialPath, const wchar_t* title, const wchar_t* defaultFileName,
                                            const FileTypeFilter* filters, size_t filterCount)
{
    FileDialogOptions options;
    options.ParentWindow = parent;
    options.Title = title;
    options.InitialPath = initialPath;
    options.DefaultFileName = defaultFileName;
    options.Filters = filters;
    options.FilterCount = filterCount;
    options.SaveMode = true;
    return ShowFileDialog(options);
}

bool DirectoryExists(wstring_view_t path)
{
    DWORD attrs = GetFileAttributesW(path.data());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

bool DirectoryExists(string_view_t path)
{
    return DirectoryExists(Utility::ToUTF16(path));
}

memory_block_t ReadFileToBuffer(wstring_view_t path)
{
    memory_block_t result;

    HANDLE hFile = CreateFileW(path.data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return result;
    }

    DWORD fileSize = GetFileSize(hFile, nullptr);
    if (fileSize == INVALID_FILE_SIZE)
    {
        DWORD err = GetLastError();
        CloseHandle(hFile);
        if (err != NO_ERROR)
        {
            return result;
        }
    }
    if (fileSize == 0)
    {
        CloseHandle(hFile);
        return result;
    }

    result.resize(fileSize);
    DWORD bytesRead = 0;
    if (!::ReadFile(hFile, result.data(), fileSize, &bytesRead, nullptr))
    {
        result.clear();
    }
    else
    {
        result.resize(bytesRead);
    }

    CloseHandle(hFile);
    return result;
}

memory_block_t ReadFileToBuffer(string_view_t path)
{
    return ReadFileToBuffer(Utility::ToUTF16(path));
}

namespace
{

// Check if a filename matches any of the patterns, Patterns can be like "*.dll" or "txt;doc;pdf" or "*"
bool MatchPattern(wstring_view_t fileName, const std::wstring& patternStr)
{
    if (patternStr.empty() || patternStr == L"*")
    {
        return true;
    }

    // Split by semicolon
    size_t start = 0;
    while (start < patternStr.size())
    {
        size_t pos = patternStr.find(L';', start);
        std::wstring pattern = (pos == std::wstring::npos) ? patternStr.substr(start) : patternStr.substr(start, pos - start);

        // Trim whitespace only (do NOT trim leading '*' as it's part of wildcard pattern)
        while (!pattern.empty() && pattern.front() == L' ')
            pattern.erase(pattern.begin());
        while (!pattern.empty() && pattern.back() == L' ')
            pattern.pop_back();

        if (!pattern.empty())
        {
            // Check if filename matches pattern
            if (pattern == L"*")
            {
                return true;
            }

            // Simple wildcard match: *.ext
            if (pattern.front() == L'*' && pattern.size() > 1)
            {
                std::wstring ext = pattern.substr(1);
                if (fileName.size() >= ext.size())
                {
                    wstring_view_t fileExt = fileName.substr(fileName.size() - ext.size());
                    if (_wcsicmp(fileExt.data(), ext.c_str()) == 0)
                    {
                        return true;
                    }
                }
            }
            else if (_wcsicmp(fileName.data(), pattern.c_str()) == 0)
            {
                return true;
            }
        }

        if (pos == std::wstring::npos)
            break;
        start = pos + 1;
    }

    return false;
}


bool EnumDirectoryFilesInternal(wstring_view_t dirPath, const std::wstring& patternStr, bool recursive, DirectoryEnumCallback callback, void* context)
{
    std::wstring searchDir(dirPath.data(), dirPath.size());
    if (!searchDir.empty() && searchDir.back() != L'\\' && searchDir.back() != L'/')
    {
        searchDir += L"\\";
    }

    std::wstring searchPattern = searchDir + L"*.*";
    WIN32_FIND_DATAW fd = {};
    HANDLE hFind = ::FindFirstFileW(searchPattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    bool success = true;
    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Skip . and ..
            if (fd.cFileName[0] == L'.' && (fd.cFileName[1] == L'\0' || (fd.cFileName[1] == L'.' && fd.cFileName[2] == L'\0')))
            {
                continue;
            }

            if (recursive)
            {
                std::wstring subDir = searchDir + fd.cFileName;
                if (!EnumDirectoryFilesInternal(subDir, patternStr, recursive, callback, context))
                {
                    success = false;
                    break;
                }
            }
        }
        else
        {
            wstring_view_t fileName(fd.cFileName);
            if (MatchPattern(fileName, patternStr))
            {
                std::wstring fullPath = searchDir + fd.cFileName;
                if (!callback(wstring_view_t(fullPath), context))
                {
                    success = false;
                    break;
                }
            }
        }
    }
    while (::FindNextFileW(hFind, &fd));

    ::FindClose(hFind);
    return success;
}

} // anonymous namespace

bool EnumDirectoryFiles(wstring_view_t dirPath, wstring_view_t pattern, bool recursive, DirectoryEnumCallback callback, void* context)
{
    if (callback == nullptr)
    {
        return false;
    }

    std::wstring patternStr(pattern.data(), pattern.size());
    return EnumDirectoryFilesInternal(dirPath, patternStr, recursive, callback, context);
}

bool EnumDirectoryFiles(string_view_t dirPath, string_view_t pattern, bool recursive, DirectoryEnumCallback callback, void* context)
{
    return EnumDirectoryFiles(Utility::ToUTF16(dirPath), Utility::ToUTF16(pattern), recursive, callback, context);
}

// Legacy non-recursive versions
bool EnumDirectoryFiles(wstring_view_t dirPath, wstring_view_t pattern, DirectoryEnumCallback callback, void* context)
{
    return EnumDirectoryFiles(dirPath, pattern, false, callback, context);
}

bool EnumDirectoryFiles(string_view_t dirPath, string_view_t pattern, DirectoryEnumCallback callback, void* context)
{
    return EnumDirectoryFiles(Utility::ToUTF16(dirPath), Utility::ToUTF16(pattern), false, callback, context);
}

namespace
{
bool CollectFilesCallback(wstring_view_t filePath, void* context)
{
    auto* files = static_cast<std::vector<wstring_t>*>(context);
    files->emplace_back(filePath);
    return true;
}
} // anonymous namespace

std::vector<wstring_t> EnumDirectoryFiles(wstring_view_t dirPath, wstring_view_t pattern, bool recursive)
{
    std::vector<wstring_t> files;
    EnumDirectoryFiles(dirPath, pattern, recursive, CollectFilesCallback, &files);
    return files;
}

std::vector<string_t> EnumDirectoryFiles(string_view_t dirPath, string_view_t pattern, bool recursive)
{
    auto wideFiles = EnumDirectoryFiles(Utility::ToUTF16(dirPath), Utility::ToUTF16(pattern), recursive);
    std::vector<string_t> result;
    result.reserve(wideFiles.size());
    for (const auto& f : wideFiles)
    {
        result.emplace_back(Utility::ToUTF8(f));
    }
    return result;
}

wstring_t GetProfilePath()
{
    const char* profilePath = core_api::get_profile_path();
    if (profilePath == nullptr || profilePath[0] == '\0')
    {
        return {};
    }

    // Convert file:// path to Windows native path
    pfc::string8 displayPath;
    filesystem::g_get_display_path(profilePath, displayPath);

    if (displayPath.is_empty())
    {
        return {};
    }

    return Utility::ToUTF16(displayPath.c_str());
}

} // namespace Utility
