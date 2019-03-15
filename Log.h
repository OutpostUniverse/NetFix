#include <fstream>

struct sockaddr_in;
struct _GUID;
typedef _GUID GUID;

struct PeerInfo;
namespace OP2Internal {
	class Packet;
};


// Global Debug file
extern std::ofstream logFile;


void Log(char* string);
void DumpIP(unsigned long ip);
void DumpAddr(sockaddr_in &addr);
void DumpPlayerNetID(int playerNetID);
void DumpAddrList(PeerInfo* peerInfo);
void DumpGuid(GUID &guid);
void DumpPacket(OP2Internal::Packet& packet);
