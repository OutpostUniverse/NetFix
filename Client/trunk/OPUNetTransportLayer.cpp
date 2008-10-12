#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include "OPUNetTransportLayer.h"


extern char sectionName[];


// ** Begin Debug Code
#include <iostream.h>
#include <fstream.h>
#include <objbase.h>


// Global Debug file
ofstream logFile("log.txt");


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
// ** End Debug Code



// Public member functions
// -----------------------

// Returns NULL on failure
OPUNetTransportLayer* OPUNetTransportLayer::Create()		// Public static constructor
{
	// Create a new object
	OPUNetTransportLayer* opuNetTransportLayer = new OPUNetTransportLayer();

	// Make sure it initializes properly
	if (!opuNetTransportLayer->InitializeWinsock())
	{
		// Error
		delete opuNetTransportLayer;
		return 0;
	}

	// Create a socket for join/host
	if (!opuNetTransportLayer->CreateSocket())
	{
		// Error
		delete opuNetTransportLayer;
		return 0;
	}

	// Return the newly constructed object
	return opuNetTransportLayer;
}

// Returns true on success, false on failure
// Will recreate the socket if it exists
// Allows the socket to be unbound after cancelling hosting
bool OPUNetTransportLayer::CreateSocket()
{
	// Check if a socket has already been created
	if (netSocket != INVALID_SOCKET)
	{
		// Close the old socket  (allows a bound socket to become unbound when it's recreated below
		closesocket(netSocket);
	}

	// Create the socket
	netSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// Enable broadcasts
	BOOL newValue = true;
	setsockopt(netSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&newValue, sizeof(newValue));

	// Check if the port needs to be bound (forced)
	forcedPort = config.GetInt(sectionName, "ForcedPort", 0);
	if (forcedPort != 0)
	{
		int retVal;
		sockaddr_in sockAddr;

		// Set the bind address
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = htons(forcedPort);
		sockAddr.sin_addr.s_addr = INADDR_ANY;
		// Try to bind to the forced port
		retVal = bind(netSocket, (sockaddr*)&sockAddr, sizeof(sockAddr));

		// Check for success
		if (retVal == 0)
		{
			logFile << "ForcedPort = " << forcedPort << endl;
		}
		else
		{
			logFile << "Warning: Could not bind to ForcedPort = " << forcedPort << endl;
			forcedPort = 0;		// Clear this so we don't try to use it when joining games
		}
	}

	// Return status
	return (netSocket != INVALID_SOCKET);
}

bool OPUNetTransportLayer::HostGame(USHORT port, const char* password, const char* creatorName, int maxPlayers, int gameType)
{
	int i;
	int errorCode;
	sockaddr_in localAddress;

// **DEBUG**
logFile.close();
logFile.open("logHost.txt");

	// Clear internal players state
	numPlayers = 0;
	for (i = 0; i < MaxRemotePlayers; i++)
	{
		peerInfo[i].status = 0;
		peerInfo[i].playerNetID = 0;
		peerInfo[i].address.sin_addr.s_addr = INADDR_ANY;
	}


	// Check if we want to bind to the host port
	if (port != 0)
	{
		// Bind to the server listen port
		// ------------------------------

		// Set the bind address
		localAddress.sin_family = AF_INET;
		localAddress.sin_port = htons(port);
		localAddress.sin_addr.s_addr = INADDR_ANY;

		// Create the server listening socket
		hostSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		// Check for errors
		if (hostSocket == INVALID_SOCKET)
		{
			// Failed to create the socket
			return false;
		}
		// Bind the socket to listen on
		errorCode = bind(hostSocket, (sockaddr*)&localAddress, sizeof(localAddress));
		// Check for errors
		if (errorCode == SOCKET_ERROR)
		{
			// Could not bind to the socket. Get client socket port info
			i = sizeof(localAddress);
			errorCode = getsockname(netSocket, (sockaddr*)&localAddress, &i);
			if (errorCode == SOCKET_ERROR)
				return false;		// Failed
			// Check if client socket port doesn't match
			if (ntohs(localAddress.sin_port) != port)
				return false;		// Failed

			// Ports match, use the client socket
			hostSocket = netSocket;
		}

// **DEBUG**
logFile << "Bound to server port: " << port << endl;
	}


	// Set the game hosting info
	// -------------------------
	// Create a unique session identifier
	CoCreateGuid(&hostedGameInfo.sessionIdentifier);	// Don't really care if this fails, not a big deal
	//hostedGameInfo.createGameInfo.startupFlags.Init(maxPlayers, gameType);
	// Set the Startup flags
	hostedGameInfo.createGameInfo.startupFlags.bDisastersOn = true;
	hostedGameInfo.createGameInfo.startupFlags.bDayNightOn = true;
	hostedGameInfo.createGameInfo.startupFlags.bMoraleOn = true;
	hostedGameInfo.createGameInfo.startupFlags.bCampaign = false;
	hostedGameInfo.createGameInfo.startupFlags.bMultiplayer = true;
	hostedGameInfo.createGameInfo.startupFlags.bCheatsOn = false;
	hostedGameInfo.createGameInfo.startupFlags.maxPlayers = maxPlayers;
	hostedGameInfo.createGameInfo.startupFlags.b1 = 0;
	hostedGameInfo.createGameInfo.startupFlags.missionType = gameType;
	hostedGameInfo.createGameInfo.startupFlags.numInitialVehicles = 0;
	// Copy the game creator name and password
	strncpy(hostedGameInfo.createGameInfo.gameCreatorName, creatorName, sizeof(hostedGameInfo.createGameInfo.gameCreatorName));
	strncpy(this->password, password, sizeof(this->password));

// **DEBUG**
logFile << " Session ID: ";
DumpGuid(hostedGameInfo.sessionIdentifier);
logFile << endl;

	// Create a Host playerNetID
	playerNetID = timeGetTime() & ~7;
	// Set the host fields
	peerInfo[0].playerNetID = playerNetID;
	peerInfo[0].address = localAddress;			// Clear the local address
	peerInfo[0].status = 2;
	// Update number of players
	numPlayers = 1;

	// Enable game host query replies
	bInvite = true;


	// Poke the game server (and let it know a new game is hosted)
	errorCode = PokeGameServer(pscGameHosted);
	// Check for errors
	if (errorCode == 0)
	{
// **DEBUG**
logFile << "Error informing game server" << endl;
	}


	// Return success status
	return true;				// Success
}

