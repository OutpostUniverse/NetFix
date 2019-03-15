#include "OPUNetTransportLayer.h"
#include <iostream>
#include <fstream>
#include <objbase.h>


// Global Debug file
std::ofstream logFile("log.txt");


void Log(char* string)
{
	logFile << string << std::endl;
}

void DumpIP(unsigned long ip)
{
	logFile << (ip & 255) 
		<< "." << ((ip >> 8) & 255) 
		<< "." << ((ip >> 16) & 255) 
		<< "." << ((ip >> 24) & 255);
}

void DumpAddr(sockaddr_in &addr)
{
	logFile << "(AF:" << addr.sin_family << ") ";
	DumpIP(addr.sin_addr.s_addr);
	logFile << ":" << ntohs(addr.sin_port);
}

void DumpPlayerNetID(int playerNetID)
{
	logFile << "[" << (playerNetID & ~7) << "." << (playerNetID & 7) << "]";
}

void DumpAddrList(PeerInfo* peerInfo)
{
	for (int i = 0; i < MaxRemotePlayers; ++i)
	{
		logFile << " " << i << ") {" << peerInfo[i].status << ", ";
		//DumpIP(peerInfo[i].address.sin_addr.s_addr);
		DumpAddr(peerInfo[i].address);
		logFile << ", ";
		DumpPlayerNetID(peerInfo[i].playerNetID);
		logFile << "}" << std::endl;
	}
}

void DumpGuid(GUID &guid)
{
	logFile << std::hex << "{" << guid.Data1 << "-" << guid.Data2 << "-" << guid.Data3 << "-";
	for (int i = 0; i < 8; ++i)	{
		logFile << (int)guid.Data4[i];
	}
	logFile << "}" << std::dec;
}

void DumpPacket(Packet* packet)
{
	logFile << " Source: " << packet->header.sourcePlayerNetID << std::endl;
	logFile << " Dest  : " << packet->header.destPlayerNetID << std::endl;
	logFile << " Size  : " << static_cast<unsigned int>(packet->header.sizeOfPayload) << std::endl;
	logFile << " type  : " << static_cast<unsigned int>(packet->header.type) << std::endl;
	logFile << " checksum : " << std::hex << packet->Checksum() << std::dec << std::endl;
	logFile << " commandType : " << packet->tlMessage.tlHeader.commandType << std::endl;
}
