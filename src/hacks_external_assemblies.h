#pragma once

#include <string>
#include <vector>
#include <memory>

struct ITypeLib;

void AddExternalCOMProgIDMapping(const char* progId, const char* clsid);
ITypeLib* GetExternalTypeLib(const wchar_t* clsidStr);

namespace ExternalAssemblyLoader
{
    void LoadAll();
    void UnloadAll();
}
