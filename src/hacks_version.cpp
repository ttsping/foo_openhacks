#include "pch.h"
#include "hacks_version.h"

static string_view_t OpenHacksFormatAbort = R"OH(OpenHacks v%s for foobar2000
author: ohyeah

MSVC: v%s %s
Compile: %s
Foobar2000 SDK: %s

Third-party libraries:

MinHook - The Minimalistic API Hooking Library for x64/x86
Copyright (C) 2009-2017 Tsuda Kageyu.
All rights reserved.


)OH";

const char* GetOpenHacksAbout()
{
    static char about[8 * 1024] = {0};
    if (about[0] == '\0')
    {
        char ver[64] = {0};
        unsigned major = _MSC_FULL_VER / 10000000;
        unsigned minor = _MSC_FULL_VER / 100000 - major * 100;
        unsigned build = _MSC_FULL_VER - major * 10000000 - minor * 100000;
        sprintf_s(ver, "%d.%d.%d.%02d", major, minor, build, _MSC_BUILD);
        sprintf_s(about, OpenHacksFormatAbort.data(), HACKS_VERSION, ver, STRINGIFY(_MSVC_LANG), "" __TIME__ " " __DATE__, STRINGIFY(FOOBAR2000_SDK_VERSION));
    }

    return about;
}
