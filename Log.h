#include <fstream>

struct sockaddr_in;
struct _GUID;
typedef _GUID GUID;

struct PeerInfo;
namespace OP2Internal {
	struct Packet;
};


// Global Debug file
extern std::ofstream logFile;


std::string FormatIP4Address(unsigned long ip);
std::string FormatPlayerNetID(int playerNetID);


void Log(const char* string);

void LogAddress(const sockaddr_in &addr);
void LogAddressList(const PeerInfo* peerInfo);
void LogGuid(const GUID &guid);
void LogPacket(const OP2Internal::Packet& packet);
