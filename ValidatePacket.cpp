#include <OP2Internal.h>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

using namespace OP2Internal;


bool ValidatePacket(Packet& packet, sockaddr_in& from)
{
	// Validate source player net id against from address **TODO**

	// **TODO** Add validity checks here

	// All tests passed. Assume packet is good.
	return true;
}
