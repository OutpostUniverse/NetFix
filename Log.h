#include "OPUNetTransportLayer.h"
#include <winsock2.h>
#include <fstream>

class ofstream;

// Global Debug file
extern std::ofstream logFile;


void Log(char* string);
void DumpIP(unsigned long ip);
void DumpAddr(sockaddr_in &addr);
void DumpPlayerNetID(int playerNetID);
void DumpAddrList(PeerInfo* peerInfo);
void DumpGuid(GUID &guid);
void DumpPacket(Packet& packet);
