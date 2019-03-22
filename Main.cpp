#include "Log.h"
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

// Provide error in modal dialog box for user and then log message
void LogWithModalDialog(const std::string& message);
std::string GetOutpost2Directory();


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


	// Get multiplayer button index that NetFix will replace
	const std::string iniPath = GetOutpost2Directory() + "Outpost2.ini"; 

	int protocolIndex = GetPrivateProfileInt(sectionName, "ProtocolIndex", DefaultProtocolIndex, iniPath.c_str());
	logFile << "[" << sectionName << "]" << " ProtocolIndex = " << protocolIndex << std::endl;
	// Set a new multiplayer protocol type
	protocolList[protocolIndex].netGameProtocol = &opuNetGameProtocol;
}


void LogWithModalDialog(const std::string& message)
{
	Log(message.c_str());
	MessageBox(nullptr, message.c_str(), "NetFixClient Error", 0);
}

std::string GetOutpost2Directory()
{
	std::string buffer;

	// Pass size of 0 to get exact buffer size
	std::size_t bufferSize = GetGameDir_s(&buffer[0], 0);

	buffer.resize(bufferSize);
	GetGameDir_s(&buffer[0], bufferSize);

	// String concatinations (string1 + string2) will not provide expected results
	// if the std::string explicity has a null terminator
	buffer.erase(std::find(buffer.begin(), buffer.end(), '\0'), buffer.end());

	return buffer;
}
