#pragma once

#include "advanced_settings.h"

//! Logging utilities with [OpenHacks] prefix
namespace OpenHacksLog
{
    //! Debug logging enabled flag - initialized at after_config_read stage
    extern bool GDebugEnabled;

    //! Initialize logging system - called at after_config_read stage
    inline void Initialize()
    {
        GDebugEnabled = AdvancedSettings::GetBooleanValue(AdvancedSettings::kDebugLoggingEnabled, false);
    }

    //! Internal logging helper - formats message and outputs to console
    void LogPrintf(const char* format, ...);
}

//! Log informational message (supports standard printf format specifiers)
#define LOG(...) OpenHacksLog::LogPrintf("[OpenHacks] " __VA_ARGS__)

//! Log error message
#define LOGE(...) OpenHacksLog::LogPrintf("[OpenHacks] Error: " __VA_ARGS__)

//! Log warning message
#define LOGW(...) OpenHacksLog::LogPrintf("[OpenHacks] Warning: " __VA_ARGS__)

//! Log debug message - only outputs when debug logging is enabled
#define LOGD(...) \
    do { if (OpenHacksLog::GDebugEnabled) { OpenHacksLog::LogPrintf("[OpenHacks] " __VA_ARGS__); } } while(0)
