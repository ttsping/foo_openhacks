#pragma once
#include <type_traits>
#include <optional>
#include <Windows.h>

enum WindowFrameStyle
{
    WindowFrameStyleDefault = 0,
    WindowFrameStyleNoCaption,
    WindowFrameStyleNoBorder,
};

struct WindowState
{
    DWORD style = 0;
    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
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
bool EnableWindowShadow(HWND window, bool enable);
uint32_t GetDPI(HWND window);
int32_t GetSystemMetricsForDpi(int32_t index, uint32_t dpi);

// Window state management
void Maximize(HWND wnd, WindowState& state);
void Restore(HWND wnd, WindowState& state);
bool IsMaximized(HWND wnd);
bool IsMinimized(HWND wnd);
void ApplyWindowFrameStyle(HWND wnd, WindowFrameStyle style);
} // namespace Utility