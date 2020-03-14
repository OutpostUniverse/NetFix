#include "GameProtocol.h"
#include "Log.h"
#include <OP2Internal.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstdint>

using namespace OP2Internal;

HINSTANCE hInstance;
OPUNetGameProtocol opuNetGameProtocol;
char sectionName[64] = "";				// Ini file section name, for loading additional parameters
const int DefaultProtocolIndex = 4;		// "SIGS"
const std::uintptr_t ExpectedOutpost2Addr = 0x00400000;


BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Don't need this, so don't waste the time
		DisableThreadLibraryCalls((HMODULE)hModule);

		hInstance = (HINSTANCE)hModule;

		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}


extern "C" __declspec(dllexport) void InitMod(char* iniSectionName)
{
	// Check the NetFixClient DLL load address
	if (hInstance != desiredLoadAddress)
	{
		LogError("NetFixClient DLL loaded to incorrect address " + FormatAddress(hInstance) + 
			". Expected address was " + FormatAddress(desiredLoadAddress));
		return;
	}
	// Check the Outpost2.exe load address
	void* op2ModuleBase = GetModuleHandle("Outpost2.exe");
	if (ExpectedOutpost2Addr != reinterpret_cast<std::uintptr_t>(op2ModuleBase))
	{
		LogError("Outpost2.exe module loaded at incorrect address " + FormatAddress(ExpectedOutpost2Addr) + 
			". Expected address was " + FormatAddress(op2ModuleBase));
		return;
	}

	// Store the .ini section name
	strncpy_s(sectionName, iniSectionName, sizeof(sectionName));

	// Get multiplayer button index that NetFix will replace
	int protocolIndex = config.GetInt(sectionName, "ProtocolIndex", DefaultProtocolIndex);
	LogDebug("ProtocolIndex set to " + std::to_string(protocolIndex));
	// Set a new multiplayer protocol type
	protocolList[protocolIndex].netGameProtocol = &opuNetGameProtocol;
}
