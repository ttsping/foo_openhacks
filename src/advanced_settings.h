#pragma once

#include <string>
#include <vector>

enum class SettingEditStyle
{
    Text = 0,  // Free text editing
    Combo = 1, // Dropdown selection
    Dir = 2,   // Directory Path selection
};

enum class SettingCategory
{
    General = 0,
    Appearance = 1,
    Behavior = 2,
    Portability = 3,
};

struct AdvancedSettingDef
{
    std::string key;
    std::string displayName;
    std::string defaultValue;
    std::string description;
    SettingEditStyle editStyle = SettingEditStyle::Text;
    SettingCategory category = SettingCategory::General;
    std::vector<std::string> comboOptions;
};

struct AdvancedSettingValue
{
    pfc::string8 key;
    pfc::string8 value;
};

static inline stream_writer_formatter<>& operator<<(stream_writer_formatter<>& out, const AdvancedSettingValue& v)
{
    out << v.key << v.value;
    return out;
}

static inline stream_reader_formatter<>& operator>>(stream_reader_formatter<>& in, AdvancedSettingValue& v)
{
    in >> v.key >> v.value;
    return in;
}

struct AdvancedSettingItem
{
    std::string key;
    std::string displayName;
    std::string value;
    std::string defaultValue;
    std::string description;
    SettingEditStyle editStyle = SettingEditStyle::Text;
    SettingCategory category = SettingCategory::General;
    std::vector<std::string> comboOptions;
    bool isModified = false;
};

class AdvancedSettings
{
public:
    // Setting key constants
    static constexpr const char* kDebugLoggingEnabled = "debug.logging.enabled";
    static constexpr const char* kPortabilityAutoLoadAssemblies = "portability.auto.load.assemblies";

    // Get all static setting definitions
    static const std::vector<AdvancedSettingDef>& GetSettingDefs();

    static std::string GetValue(const std::string& key);
    static void SetValue(const std::string& key, const std::string& value);
    static void ResetValue(const std::string& key);
    // Get all items merged (defs + user values)
    static std::vector<AdvancedSettingItem> GetAllItems();

    static bool GetBooleanValue(const std::string& key, bool defaultValue = false);
    static int GetIntValue(const std::string& key, int defaultValue = 0);
    static double GetDoubleValue(const std::string& key, double defaultValue = 0.0);
};