bool OPUNetTransportLayer::SearchForGames(char* hostAddressString, unsigned short defaultHostPort)
{
	Packet packet;
	sockaddr_in hostAddress;
	int errorCode;

	// Create the default host address
	hostAddress.sin_family = AF_INET;
	hostAddress.sin_port = htons(defaultHostPort);
	hostAddress.sin_addr.S_un.S_addr = INADDR_BROADCAST;

	// Try to convert the string fields
	errorCode = GetHostAddress(hostAddressString, hostAddress);
	if (errorCode == 0)
		return false;

	// Construct the SearchQuery packet
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(HostedGameSearchQuery);
	packet.header.type = 1;
	packet.tlMessage.searchQuery.commandType = tlcHostedGameSearchQuery;
	packet.tlMessage.searchQuery.gameIdentifier = gameIdentifier;
	packet.tlMessage.searchQuery.timeStamp = timeGetTime();
	packet.tlMessage.searchQuery.password[0] = 0;

// **DEBUG**
logFile << "Search for games: ";
DumpAddr(hostAddress);
logFile << endl;

	// Send the HostGameSearchQuery
	return SendTo(packet, hostAddress);
}

bool OPUNetTransportLayer::JoinGame(HostedGameInfo &game, const char* password)
{
	int i;
	Packet packet;


	// Clear internal players state
	numPlayers = 0;
	for (i = 0; i < MaxRemotePlayers; i++)
	{
		peerInfo[i].status = 0;
		peerInfo[i].playerNetID = 0;
		peerInfo[i].address.sin_addr.s_addr = INADDR_ANY;
	}


	// Store a pointer to the game we're trying to join
	joiningGameInfo = &game;

	// Construct the JoinRequest packet
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(JoinRequest);
	packet.header.type = 1;
	packet.tlMessage.joinRequest.commandType = tlcJoinRequest;
	packet.tlMessage.joinRequest.sessionIdentifier = game.sessionIdentifier;
	packet.tlMessage.joinRequest.returnPortNum = forcedPort;
	strncpy(packet.tlMessage.joinRequest.password, password, sizeof(packet.tlMessage.joinRequest.password));

// **DEBUG**
logFile << "Sending join request: ";
DumpAddr(game.address);
logFile << endl << "  Session ID: ";
DumpGuid(packet.tlMessage.joinRequest.sessionIdentifier);
logFile << endl;
//DumpPacket(&packet);

	sockaddr_in gameServerAddr;
	// Check if a Game Server is set
	if (GetGameServerAddress(gameServerAddr))
	{
		// Send a Join message through the server too
		SendTo(packet, gameServerAddr);
	}

	// Send the JoinRequest
	return SendTo(packet, game.address);
}


