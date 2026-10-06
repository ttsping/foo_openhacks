#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#ifndef FORCEINLINE
#define FORCEINLINE __forceinline
#endif

#define _STR(x) #x
#define STRINGIFY(x) _STR(x)

using memory_block_t = std::vector<uint8_t>;
using string_t = std::string;
using string_view_t = std::string_view;
using wstring_t = std::wstring;
using wstring_view_t = std::wstring_view;

// constants
constexpr static wstring_view_t kDUIMainWindowClassName = L"{97E27FAA-C0B3-4b8e-A693-ED7881E99FC1}";
constexpr static wstring_view_t kDUIRebarWindowClassName = L"ATL:ReBarWindow32";
constexpr static wstring_view_t kDUIMainMenuBandClassName = L"{5EF9C83C-AB6A-49d7-874A-76428EB4D1EA}";
constexpr static wstring_view_t kDUIStatusBarClassName = L"ATL:msctls_statusbar32";
