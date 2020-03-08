#include <string>

struct sockaddr_in;
struct _GUID;
typedef _GUID GUID;

struct PeerInfo;
namespace OP2Internal {
	struct Packet;
};


std::string FormatAddress(const sockaddr_in& address);
std::string FormatIP4Address(unsigned long ip);
std::string FormatPlayerList(const PeerInfo* peerInfo);
std::string FormatPlayerNetID(int playerNetID);
std::string FormatGuid(const GUID& guid);
std::string FormatPacket(const OP2Internal::Packet& packet);


void Log(const std::string& message);
void LogError(const std::string& message);
