#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

// Force Exports from Outpost2.exe
#include "..\API\ForcedExports\ForcedExports.h"
using namespace OP2ForcedExport;

#include "OPUNetGameProtocol.h"


HINSTANCE hInstance;
OPUNetGameProtocol opuNetGameProtocol;
char sectionName[64] = "";				// Ini file section name, for loading additional parameters
const DefaultProtocolIndex = 3;			// "Serial"


BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Sanity check on the DLL load address
		if (hModule != desiredLoadAddress)
		{
			MessageBox(0, "DLL loaded to bad address", "Failed", 0);
			return FALSE;
		}
		// Don't need this, so don't waste the time
		DisableThreadLibraryCalls((HMODULE)hModule);
		// Store the module handle
		hInstance = (HINSTANCE)hModule;

		break;
	case DLL_PROCESS_DETACH:
		break;
	}

    return TRUE;
}

extern "C" __declspec(dllexport) void InitMod(char* iniSectionName)
{
	// Store the .ini section name
	strncpy(sectionName, iniSectionName, sizeof(sectionName));

	int protocolIndex;
	protocolIndex = config.GetInt(sectionName, "ProtocolIndex", DefaultProtocolIndex);
	// Set a new multiplayer protocol type
	protocolList[protocolIndex].netGameProtocol = &opuNetGameProtocol;
}
