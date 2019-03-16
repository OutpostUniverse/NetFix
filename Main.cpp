#include "op2ext.h"
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

// Force Exports from Outpost2.exe
#include <OP2Internal.h>
using namespace OP2Internal;

#include "OPUNetGameProtocol.h"

#include <fstream>
#include <cstddef>
#include <string>
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

std::string GetOutpost2Directory();

extern "C" __declspec(dllexport) void InitMod(char* iniSectionName)
{
	// Sanity check the DLL load address
	if (hInstance != desiredLoadAddress)
	{
		MessageBox(nullptr, "DLL loaded to bad address", "NetFix Load Failed", 0);
		return;
	}
	// Sanity check the Outpost2.exe load address
	void* op2ModuleBase = GetModuleHandle("Outpost2.exe");
	if (ExpectedOutpost2Addr != (int)op2ModuleBase)
	{
		MessageBox(nullptr, "Outpost2.exe module not loaded at usual address", "NetFix Load Failed", 0);
		return;
	}



	// Store the .ini section name
	strncpy(sectionName, iniSectionName, sizeof(sectionName));


	// Get multiplayer button index that NetFix will replace
	const std::string iniPath = GetOutpost2Directory() + "Outpost2.ini"; 

	int protocolIndex = GetPrivateProfileInt(sectionName, "ProtocolIndex", DefaultProtocolIndex, iniPath.c_str());
	logFile << "[" << sectionName << "]" << " ProtocolIndex = " << protocolIndex << std::endl;
	// Set a new multiplayer protocol type
	protocolList[protocolIndex].netGameProtocol = &opuNetGameProtocol;
}

std::string GetOutpost2Directory()
{
	char buffer[MAX_PATH];
	std::size_t bufferSize = GetGameDir_s(buffer, MAX_PATH);

	if (bufferSize != 0) {
		throw std::runtime_error("Absolute directory to Outpost 2 must be less than " + MAX_PATH);
	}

	return std::string(buffer);
}
