#pragma once
#include <type_traits>
#include <optional>
#include <vector>

enum WindowFrameStyle
{
    WindowFrameStyleDefault = 0,
    WindowFrameStyleNoBorder = 1,
};

struct WindowState
{
    bool fullscreen = false;
    DWORD style = 0;
    WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
};

struct FileTypeFilter
{
    const wchar_t* Name = nullptr;    // Display name shown in filter dropdown (e.g., "Text Files")
    const wchar_t* Pattern = nullptr; // File pattern (e.g., "*.txt")
};

struct FileDialogOptions
{
    HWND ParentWindow = nullptr;              // Parent window handle
    const wchar_t* Title = nullptr;           // Dialog title
    const wchar_t* InitialPath = nullptr;     // Initial folder path
    const wchar_t* DefaultFileName = nullptr; // Default file name (for save mode)
    const FileTypeFilter* Filters = nullptr;  // File type filters array
    size_t FilterCount = 0;                   // Number of filters
    bool SaveMode = false;                    // True for save dialog, false for open
    bool PickFolders = false;                 // True to pick folders instead of files
};

namespace Utility
{
template <typename F, typename = std::enable_if_t<std::is_function<typename std::remove_pointer<F>::type>::value>>
bool GetProcAddress(HMODULE h, const char* funcName, F& f)
{
    if (auto ptr = ::GetProcAddress(h, funcName))
    {
        f = reinterpret_cast<F>(ptr);
        return true;
    }

    f = nullptr;
    return false;
}

RECT& ClientToScreen(HWND wnd, RECT& rc);
RECT& ScreenToClient(HWND wnd, RECT& rc);

bool IsCompositionEnabled();
bool IsWindows11OrGreater();
bool EnableWindowShadow(HWND window, bool enable);
uint32_t GetDPI(HWND window);
int32_t GetSystemMetricsForDpi(int32_t index, uint32_t dpi);

// Window state management
void Maximize(HWND wnd, WindowState& state);
void Restore(HWND wnd, WindowState& state);
bool IsMaximized(HWND wnd);
bool IsMinimized(HWND wnd);
bool IsFullscreenOrMaximized(HWND wnd);
void ApplyWindowFrameStyle(HWND wnd, WindowFrameStyle style);

// Fullscreen management
void EnterFullscreen(HWND wnd, WindowState& state);
void ExitFullscreen(HWND wnd, WindowState& state);
bool IsFullscreen(HWND wnd);

std::optional<std::wstring> ShowFileDialog(const FileDialogOptions& options);
std::optional<std::wstring> PickFolder(HWND parent, const wchar_t* initialPath = nullptr, const wchar_t* title = nullptr);
std::optional<std::wstring> PickFile(HWND parent, const wchar_t* initialPath = nullptr, const wchar_t* title = nullptr, const FileTypeFilter* filters = nullptr,
                                     size_t filterCount = 0);
std::optional<std::wstring> PickFileForSave(HWND parent, const wchar_t* initialPath = nullptr, const wchar_t* title = nullptr,
                                            const wchar_t* defaultFileName = nullptr, const FileTypeFilter* filters = nullptr, size_t filterCount = 0);

bool DirectoryExists(wstring_view_t path);
bool DirectoryExists(string_view_t path);

memory_block_t ReadFileToBuffer(wstring_view_t path);
memory_block_t ReadFileToBuffer(string_view_t path);

//! Callback for enumerating files in a directory
//! @param filePath Full file path (directory + filename)
//! @param context User-provided context
using DirectoryEnumCallback = bool (*)(wstring_view_t filePath, void* context);

//! Enumerate files in a directory
//! @param dirPath Directory path
//! @param pattern Pattern string, e.g. "*.dll" or "txt;doc;pdf" or "*" for all files
//! @param recursive Whether to enumerate subdirectories
//! @param callback Callback function called for each file
//! @param context User-provided context passed to callback
bool EnumDirectoryFiles(wstring_view_t dirPath, wstring_view_t pattern, bool recursive, DirectoryEnumCallback callback, void* context);
bool EnumDirectoryFiles(string_view_t dirPath, string_view_t pattern, bool recursive, DirectoryEnumCallback callback, void* context);

//! Enumerate all files in a directory (non-recursive)
bool EnumDirectoryFiles(wstring_view_t dirPath, wstring_view_t pattern, DirectoryEnumCallback callback, void* context);
bool EnumDirectoryFiles(string_view_t dirPath, string_view_t pattern, DirectoryEnumCallback callback, void* context);

//! Simple version that returns list of matching files
//! @param pattern Pattern string, e.g. "*.dll" or "txt;doc;pdf" or "*" for all files
//! @param recursive Whether to enumerate subdirectories
//! @return Vector of matching file names (empty on error or no files)
std::vector<wstring_t> EnumDirectoryFiles(wstring_view_t dirPath, wstring_view_t pattern, bool recursive);
std::vector<string_t> EnumDirectoryFiles(string_view_t dirPath, string_view_t pattern, bool recursive);

//! Get foobar2000 profile path
wstring_t GetProfilePath();

} // namespace Utility