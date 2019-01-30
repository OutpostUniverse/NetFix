#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

// Force Exports from Outpost2.exe
#include <OP2Internal.h>
using namespace OP2Internal;

#include "OPUNetGameProtocol.h"

#include <fstream>
extern std::ofstream logFile;


HINSTANCE hInstance;
OPUNetGameProtocol opuNetGameProtocol;
char sectionName[64] = "";				// Ini file section name, for loading additional parameters
const int DefaultProtocolIndex = 4;		// "SIGS"
const int ExpectedOutpost2Addr = 0x00400000;


BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
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
	// Sanity check the DLL load address
	if (hInstance != desiredLoadAddress)
	{
		MessageBox(0, "DLL loaded to bad address", "NetFix Load Failed", 0);
		return;
	}
	// Sanity check the Outpost2.exe load address
	void* op2ModuleBase = GetModuleHandle("Outpost2.exe");
	if (ExpectedOutpost2Addr != (int)op2ModuleBase)
	{
		MessageBox(0, "Outpost2.exe module not loaded at usual address", "NetFix Load Failed", 0);
		return;
	}



	// Store the .ini section name
	strncpy(sectionName, iniSectionName, sizeof(sectionName));

	int protocolIndex;
	// Get button index to replace functionality of
	protocolIndex = GetPrivateProfileInt(sectionName, "ProtocolIndex", DefaultProtocolIndex, ".\\Outpost2.ini");
	logFile << "[" << sectionName << "]" << " ProtocolIndex = " << protocolIndex << std::endl;
	// Set a new multiplayer protocol type
	protocolList[protocolIndex].netGameProtocol = &opuNetGameProtocol;
}
