#include "Log.h"
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

// Force Exports from Outpost2.exe
#include <OP2Internal.h>
using namespace OP2Internal;

#include "OPUNetGameProtocol.h"
#include <string>
#include <fstream>
extern std::ofstream logFile;


HINSTANCE hInstance;
OPUNetGameProtocol opuNetGameProtocol;
char sectionName[64] = "";				// Ini file section name, for loading additional parameters
const int DefaultProtocolIndex = 4;		// "SIGS"
const int ExpectedOutpost2Addr = 0x00400000;

// Provide error in modal dialog box for user and then log message
void LogWithModalDialog(const std::string& message);

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
	// Check the NetFixClient DLL load address
	if (hInstance != desiredLoadAddress)
	{
		LogWithModalDialog("NetFixClient DLL loaded to incorrect address");
		return;
	}
	// Check the Outpost2.exe load address
	void* op2ModuleBase = GetModuleHandle("Outpost2.exe");
	if (ExpectedOutpost2Addr != (int)op2ModuleBase)
	{
		LogWithModalDialog("Outpost2.exe module not loaded at usual address");
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

void LogWithModalDialog(const std::string& message)
{
	MessageBox(nullptr, message.c_str(), "NetFixClient Error", 0);
	Log(message.c_str());
}
