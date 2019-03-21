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


std::string FormatAddress(sockaddr_in& address)
{
	std::stringstream ss;

	ss << "(AF:" << address.sin_family << ") ";
	ss << FormatIP4Address(address.sin_addr.s_addr);
	ss << ":" << ntohs(address.sin_port);

	return ss.str();
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

std::string FormatPlayerList(PeerInfo* peerInfo)
{
	std::stringstream ss;

	for (int i = 0; i < MaxRemotePlayers; ++i)
	{
		ss << " " << i << ") {" << peerInfo[i].status << ", ";
		ss << FormatAddress(peerInfo[i].address);
		ss << ", ";
		ss << FormatPlayerNetID(peerInfo[i].playerNetID);
		ss << "}" << std::endl;
	}

	return ss.str();
}

std::string FormatPlayerNetID(int playerNetID)
{
	std::stringstream ss;

	ss << "[" << (playerNetID & ~7) << "." << (playerNetID & 7) << "]";

	return ss.str();
}

std::string FormatGuid(GUID& guid)
{
	std::stringstream ss;

	ss << std::hex << "{" << guid.Data1 << "-" << guid.Data2 << "-" << guid.Data3 << "-";
	for (int i = 0; i < 8; ++i)	{
		ss << (int)guid.Data4[i];
	}
	ss << "}" << std::dec;

	return ss.str();
}


void Log(const char* string)
{
	logFile << string << std::endl;
}

void LogAddress(sockaddr_in &address)
{
	logFile << FormatAddress(address); // Note: No std::endl
}

void LogAddressList(PeerInfo* peerInfo)
{
	logFile << FormatPlayerList(peerInfo); // Note: std::endl already included
}

void LogGuid(GUID &guid)
{
	logFile << FormatGuid(guid); // Note: No std::endl;
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
