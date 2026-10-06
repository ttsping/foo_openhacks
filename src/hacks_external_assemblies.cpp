#include "pch.h"
#include "hacks_external_assemblies.h"
#include <regex>
#include <utility>
#include <map>
#include <mutex>
#include "hacks_log.h"
#include "advanced_settings.h"

namespace
{

struct LoadedAssembly
{
    std::wstring dllPath;
    HMODULE hModule = nullptr;
    DWORD cookie = 0;
    std::string clsid;
    std::string progid;
    ITypeLib* pTypeLib = nullptr;
};

std::vector<LoadedAssembly> GLoadedAssemblies;

static std::map<std::wstring, ITypeLib*> StaticTLibidToTypeLib;
static std::recursive_mutex StaticTypeLibMapMutex;

wstring_t ResolveAssemblyDirectory()
{
    auto settingValue = AdvancedSettings::GetValue(AdvancedSettings::kPortabilityAutoLoadAssemblies);
    if (settingValue.empty())
    {
        return {};
    }

    std::wstring settingValueW = Utility::ToUTF16(settingValue);
    if (::PathIsRelativeW(settingValueW.c_str()))
    {
        std::wstring profilePathW = Utility::GetProfilePath();
        if (profilePathW.empty())
            return {};

        profilePathW.resize(MAX_PATH);
        if (!::PathAppendW(profilePathW.data(), settingValueW.c_str()))
        {
            LOGE("Failed to append path: %s", settingValue.c_str());
            return {};
        }

        profilePathW.resize(wcslen(profilePathW.c_str()));
        profilePathW.shrink_to_fit();
        return profilePathW;
    }

    return settingValueW;
}

bool ParseManifest(const wchar_t* manifestPath, std::string& clsid, std::string& progid)
{
    auto buffer = Utility::ReadFileToBuffer(manifestPath);
    if (buffer.empty())
    {
        return false;
    }

    std::string content(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    try
    {
        // Parse CLSID: clsid="{...}"
        std::regex clsidRegex(R"(clsid="\{([A-Fa-f0-9-]+)\}")");
        std::smatch clsidMatch;
        if (std::regex_search(content, clsidMatch, clsidRegex))
        {
            clsid = "{" + clsidMatch[1].str() + "}";
        }
        else
        {
            return false;
        }

        // Parse ProgID: progid="..."
        std::regex progidRegex(R"regex(progid="([^"]+)")regex");
        std::smatch progidMatch;
        if (std::regex_search(content, progidMatch, progidRegex))
        {
            progid = progidMatch[1].str();
        }
        else
        {
            return false;
        }

        return true;
    }
    catch (const std::regex_error&)
    {
        LOGE("Regex error parsing manifest");
        return false;
    }
}

bool LoadSingleAssembly(const std::wstring& dllPath)
{
    auto dllPathStr = Utility::ToUTF8(dllPath);
    LOGD("Loading assembly: %s", dllPathStr.c_str());

    const wchar_t* fileName = ::PathFindFileNameW(dllPath.c_str());
    if (fileName == nullptr || fileName[0] == L'\0')
    {
        LOGW("Invalid DLL path: %s", dllPathStr.c_str());
        return false;
    }

    std::wstring baseDir = dllPath;
    size_t lastSlash = baseDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
    {
        baseDir = baseDir.substr(0, lastSlash);
    }

    std::wstring dllName = fileName;
    std::wstring manifestPath = baseDir;
    if (!manifestPath.empty() && manifestPath.back() != L'\\')
        manifestPath += L"\\";
    manifestPath += dllName + L".manifest";

    // Parse manifest
    std::string clsid;
    std::string progid;
    if (!ParseManifest(manifestPath.c_str(), clsid, progid))
    {
        LOGW("Assembly skipped (no valid manifest): %s", Utility::ToUTF8(dllName).c_str());
        return false;
    }

    LOGD("Manifest: CLSID=%s, ProgID=%s", clsid.c_str(), progid.c_str());

    HMODULE hModule = LoadLibraryW(dllPath.c_str());
    if (hModule == nullptr)
    {
        DWORD error = GetLastError();
        LOGE("Failed to load DLL: %s (error: %u)", dllPathStr.c_str(), error);
        return false;
    }

    using DllGetClassObject_t = HRESULT(WINAPI*)(REFCLSID, REFIID, void**);
    DllGetClassObject_t pfnDllGetClassObject = nullptr;
    if (!Utility::GetProcAddress(hModule, "DllGetClassObject", pfnDllGetClassObject))
    {
        LOGE("DLL does not export DllGetClassObject: %s", dllPathStr.c_str());
        FreeLibrary(hModule);
        return false;
    }

    // Convert CLSID string to GUID
    CLSID clsidGuid = {};
    auto clsidWide = Utility::ToUTF16(clsid);
    if (FAILED(CLSIDFromString(clsidWide.c_str(), &clsidGuid)))
    {
        LOGE("Invalid CLSID format: %s", clsid.c_str());
        FreeLibrary(hModule);
        return false;
    }

    // Get class factory
    ComPtr<IClassFactory> pFactory = nullptr;
    HRESULT hr = pfnDllGetClassObject(clsidGuid, IID_IClassFactory, (void**)&pFactory);
    if (FAILED(hr) || pFactory == nullptr)
    {
        LOGE("DllGetClassObject failed for %s (hr: 0x%08X)", dllPathStr.c_str(), hr);
        FreeLibrary(hModule);
        return false;
    }

    // Register class factory
    DWORD cookie = 0;
    hr = CoRegisterClassObject(clsidGuid, pFactory.Get(), CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    if (FAILED(hr) && hr != CO_E_OBJISREG)
    {
        LOGE("CoRegisterClassObject failed for %s (hr: 0x%08X)", dllPathStr.c_str(), hr);
        FreeLibrary(hModule);
        return false;
    }

    // Register ProgID mapping
    AddExternalCOMProgIDMapping(progid.c_str(), clsid.c_str());

    // Try to load type library (for hooking LoadRegTypeLib). Sources, in order:
    //  1. typelib embedded in the DLL itself (TYPELIB resource)
    //  2. sibling "<dll base name>.tlb" file next to the DLL.
    ITypeLib* pTypeLib = nullptr;
    HRESULT hrTypeLib = LoadTypeLibEx(dllPath.c_str(), REGKIND_NONE, &pTypeLib);
    if (FAILED(hrTypeLib) || pTypeLib == nullptr)
    {
        std::wstring tlbPath = dllPath;
        const size_t slashPos = tlbPath.find_last_of(L"\\/");
        const size_t dotPos = tlbPath.find_last_of(L'.');
        if (dotPos != std::wstring::npos && (slashPos == std::wstring::npos || dotPos > slashPos))
            tlbPath.erase(dotPos);
        tlbPath += L".tlb";
        hrTypeLib = LoadTypeLibEx(tlbPath.c_str(), REGKIND_NONE, &pTypeLib);
        if (SUCCEEDED(hrTypeLib) && pTypeLib != nullptr)
        {
            LOGD("Type library loaded from sibling file: %s", Utility::ToUTF8(tlbPath).c_str());
        }
    }
    if (SUCCEEDED(hrTypeLib) && pTypeLib != nullptr)
    {
        // Get TLIBID (Type Library GUID) from the loaded type library
        TLIBATTR* pLibAttr = nullptr;
        if (SUCCEEDED(pTypeLib->GetLibAttr(&pLibAttr)) && pLibAttr != nullptr)
        {
            // Convert TLIBID to string for mapping key
            LPOLESTR tlibidStr = nullptr;
            if (SUCCEEDED(StringFromCLSID(pLibAttr->guid, &tlibidStr)) && tlibidStr != nullptr)
            {
                // Add to TLIBID -> TypeLib mapping
                std::lock_guard<std::recursive_mutex> lock(StaticTypeLibMapMutex);
                StaticTLibidToTypeLib[tlibidStr] = pTypeLib;
                LOGD("Type library loaded for TLIBID %S", tlibidStr);
                CoTaskMemFree(tlibidStr);
            }
            pTypeLib->ReleaseTLibAttr(pLibAttr);
        }
        else
        {
            LOGW("Failed to get TLIBID from type library in %s", dllPathStr.c_str());
        }
    }
    else
    {
        LOGD("No typelib for %s (OK if component implements IDispatch directly)", dllPathStr.c_str());
    }

    // Store loaded assembly info
    LoadedAssembly assembly;
    assembly.dllPath = dllPath;
    assembly.hModule = hModule;
    assembly.cookie = cookie;
    assembly.clsid = clsid;
    assembly.progid = progid;
    assembly.pTypeLib = pTypeLib; // May be nullptr if no type library
    GLoadedAssemblies.push_back(std::move(assembly));

    LOG("Loaded: %s (ProgID: %s)", Utility::ToUTF8(dllName).c_str(), progid.c_str());
    return true;
}

} // anonymous namespace

ITypeLib* GetExternalTypeLib(const wchar_t* tlibidStr)
{
    std::lock_guard<std::recursive_mutex> lock(StaticTypeLibMapMutex);
    if (auto it = StaticTLibidToTypeLib.find(tlibidStr); it != StaticTLibidToTypeLib.end())
        return it->second;
    return nullptr;
}

void ExternalAssemblyLoader::LoadAll()
{
    std::wstring assemblyDir = ResolveAssemblyDirectory();
    if (assemblyDir.empty())
    {
        LOGD("Assembly directory not configured or invalid, skipping");
        return;
    }

    LOGD("Assembly directory: %s", Utility::ToUTF8(assemblyDir).c_str());

    if (!Utility::DirectoryExists(assemblyDir))
    {
        LOGD("Assembly directory does not exist, skipping");
        return;
    }

    auto dllFiles = Utility::EnumDirectoryFiles(assemblyDir, L"*.dll", false);

    for (const auto& filePath : dllFiles)
    {
        LoadSingleAssembly(filePath);
    }

    if (!GLoadedAssemblies.empty())
    {
        LOG("Loaded %zu external COM component(s)", GLoadedAssemblies.size());
    }
}

void ExternalAssemblyLoader::UnloadAll()
{
    for (auto& assembly : GLoadedAssemblies)
    {
        bool revoked = false;
        if (assembly.cookie != 0)
            revoked = SUCCEEDED(CoRevokeClassObject(assembly.cookie));

        if (assembly.pTypeLib != nullptr)
            assembly.pTypeLib->Release();

        // Only free the DLL if the class object was actually revoked.
        if (revoked && assembly.hModule != nullptr)
            FreeLibrary(assembly.hModule);

        LOGD("Unloaded: %s", Utility::ToUTF8(assembly.dllPath).c_str());
    }

    GLoadedAssemblies.clear();

    {
        std::lock_guard<std::recursive_mutex> lock(StaticTypeLibMapMutex);
        StaticTLibidToTypeLib.clear();
    }
}
