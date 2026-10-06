#include "pch.h"
#include "advanced_settings.h"

namespace
{
// {E3A7D4F1-8B2C-4F6A-9D1E-5C3B7A8E2F40}
static const GUID cfg_guid_advanced_settings = {0xe3a7d4f1, 0x8b2c, 0x4f6a, {0x9d, 0x1e, 0x5c, 0x3b, 0x7a, 0x8e, 0x2f, 0x40}};

cfg_objList<AdvancedSettingValue> GAdvancedSettingValues(cfg_guid_advanced_settings);

// Static setting definitions
const std::vector<AdvancedSettingDef> kSettingDefs = {
    // General - Debug
    {AdvancedSettings::kDebugLoggingEnabled,
     "Debug Logging",
     "false",
     "Enable debug logging output to console. Debug messages will be shown with [OpenHacks] prefix.",
     SettingEditStyle::Combo,
     SettingCategory::General,
     {"false", "true"}},
    // Portability
    {AdvancedSettings::kPortabilityAutoLoadAssemblies,
     "Assemblies",
     "assemblies",
     "The folder path where COM component DLLs are placed. DLL files in this folder will be automatically loaded at startup. The DLL must include a manifest "
     "declaration to be usable by JS scripts. Supports both absolute and relative (to profile path) paths.",
     SettingEditStyle::Dir,
     SettingCategory::Portability,
     {}},
};

} // namespace

const std::vector<AdvancedSettingDef>& AdvancedSettings::GetSettingDefs()
{
    return kSettingDefs;
}

std::string AdvancedSettings::GetValue(const std::string& key)
{
    for (size_t i = 0; i < GAdvancedSettingValues.get_count(); ++i)
    {
        if (std::string(GAdvancedSettingValues[i].key.get_ptr()) == key)
        {
            return GAdvancedSettingValues[i].value.get_ptr();
        }
    }
    // Return default value from definition if no user value is set
    for (const auto& def : kSettingDefs)
    {
        if (def.key == key)
        {
            return def.defaultValue;
        }
    }
    return "";
}

void AdvancedSettings::SetValue(const std::string& key, const std::string& value)
{
    // Find existing entry
    for (size_t i = 0; i < GAdvancedSettingValues.get_count(); ++i)
    {
        if (std::string(GAdvancedSettingValues[i].key.get_ptr()) == key)
        {
            GAdvancedSettingValues[i].value = value.c_str();
            return;
        }
    }
    // Add new entry
    AdvancedSettingValue entry;
    entry.key = key.c_str();
    entry.value = value.c_str();
    GAdvancedSettingValues.add_item(entry);
}

void AdvancedSettings::ResetValue(const std::string& key)
{
    // Remove the user value entry so the default is used
    for (size_t i = 0; i < GAdvancedSettingValues.get_count(); ++i)
    {
        if (std::string(GAdvancedSettingValues[i].key.get_ptr()) == key)
        {
            pfc::bit_array_true mask;
            // Only remove this specific item
            pfc::bit_array_one at(i);
            GAdvancedSettingValues.remove_mask(at);
            return;
        }
    }
}

std::vector<AdvancedSettingItem> AdvancedSettings::GetAllItems()
{
    auto result = std::vector<AdvancedSettingItem>();
    result.reserve(kSettingDefs.size());

    for (const auto& def : kSettingDefs)
    {
        AdvancedSettingItem item;
        item.key = def.key;
        item.displayName = def.displayName;
        item.defaultValue = def.defaultValue;
        item.description = def.description;
        item.editStyle = def.editStyle;
        item.category = def.category;
        item.comboOptions = def.comboOptions;

        // Check if user value exists
        std::string userValue = GetValue(def.key);
        if (!userValue.empty() || !def.defaultValue.empty())
        {
            // User value exists if we found an entry
            bool hasUserValue = false;
            for (size_t i = 0; i < GAdvancedSettingValues.get_count(); ++i)
            {
                if (std::string(GAdvancedSettingValues[i].key.get_ptr()) == def.key)
                {
                    hasUserValue = true;
                    break;
                }
            }
            if (hasUserValue)
            {
                item.value = userValue;
            }
            else
            {
                item.value = def.defaultValue;
            }
        }
        else
        {
            item.value = def.defaultValue;
        }

        item.isModified = (item.value != def.defaultValue);
        result.push_back(std::move(item));
    }

    return result;
}

bool AdvancedSettings::GetBooleanValue(const std::string& key, bool defaultVal)
{
    auto val = GetValue(key);
    if (val.empty())
    {
        // Try default from definition
        for (const auto& def : kSettingDefs)
        {
            if (def.key == key)
            {
                return def.defaultValue == "true";
            }
        }
        return defaultVal;
    }
    return val == "true";
}

int AdvancedSettings::GetIntValue(const std::string& key, int defaultVal)
{
    auto val = GetValue(key);
    if (val.empty())
    {
        for (const auto& def : kSettingDefs)
        {
            if (def.key == key)
            {
                try
                {
                    return std::stoi(def.defaultValue);
                }
                catch (...)
                {
                    return defaultVal;
                }
            }
        }
        return defaultVal;
    }
    try
    {
        return std::stoi(val);
    }
    catch (...)
    {
        return defaultVal;
    }
}

double AdvancedSettings::GetDoubleValue(const std::string& key, double defaultVal)
{
    auto val = GetValue(key);
    if (val.empty())
    {
        for (const auto& def : kSettingDefs)
        {
            if (def.key == key)
            {
                try
                {
                    return std::stod(def.defaultValue);
                }
                catch (...)
                {
                    return defaultVal;
                }
            }
        }
        return defaultVal;
    }
    try
    {
        return std::stod(val);
    }
    catch (...)
    {
        return defaultVal;
    }
}
