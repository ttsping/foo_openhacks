// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#define _WIN32_WINNT 0x0A00      // _WIN32_WINNT_WIN10
#define _WIN32_IE 0x0A00         // _WIN32_IE_IE110
#define NTDDI_VERSION 0x0A000002 // NTDDI_WIN10_RS1

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

// workaround for STL mutex changes.
// https://github.com/microsoft/STL/wiki/Changelog#vs-2022-1710
#ifndef _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
#endif

// ATL/WTL Headers
#include <atlbase.h>
#include <atlwin.h>
#include <atlapp.h>
#include <atlframe.h>
#include <atlsplit.h>
#include <atlddx.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atldlgs.h>
#include <atlcrack.h>
#include <atlmisc.h>

#include <shlwapi.h>
#include <mmsystem.h>
#include <wrl/client.h>
#include <map>
#include <vector>
#include <mutex>
#include <string>
#include <memory>

#include <foobar2000/SDK/foobar2000-all.h>
#include "hacks_priv.h"
#include "str.h"
#include "win32_types.h"
#include "win32_utils.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace Microsoft::WRL;

#endif // PCH_H