void OPUNetTransportLayer::OnJoinAccepted(Packet &packet)
{
	int localPlayerNum;
	bool bSuccess;

	// Make sure a join was requested
	if (joiningGameInfo == 0)
		return;			// Abort

	// Join successful
	// ---------------
	// Store the Host info
	peerInfo[0].playerNetID = packet.header.sourcePlayerNetID;	// Store Host playerNetID
	peerInfo[0].address = joiningGameInfo->address;				// Store Host address
	peerInfo[0].status = 2;										// **
	// Get the assigned playerNetID
	playerNetID = packet.tlMessage.joinReply.newPlayerNetID;	// Store playerNetID
	localPlayerNum = playerNetID & 7;							// Cache (frequently used)
	// Update local info
	peerInfo[localPlayerNum].playerNetID = playerNetID;
	peerInfo[localPlayerNum].address.sin_addr.s_addr = INADDR_ANY;	// Clear the address
	peerInfo[localPlayerNum].status = 2;


	// Update num players (for quit messages from cancelled games)
	numPlayers = 1;


	// Send updated status to host
	bSuccess = SendStatusUpdate();
	// Check for errors replying
	if (bSuccess == false)
	{
		// Error. Inform User
		MsgBox(0, "Error sending updated status to host", "Error", 0);
	}


	// Reset network traffic counters
	ResetTrafficCounters();
}


int OPUNetTransportLayer::GetNumPlayers()
{
	return numPlayers;
}



// ***********************************************************************************
// ***********************************************************************************



// Virtual member functions
// ------------------------

OPUNetTransportLayer::~OPUNetTransportLayer()
{
	// Check if a game was cancelled
	if (bInvite == true)
	{
		// Inform the game server
		PokeGameServer(pscGameCancelled);
	}

	// Make sure we don't Cleanup if we haven't done Startup
	if (bInitialized)
	{
		// Release the sockets
		if (netSocket != INVALID_SOCKET)
			closesocket(netSocket);
		if (hostSocket != INVALID_SOCKET)
			closesocket(hostSocket);

		// Shutdown Winsock
		WSACleanup();
	}
}

int OPUNetTransportLayer::GetHostPlayerNetID()
{
	return peerInfo[0].playerNetID;
}

// Called when the game is starting (but not when cancelled)
void OPUNetTransportLayer::ShutDownInvite()
{
	// Disable game host query replies
	bInvite = false;
	bGameStarted = true;

	// Let the game server know the game is starting
	PokeGameServer(pscGameStarted);
}

int OPUNetTransportLayer::ReplicatePlayersList()
{
	int i;
	int retVal;
	Packet packet;

	// Fill in the packet header
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(PlayersList);
	packet.header.type = 1;
	// Fill in the packet body
	packet.tlMessage.playersList.commandType = tlcSetPlayersList;
	packet.tlMessage.playersList.numPlayers = numPlayers;
	// Copy the players list
	for (i = 0; i < MaxRemotePlayers; i++)
	{
		packet.tlMessage.playersList.netPeerInfo[i].ip = peerInfo[i].address.sin_addr.s_addr;
		packet.tlMessage.playersList.netPeerInfo[i].port = peerInfo[i].address.sin_port;
		packet.tlMessage.playersList.netPeerInfo[i].status = peerInfo[i].status;
		packet.tlMessage.playersList.netPeerInfo[i].playerNetID = peerInfo[i].playerNetID;
	};


	// Send the Player List
	retVal = SendUntilStatusUpdate(&packet, 3, 16, 500);


	// Check for errors
	if (retVal == 0)
	{
		// Failed. Inform other players of failure.
		packet.header.sourcePlayerNetID = 0;
		packet.header.destPlayerNetID = 0;
		packet.header.sizeOfPayload = 4;
		packet.header.type = 1;
		packet.tlMessage.tlHeader.commandType = tlcSetPlayersListFailed;

		// Wait until all clients acknowledge failure
		SendUntilStatusUpdate(&packet, 4, 16, 500);
		return -1;		// Failed
	}


	// Success
	peerInfo[0].status = 3;			// **
	return 1;
}

// Fills the netIDList with opponent (non-local) playerNetIDs
// Returns the number of entries copied into the supplied array
// or -1 if the output buffer is too small, (after having been filled)
int OPUNetTransportLayer::GetOpponentNetIDList(int netIDList[], int maxNumID)
{
	int i;
	int j;

	// Copy all non local playerNetIDs
	for (i = 0, j = 0; i < MaxRemotePlayers; i++)
	{
		// Make sure the ID is valid
		if (peerInfo[i].playerNetID != 0)
		{
			// Make sure it doesn't match the local player
			if (peerInfo[i].playerNetID != playerNetID)
			{
				// Abort if the output buffer is full
				if (j >= maxNumID)
					return -1;		// Error. Buffer too small

				// Copy it to the dest buffer
				netIDList[j] = peerInfo[i].playerNetID;
				j++;
			}
		}
	}

	return j;		// Success
}

void OPUNetTransportLayer::RemovePlayer(int playerNetID)
{
	unsigned int playerIndex;

	// Detemine which player to remove
	playerIndex = playerNetID & 7;

	// Make sure the player exists
	if (peerInfo[playerIndex].status != 0)
	{
		// Remove the player
		peerInfo[playerIndex].playerNetID = 0;
		peerInfo[playerIndex].status = 0;
		peerInfo[playerIndex].address.sin_addr.s_addr = INADDR_ANY;		// Clear the address
		// Update player count
		numPlayers--;
	}
}

