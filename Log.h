#include <fstream>
#include <string>

struct sockaddr_in;
struct _GUID;
typedef _GUID GUID;

struct PeerInfo;
namespace OP2Internal {
	struct Packet;
};


// Global Debug file
extern std::ofstream logFile;


void Log(const char* string);
// Provide error in modal dialog box for user and then log message
void LogWithModalDialog(const std::string& message);

void LogAddress(sockaddr_in &addr);
void LogAddressList(PeerInfo* peerInfo);
void LogGuid(GUID &guid);
void LogPacket(OP2Internal::Packet& packet);
