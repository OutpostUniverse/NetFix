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

void LogAddress(sockaddr_in &addr);
void LogAddressList(PeerInfo* peerInfo);
void LogGuid(GUID &guid);
void LogPacket(OP2Internal::Packet& packet);