int OPUNetTransportLayer::Send(Packet* packet)
{
	int errorCode;
	int packetSize;
	sockaddr_in* address;

	// Calculate the packet size
	packetSize = packet->header.sizeOfPayload + sizeof(packet->header);
	// Calculate checksum
	packet->header.checksum = packet->Checksum();

	// Check if this packet should be broadcast
	if (packet->header.destPlayerNetID == 0)
	{
		// Broadcast
		unsigned int i;

		// Send packet to all players
		for (i = 0; i < MaxRemotePlayers; i++)
		{
			// Make sure the player record is valid
			if (peerInfo[i].status != 0)
			{
				// Don't send to self
				if (peerInfo[i].playerNetID != playerNetID)
				{
					// Send the packet to current player
					address = &peerInfo[i].address;
					errorCode = sendto(netSocket, (char*)packet, packetSize, 0, (sockaddr*)address, sizeof(*address));

					// Check for success
					if (errorCode != SOCKET_ERROR)
					{
						// Success. Increment the traffic counters
						trafficCounters.numPacketsSent++;
						trafficCounters.numBytesSent += packetSize;
					}
				}
			}
		}
	}
	else
	{
		// Singlecast
		int i;

		// Get the PeerInfo index
		i = (packet->header.destPlayerNetID & 7);
		// Make sure the player record is valid
		if (peerInfo[i].status != 0)
		{
			// Don't send to self
			if (peerInfo[i].playerNetID != playerNetID)
			{
				address = &peerInfo[i].address;
				errorCode = sendto(netSocket, (char*)packet, packetSize, 0, (sockaddr*)address, sizeof(*address));

				// Check for success
				if (errorCode != SOCKET_ERROR)
				{
					// Success. Increment the traffic counters
					trafficCounters.numPacketsSent++;
					trafficCounters.numBytesSent += packetSize;
				}
			}
		}
	}

	return true;		// Success
}

int OPUNetTransportLayer::Receive(Packet* packet)
{
	int i;
	unsigned long numBytes;
	sockaddr_in from;
	bool bRetVal;

	for (;;)
	{
		// Check if we need to return a JoinReturned packet
		if (numJoining != 0)
		{
			// Check each player for joining
			for (i = 0; i < MaxRemotePlayers; i++)
			{
				// Check if this player is joining
				if (peerInfo[i].bReturnJoinPacket)
				{
					// Construct the JoinGranted packet
					packet->header.sourcePlayerNetID = 0;
					packet->header.destPlayerNetID = peerInfo[0].playerNetID;
					packet->header.sizeOfPayload = sizeof(JoinReturned);
					packet->header.type = 1;
					packet->tlMessage.tlHeader.commandType = tlcJoinGranted;
					packet->tlMessage.joinReturned.newPlayerNetID = peerInfo[i].playerNetID;

					// Mark as returned
					peerInfo[i].bReturnJoinPacket = false;
					numJoining--;
					return true;		// Return packet for processing
				}
				else
				{
					// Check if partially joined
					if (peerInfo[i].status == 1)
					{
						// Check if timed out
						if ((timeGetTime() - (peerInfo[i].playerNetID & ~7)) > JoinTimeOut)
						{
							// Cancel the join, and reclaim the player record
							numPlayers--;
							numJoining--;
							peerInfo[i].bReturnJoinPacket = false;
							peerInfo[i].status = 0;
							peerInfo[i].address.sin_addr.s_addr = INADDR_ANY;
							peerInfo[i].playerNetID = 0;
						}
					}
				}
			}
		}


		// Try to read from the net socket
		numBytes = ReadSocket(netSocket, *packet, from);
		// Check for errors
		if (numBytes == -1)
		{
			// Try to read from the host socket
			numBytes = ReadSocket(hostSocket, *packet, from);
			// Check for errors
			if (numBytes == -1)
				return false;		// Abort
		}


// **DEBUG**
//logFile << "ReadSocket: " << packet->tlMessage.tlHeader.commandType << endl;

		// Error check the packet
		if (numBytes < sizeof(PacketHeader))
			continue;		// Discard packet
		if (numBytes < sizeof(PacketHeader) + packet->header.sizeOfPayload)
			continue;		// Discard packet
		if (packet->header.checksum != packet->Checksum())
			continue;		// Discard packet

		// Count the received packet
		trafficCounters.numPacketsReceived++;
		trafficCounters.numBytesReceived += packet->header.sizeOfPayload + sizeof(packet->header);

		// Determine if immediate processing is required
		bRetVal = DoImmediateProcessing(*packet, from);


		// Check destination
		if ((packet->header.destPlayerNetID != 0) && (packet->header.destPlayerNetID != playerNetID))
			continue;		// Discard packet

		// Check if the packet was unhandled
		if (bRetVal == false)
		{
			// Non immediate processed packet received. Return packet
			return true;
		}
	}
}

