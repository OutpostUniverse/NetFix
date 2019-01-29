#include <winsock2.h>
#include <iostream.h>
#include <fstream.h>
#include <objbase.h>
#include "OPUNetTransportLayer.h"


// Global Debug file
ofstream logFile("log.txt");


void Log(char* string)
{
	logFile << string << endl;
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
	int i;

	for (i = 0; i < MaxRemotePlayers; i++)
	{
		logFile << " " << i << ") {" << peerInfo[i].status << ", ";
		//DumpIP(peerInfo[i].address.sin_addr.s_addr);
		DumpAddr(peerInfo[i].address);
		logFile << ", ";
		DumpPlayerNetID(peerInfo[i].playerNetID);
		logFile << "}" << endl;
	}
}

void DumpGuid(GUID &guid)
{
	int i;

	logFile << hex << "{" << guid.Data1 << "-" << guid.Data2 << "-" << guid.Data3 << "-";
	for (i = 0; i < 8; i++)
	{
		logFile << (int)guid.Data4[i];
	}
	logFile << "}" << dec;
}

void DumpPacket(Packet* packet)
{
	logFile << " Source: " << packet->header.sourcePlayerNetID << endl;
	logFile << " Dest  : " << packet->header.destPlayerNetID << endl;
	logFile << " Size  : " << (unsigned int)packet->header.sizeOfPayload << endl;
	logFile << " type  : " << (unsigned int)packet->header.type << endl;
	logFile << " checksum : " << hex << packet->Checksum() << dec << endl;
	logFile << " commandType : " << packet->tlMessage.tlHeader.commandType << endl;
}