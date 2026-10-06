// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "hacks_version.h"
#include <libPPUI/Controls.h>
#include "hacks_com_impl.h"
#include "hacks_external_assemblies.h"

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID /*reserved*/)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(mod);
        CSeparator::Register();
        if (SUCCEEDED(OleInitialize(nullptr)))
            OpenHacksCOM::Initialise(mod);
        break;

    case DLL_PROCESS_DETACH:
        ExternalAssemblyLoader::UnloadAll();
        OpenHacksCOM::Finalise();
        OleUninitialize();

        break;
    default:
        break;
    }
    return TRUE;
}

namespace
{
DECLARE_COMPONENT_VERSION_COPY("OpenHacks", HACKS_VERSION, GetOpenHacksAbout())
// VALIDATE_COMPONENT_FILENAME("foo_openhacks.dll")
} // namespace