int OPUNetTransportLayer::IsHost()				// IsCurrentGameHost?
{
	return (bInvite && (playerNetID == peerInfo[0].playerNetID));
}

int OPUNetTransportLayer::IsValidPlayer()		// IsHostWaitingToStart?
{
	return bInvite;
}

int OPUNetTransportLayer::F1()
{
	return 1;
}

int OPUNetTransportLayer::GetAddressString(int playerNetID, char* addressString, int bufferSize)
{
	int playerIndex;
	sockaddr_in* address;

	// Determine the playerIndex
	playerIndex = (playerNetID & 7);

	// Get the address and convert it to a string
	address = &peerInfo[playerIndex].address;
	scr_snprintf(addressString, bufferSize, "%i.%i.%i.%i", address->sin_addr.S_un.S_un_b.s_b1, address->sin_addr.S_un.S_un_b.s_b2, address->sin_addr.S_un.S_un_b.s_b3, address->sin_addr.S_un.S_un_b.s_b4);

	return true;		// Success
}

int OPUNetTransportLayer::ResetTrafficCounters()
{
	// Clear the TrafficCounters
	memset(&trafficCounters, 0, sizeof(trafficCounters));
	trafficCounters.timeOfLastReset = timeGetTime();

	return true;		// Success
}

int OPUNetTransportLayer::GetTrafficCounts(TrafficCounters* trafficCounters)
{
	// Copy the TrafficCounter to the supplied buffer
	*trafficCounters = this->trafficCounters;

	return true;		// Success
}



// ***********************************************************************************
// ***********************************************************************************


// Private member functions
// ------------------------


// -------------------------------------------

OPUNetTransportLayer::OPUNetTransportLayer()	// Private Constructor  [Prevent object creation]
{
	playerNetID = 0;
	numPlayers = 0;
	bInitialized = false;
	netSocket = INVALID_SOCKET;
	hostSocket = INVALID_SOCKET;
	forcedPort = 0;
	memset(&peerInfo, 0, sizeof(peerInfo));
	bInvite = false;
	bGameStarted = false;
	memset(&hostedGameInfo, 0, sizeof(hostedGameInfo));
	hostedGameInfo.ping = -1;
	ResetTrafficCounters();
	joiningGameInfo = 0;
	numJoining = 0;
	randValue = timeGetTime() ^ RandValueXor;
}


// -------------------------------------------

bool OPUNetTransportLayer::InitializeWinsock()
{
	WSADATA wsaData;
	int err;
	WORD version = MAKEWORD(2, 2);

	if (!bInitialized)
	{
		// Initialize Winsock
		err = WSAStartup(version, &wsaData);

		// Check for success
		if (err == 0)
		{
			// Check if we got the right version
			if (wsaData.wVersion == version)
			{
				// Success
				bInitialized = true;
			}
			else
			{
				// Wrong version
				WSACleanup();
			}
		}
	}

	return bInitialized;
}


// -------------------------------------------

// Returns: -1 = Success, 0 = Failed (bad addr), 1 = Failed (no addr specified)
int OPUNetTransportLayer::GetHostAddress(char* hostAddressString, sockaddr_in &hostAddress)
{
	char* portNumString;
	int port;

	// Check if a specific host address was indicated
	if (hostAddressString == 0)
		return 1;				// Failed (no address specified)

	// Convert hostAddress string to network address
	// ---------------------------------------------

	// Skip any leading spaces
	while (hostAddressString[0] == ' ' || hostAddressString[0] == '\t')
		hostAddressString++;

	// Check if a port number was specified
	portNumString = strchr(hostAddressString, ':');
	if (portNumString != 0)
	{
		// Convert the port number
		port = atoi(&portNumString[1]);
		// Make sure a number was actually specified  (otherwise use the default port)
		if (port != 0)
			hostAddress.sin_port = htons(port);
		// Truncate the address string, so lookup succeeds
		portNumString[0] = 0;
	}

	// Check if the address part is empty  (in which case use the default)
	if (hostAddressString[0] == 0)
	{
		if (portNumString != 0)
			portNumString[0] = ':';		// Restore string
		return 1;					// Failed (no address specified)
	}

	// First try a numeric conversion
	hostAddress.sin_addr.S_un.S_addr = inet_addr(hostAddressString);
	// Check for failure
	if (hostAddress.sin_addr.S_un.S_addr == INADDR_NONE)
	{
		HOSTENT* hostEnt;
		// Try looking up the address
		hostEnt = gethostbyname(hostAddressString);
		// Check for failure
		if (hostEnt == 0)
		{
			if (portNumString != 0)
				portNumString[0] = ':';		// Restore string
			return 0;				// Failed (invalid address)
		}
		// Get the host IP address
		hostAddress.sin_addr.S_un.S_addr = *(unsigned long*)hostEnt->h_addr_list[0];
	}

	if (portNumString != 0)
		portNumString[0] = ':';		// Restore string
	return -1;						// Success
}


