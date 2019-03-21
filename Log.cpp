#include "Log.h"
#include "OPUNetTransportLayer.h"
#include <winsock2.h>
#include <objbase.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>


// Global Debug file
std::ofstream logFile("log.txt");


void Log(const char* string)
{
	logFile << string << std::endl;
}

std::string FormatIP4Address(unsigned long ip)
{
	std::stringstream ss;

	ss << (ip & 255) << "." 
		<< ((ip >> 8) & 255) << "." 
		<< ((ip >> 16) & 255) << "." 
		<< ((ip >> 24) & 255);

	return ss.str();
}

void LogAddress(sockaddr_in &addr)
{
	logFile << "(AF:" << addr.sin_family << ") ";
	logFile << FormatIP4Address(addr.sin_addr.s_addr);
	logFile << ":" << ntohs(addr.sin_port);
}

std::string FormatPlayerNetID(int playerNetID)
{
	std::stringstream ss;

	ss << "[" << (playerNetID & ~7) << "." << (playerNetID & 7) << "]";

	return ss.str();
}

void LogAddressList(PeerInfo* peerInfo)
{
	for (int i = 0; i < MaxRemotePlayers; ++i)
	{
		logFile << " " << i << ") {" << peerInfo[i].status << ", ";
		//logFile << FormatIP4Address(peerInfo[i].address.sin_addr.s_addr);
		LogAddress(peerInfo[i].address);
		logFile << ", ";
		logFile << FormatPlayerNetID(peerInfo[i].playerNetID);
		logFile << "}" << std::endl;
	}
}

void LogGuid(GUID &guid)
{
	logFile << std::hex << "{" << guid.Data1 << "-" << guid.Data2 << "-" << guid.Data3 << "-";
	for (int i = 0; i < 8; ++i)	{
		logFile << (int)guid.Data4[i];
	}
	logFile << "}" << std::dec;
}

void LogPacket(OP2Internal::Packet& packet)
{
	logFile << " Source: " << packet.header.sourcePlayerNetID << std::endl;
	logFile << " Dest  : " << packet.header.destPlayerNetID << std::endl;
	logFile << " Size  : " << static_cast<unsigned int>(packet.header.sizeOfPayload) << std::endl;
	logFile << " type  : " << static_cast<unsigned int>(packet.header.type) << std::endl;
	logFile << " checksum : " << std::hex << packet.Checksum() << std::dec << std::endl;
	logFile << " commandType : " << packet.tlMessage.tlHeader.commandType << std::endl;
}
