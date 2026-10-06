#include "pch.h"
#include "hacks_log.h"
#include <cstdarg>
#include <cstdio>

namespace OpenHacksLog
{
    bool GDebugEnabled = false;

    void LogPrintf(const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        console::info(buffer);
    }
}