// -------------------------------------------

// Returns a new playerNetID
int OPUNetTransportLayer::AddPlayer(sockaddr_in& from)
{
	int newPlayerIndex;

	// Make sure there is room for a new player
	if (numPlayers >= hostedGameInfo.createGameInfo.startupFlags.maxPlayers)
		return 0;		// Failed

	// Find an empty slot
	for (newPlayerIndex = 0; newPlayerIndex < MaxRemotePlayers; newPlayerIndex++)
	{
		// Check if this slot is empty
		if (peerInfo[newPlayerIndex].status == 0)
		{
			// Insert player into empty slot
			// -----------------------------
			// Store the source address
			peerInfo[newPlayerIndex].address = from;
			peerInfo[newPlayerIndex].status = 1;			// **
			peerInfo[newPlayerIndex].playerNetID = newPlayerIndex | (timeGetTime() & ~7);
			// Increase connected player count
			numPlayers++;
			numJoining++;

			// Return the new playerNetID
			return peerInfo[newPlayerIndex].playerNetID;	// Success
		}
	}

	// Failed
	return 0;
}


// -------------------------------------------

int OPUNetTransportLayer::ReadSocket(SOCKET sourceSocket, Packet& packet, sockaddr_in& from)
{
	int errorCode;
	unsigned long numBytes;
	int fromLen = sizeof(from);

	// Check if the host socket is in use
	if (sourceSocket == INVALID_SOCKET)
		return -1;		// Abort

	// Check the server port for data
	errorCode = ioctlsocket(sourceSocket, FIONREAD, &numBytes);
	// Check for success
	if (errorCode == SOCKET_ERROR)
		return -1;		// Abort
	if (numBytes == 0)
		return -1;		// Abort

	// Read the data
	numBytes = recvfrom(sourceSocket, (char*)&packet, sizeof(packet), 0, (sockaddr*)&from, &fromLen);

	// Check for errors
	if (numBytes == SOCKET_ERROR)
		return -1;		// Abort

	// Return number of bytes read
	return numBytes;
}


// -------------------------------------------

bool OPUNetTransportLayer::SendTo(Packet& packet, sockaddr_in& to)
{
	int packetSize;
	int errorCode;


	// Calculate Packet size
	packetSize = packet.header.sizeOfPayload + sizeof(packet.header);
	// Calculate Packet checksum
	packet.header.checksum = packet.Checksum();

	// Send the packet
	errorCode = sendto(netSocket, (char*)&packet, packetSize, 0, (sockaddr*)&to, sizeof(to));

	// Check for errors
	if (errorCode != SOCKET_ERROR)
	{
		// Update traffic counters
		trafficCounters.numPacketsSent++;
		trafficCounters.numBytesSent += packetSize;
	}
	else
	{
		// **DEBUG**
		logFile << "SendTo error: ";
		DumpAddr(to);
		logFile << endl;
		logFile << WSAGetLastError() << endl;
	}

	return (errorCode != SOCKET_ERROR);
}


// -------------------------------------------

bool OPUNetTransportLayer::SendStatusUpdate()
{
	Packet packet;

	// Fill in a Status Update packet
	packet.header.destPlayerNetID = 0;
	packet.header.sourcePlayerNetID = playerNetID;
	packet.header.sizeOfPayload = sizeof(StatusUpdate);
	packet.header.type = 1;
	packet.tlMessage.tlHeader.commandType = tlcUpdateStatus;
	packet.tlMessage.statusUpdate.newStatus = peerInfo[playerNetID & 7].status;		// Copy local status

	// Send the new status to the host
	return SendTo(packet, peerInfo[0].address);
}


// -------------------------------------------

bool OPUNetTransportLayer::SendUntilStatusUpdate(Packet *packet, int untilStatus, int maxTries, int repeatDelay)
{
	int numTries;
	int numPlayers;
	int playerNum;
	int index;
	bool bStillWaiting;
	int playerNetIDList[MaxRemotePlayers];
	Packet dummyPacket;

	// Checksum the packet
	packet->header.checksum = packet->Checksum();

	// Get the opponent playerNetIDList
	numPlayers = GetOpponentNetIDList(playerNetIDList, MaxRemotePlayers);

	// Repeat sending packet
	for (numTries = 0; numTries < maxTries; numTries++)
	{
		// Haven't yet sent any packets
		bStillWaiting = false;
		// Resend packet to all players who haven't updated status
		for (playerNum = 0; playerNum < numPlayers; playerNum++)
		{
			// Get the index of the current player peer info
			index = playerNetIDList[playerNum] & 7;
			if (peerInfo[index].status != untilStatus)
			{
				// Sent packet to this player
				SendTo(*packet, peerInfo[index].address);
			}
		}

		// Check for success
		if (bStillWaiting == false)
			return true;		// Success

		// Wait
		Sleep(repeatDelay);

		// Pump the message receive processing
		while(Receive(&dummyPacket))
			;
	}

	return false;		// Failed
}


// -------------------------------------------

// Returns true if the packet was processed, and false otherwise
bool OPUNetTransportLayer::DoImmediateProcessing(Packet &packet, sockaddr_in &from)
{
	// Make sure it's an immediately processed TransportLayer message
	if (packet.header.type != 1)
		return false;				// Abort. Non immediate transport message

	// Create shorthand reference to known packet type
	TransportLayerMessage &tlMessage = packet.tlMessage;

	// Check if we need to repond to game host queries
	if (bInvite)
	{
		int returnPortNum;

		switch (tlMessage.tlHeader.commandType)
		{
		case tlcJoinRequest:		// 0: JoinRequest
			// Verify packet size
			if (packet.header.sizeOfPayload != sizeof(JoinRequest))
				return true;		// Packet handled (discard)
			// Check the session identifier
			if (packet.tlMessage.joinRequest.sessionIdentifier != hostedGameInfo.sessionIdentifier)
				return true;		// Packet handled (discard)

			returnPortNum = tlMessage.joinRequest.returnPortNum;

			// Create a reply
			packet.header.sourcePlayerNetID = playerNetID;		// Client will need the Host's ID
			packet.header.sizeOfPayload = sizeof(JoinReply);
			tlMessage.joinReply.newPlayerNetID = AddPlayer(from);
			// Determine if join was successful
			if (tlMessage.joinReply.newPlayerNetID != 0)
			{
				tlMessage.tlHeader.commandType = tlcJoinGranted;
				// **DEBUG**
				logFile << "Client join accepted: ";
				DumpAddr(from);
				logFile << " (" << tlMessage.joinReply.newPlayerNetID << ")" << endl;

				// Check if a forced return port has been set
				if (returnPortNum != 0)
				{
					logFile << "Return Port forced to " << returnPortNum << endl;
					// Set the new return port number
					peerInfo[tlMessage.joinReply.newPlayerNetID & 7].address.sin_port = returnPortNum;
				}
			}
			else
			{
				tlMessage.tlHeader.commandType = tlcJoinRefused;
				// **DEBUG**
				logFile << "Client join refused: ";
				DumpAddr(from);
				logFile << endl;
			}

			// Send the reply
			SendTo(packet, from);

			return true;			// Packet handled
		case tlcHostedGameSearchQuery:		// 7: HostedGameSearchQuery  [Custom format]
			// Verify packet size
			if (packet.header.sizeOfPayload != sizeof(HostedGameSearchQuery))
				return true;		// Packet handled (discard)
// **DEBUG**
logFile << "Game Search Query: ";
DumpAddr(from);
logFile << endl;
			// Verify Game Identifier
			if (tlMessage.searchQuery.gameIdentifier != gameIdentifier)
				return true;		// Packet handled (discard)
			// Verify password
			if (strncmp(tlMessage.searchQuery.password, password, sizeof(password)) != 0)
				return true;		// Packet handled (discard)

			// Create a reply
			packet.header.sizeOfPayload = sizeof(HostedGameSearchReply);
			tlMessage.tlHeader.commandType = tlcHostedGameSearchReply;
			tlMessage.searchReply.sessionIdentifier = hostedGameInfo.sessionIdentifier;
			tlMessage.searchReply.createGameInfo = hostedGameInfo.createGameInfo;
			tlMessage.searchReply.hostAddress.sin_addr.s_addr = 0;		// Clear return address  (NAT will obscure it, let a game server or client fix it when the packet is received)

			// Send the reply
			SendTo(packet, from);

			return true;			// Packet handled
		}
	}

	// Check if we need to respond to pre game setup messages
	if (bGameStarted == false)
	{
		switch (tlMessage.tlHeader.commandType)
		{
		case tlcStartGame:
// **DEBUG**
logFile << "GameStarting" << endl;
			break;
		case tlcSetPlayersList:
			// Verify packet size
			if (packet.header.sizeOfPayload != sizeof(PlayersList))
				return true;		// Packet handled (discard)

			if (peerInfo[playerNetID & 7].status == 2)
			{
				// Copy the number of players
				numPlayers = tlMessage.playersList.numPlayers;

				// Copy the Player List
				int i;
				for (i = 1; i < MaxRemotePlayers; i++)
				{
					peerInfo[i].address.sin_family = AF_INET;
					peerInfo[i].address.sin_port = tlMessage.playersList.netPeerInfo[i].port;
					peerInfo[i].address.sin_addr.s_addr = tlMessage.playersList.netPeerInfo[i].ip;
					memset(peerInfo[i].address.sin_zero, 0, sizeof(peerInfo[i].address.sin_zero));
					peerInfo[i].status = tlMessage.playersList.netPeerInfo[i].status;
					peerInfo[i].playerNetID = tlMessage.playersList.netPeerInfo[i].playerNetID;
				}

// **DEBUG**
logFile << "Replicated Players List:" << endl;
DumpAddrList(peerInfo);

				// Form a new packet to return to the game
				packet.header.sourcePlayerNetID = 0;
				packet.header.sizeOfPayload = 4;
				packet.tlMessage.tlHeader.commandType = tlcStartGame;

				// Send Status update to host
				SendStatusUpdate();

				return false;		// Return packet for further processing
			}

			// Send Status update to host
			SendStatusUpdate();

			return true;			// Packet handled
		case tlcSetPlayersListFailed:
			peerInfo[0].status = 4;						// **
			peerInfo[playerNetID & 7].status = 4;		// **

			// Form a new packet to return to the game
			packet.header.sizeOfPayload = 4;
			packet.header.sourcePlayerNetID = 0;
			packet.tlMessage.tlHeader.commandType = tlcSetPlayersList;

			return false;			// Return packet for further processing
			break;

		case tlcUpdateStatus:
			// Verify packet size
			if (packet.header.sizeOfPayload != sizeof(StatusUpdate))
				return true;		// Packet handled (discard)

			// Cache which peerInfo struct needs to be updated
			PeerInfo* updatedPeerInfo;
			updatedPeerInfo = &peerInfo[packet.header.sourcePlayerNetID & 7];

			// Check if we need to mark joining
			if ((updatedPeerInfo->status == 1) && (packet.tlMessage.statusUpdate.newStatus == 2))
			{
				// Mark this player for returning a join packet
				updatedPeerInfo->bReturnJoinPacket = true;
			}

			// Update the Player Status
			if (updatedPeerInfo->status <= tlMessage.statusUpdate.newStatus)
				updatedPeerInfo->status = tlMessage.statusUpdate.newStatus;

			return true;			// Packet handled

		case tlcHostedGameSearchReply:		// [Custom format]
// **DEBUG**
logFile << "Hosted Game Search Reply: ";
DumpAddr(from);
logFile << endl;
			// Verify packet size
			if (packet.header.sizeOfPayload != sizeof(HostedGameSearchReply))
				return true;		// Packet handled (discard)
			// Check the game identifier
			if (packet.tlMessage.searchReply.gameIdentifier != gameIdentifier)
				return true;		// Packet handled (discard)

			// Update the interal address if needed
			if (packet.tlMessage.searchReply.hostAddress.sin_addr.s_addr == 0)
			{
				// Update the from address to that of the sender  (NAT will hide the real return address from the sender)
				packet.tlMessage.searchReply.hostAddress = from;
			}
			else
			{
				// Just make sure the address family is correct
				packet.tlMessage.searchReply.hostAddress.sin_family = from.sin_family;
			}

			return false;			// Return Packet for processing
		}
	}

	return false;					// Unhandled (non-immediate) message
}


bool OPUNetTransportLayer::PokeGameServer(PokeStatusCode status)
{
	Packet packet;
	sockaddr_in gameServerAddr;
	int errorCode;

	// Find the game server address
	errorCode = GetGameServerAddress(gameServerAddr);
	// Check for errors
	if (errorCode == 0)
		return false;

	// Fill in the packet header
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(GameServerPoke);
	packet.header.type = 1;
	// Fill in the command type
	packet.tlMessage.tlHeader.commandType = tlcGameServerPoke;
	packet.tlMessage.gameServerPoke.statusCode = status;
	packet.tlMessage.gameServerPoke.randValue = randValue;
	// Calculate the checksum
	packet.header.checksum = packet.Checksum();

	// **TODO** Set timeout for a server response?
	//  Might need to resend Game Hosted packet if it gets dropped
	//  Or maybe even Game Started or Game Cancelled

	// Send the packet
	return SendTo(packet, gameServerAddr);
}


bool OPUNetTransportLayer::GetGameServerAddress(sockaddr_in &gameServerAddr)
{
	int errorCode;
	char addrString[256];

	// Get the address string
	GetGameServerAddressString(addrString, sizeof(addrString));

	// Set default address values
	gameServerAddr.sin_family = AF_INET;
	gameServerAddr.sin_port = htons(DefaultGameServerPort);
	memset(gameServerAddr.sin_zero, 0, sizeof(gameServerAddr.sin_zero));

	// Convert the address string to a sockaddr_in struct
	errorCode = GetHostAddress(addrString, gameServerAddr);
	if (errorCode == -1)
		return true;		// Success
	else
		return false;		// Error
}

void OPUNetTransportLayer::GetGameServerAddressString(char* gameServerAddressString, int maxLength)
{
	// Get the address string
	config.GetString(sectionName, "GameServerAddr", gameServerAddressString, maxLength, "");

	logFile << "[" << sectionName << "]" << " GameServerAddr = " << gameServerAddressString << endl;
}
