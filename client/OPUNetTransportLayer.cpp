// **TODO** Only answer first join request (game host)
// **TODO** Discard packets from bad net ids (non-zero values that don't match up to proper index, (with proper source IP?))

#include "OPUNetTransportLayer.h"
#include "Log.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <string>
#include <cstring>

extern char sectionName[];


bool ValidatePacket(Packet& packet, sockaddr_in& fromAddress);


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
		return nullptr;
	}

	// Create a socket for join/host
	if (!opuNetTransportLayer->CreateSocket())
	{
		// Error
		delete opuNetTransportLayer;
		return nullptr;
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
			LogDebug("ForcedPort = " + std::to_string(forcedPort));
		}
		else
		{
			Log("Warning: Could not bind to ForcedPort = " + std::to_string(forcedPort));
			forcedPort = 0;		// Clear this so we don't try to use it when joining games
		}
	}

	// Return status
	return netSocket != INVALID_SOCKET;
}

bool OPUNetTransportLayer::HostGame(Port port, const char* hostPassword, const char* creatorName, int maxPlayers, int gameType)
{
	ClearPlayers();

	sockaddr_in localAddress;
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
		int errorCode = bind(hostSocket, (sockaddr*)&localAddress, sizeof(localAddress));
		// Check for errors
		if (errorCode == SOCKET_ERROR)
		{
			// Could not bind to the socket. Get client socket port info
			int sizeofAddress = sizeof(localAddress);
			errorCode = getsockname(netSocket, (sockaddr*)&localAddress, &sizeofAddress);
			if (errorCode == SOCKET_ERROR) {
				return false;
			}
			// Check if client socket port doesn't match
			if (ntohs(localAddress.sin_port) != port) {
				return false;
			}

			// Ports match, use the client socket
			hostSocket = netSocket;
		}

		LogDebug("Bound to server port: " + std::to_string(port));
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
	// Copy the game creator name and hostPassword
	strncpy_s(hostedGameInfo.createGameInfo.gameCreatorName, creatorName, sizeof(hostedGameInfo.createGameInfo.gameCreatorName));
	strncpy_s(this->hostPassword, hostPassword, sizeof(this->hostPassword));

	LogDebug(" Session ID: " + FormatGuid(hostedGameInfo.sessionIdentifier));

	// Create a Host playerNetID
	playerNetID = PlayerNetID::SetCurrentTime(HostPlayerIndex);
	LogDebug(" Host playerNetID: " + FormatPlayerNetID(playerNetID));
	// Set the host fields
	peerInfos[HostPlayerIndex].playerNetID = playerNetID;
	peerInfos[HostPlayerIndex].address = localAddress;
	peerInfos[HostPlayerIndex].status = PeerStatus::Normal;
	// Update number of players
	numPlayers = 1;

	// Enable game host query replies
	bInvite = true;


	// Poke the game server (and let it know a new game is hosted)
	int errorCode = PokeGameServer(PokeStatusCode::GameHosted);
	// Check for errors
	if (errorCode == 0)
	{
		Log("Error informing game server");
	}

	// Return success status
	return true;
}

bool OPUNetTransportLayer::GetExternalAddress()
{
	// Build the request packet
	Packet packet;
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(RequestExternalAddress);
	packet.header.type = 1;
	packet.tlMessage.tlHeader.commandType = TransportLayerCommand::RequestExternalAddress;
	packet.tlMessage.requestExternalAddress.internalPort = GetPort();

	// Get the game server address
	sockaddr_in gameServerAddr;
	if (!GetGameServerAddress(gameServerAddr))
	{
		// Error. Could not obtain game server address
		return false;		// Error
	}

	// Send the request packet to the game server (first port)
	bool errorCode = SendTo(packet, gameServerAddr);
	if (errorCode == false) {
		return false;
	}

	// Send the request packet to the game server (second port)
	gameServerAddr.sin_port = htons(ntohs(gameServerAddr.sin_port) + 1);
	errorCode = SendTo(packet, gameServerAddr);

	return errorCode;
}

bool OPUNetTransportLayer::SearchForGames(char* hostAddressString, Port defaultHostPort)
{
	// Create the default host address
	sockaddr_in hostAddress;
	hostAddress.sin_family = AF_INET;
	hostAddress.sin_port = htons(defaultHostPort);
	hostAddress.sin_addr.S_un.S_addr = INADDR_BROADCAST;

	// Try to convert the string fields
	auto errorCode = GetHostAddress(hostAddressString, hostAddress);
	if (errorCode == HostAddressCode::InvalidAddress) {
		return false;
	}

	// Construct the SearchQuery packet
	Packet packet;
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(HostedGameSearchQuery);
	packet.header.type = 1;
	packet.tlMessage.searchQuery.commandType = TransportLayerCommand::HostedGameSearchQuery;
	packet.tlMessage.searchQuery.gameIdentifier = gameIdentifier;
	packet.tlMessage.searchQuery.timeStamp = timeGetTime();
	packet.tlMessage.searchQuery.password[0] = 0;

	LogDebug("Search for games: " + FormatAddress(hostAddress));

	// Send the HostGameSearchQuery
	return SendTo(packet, hostAddress);
}

bool OPUNetTransportLayer::JoinGame(HostedGameInfo &game, const char* joinRequestPassword)
{
	ClearPlayers();

	// Store a pointer to the game we're trying to join
	joiningGameInfo = &game;

	// Construct the JoinRequest packet
	Packet packet;
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(JoinRequest);
	packet.header.type = 1;
	packet.tlMessage.joinRequest.commandType = TransportLayerCommand::JoinRequest;
	packet.tlMessage.joinRequest.sessionIdentifier = game.sessionIdentifier;
	packet.tlMessage.joinRequest.returnPortNum = forcedPort;
	strncpy_s(packet.tlMessage.joinRequest.password, joinRequestPassword, sizeof(packet.tlMessage.joinRequest.password));

	LogDebug("Sending join request: " + FormatAddress(game.address));
	LogDebug("  Session ID: " + FormatGuid(packet.tlMessage.joinRequest.sessionIdentifier));
	LogDebug(FormatPacket(packet));

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
	// Make sure a join was requested
	if (joiningGameInfo == nullptr) {
		return;
	}

	// Join successful
	// ---------------
	// Store the Host info
	peerInfos[HostPlayerIndex].playerNetID = packet.header.sourcePlayerNetID;	// Store Host playerNetID
	peerInfos[HostPlayerIndex].address = joiningGameInfo->address;				// Store Host address
	peerInfos[HostPlayerIndex].status = PeerStatus::Normal;
	// Get the assigned playerNetID
	playerNetID = packet.tlMessage.joinReply.newPlayerNetID;	// Store playerNetID
	int localPlayerNum = PlayerNetID::GetPlayerIndex(playerNetID);   // Cache (frequently used)
	// Update local info
	peerInfos[localPlayerNum].playerNetID = playerNetID;
	peerInfos[localPlayerNum].address.sin_addr.s_addr = INADDR_ANY;	// Clear the address
	peerInfos[localPlayerNum].status = PeerStatus::Normal;

	LogDebug("OnJoinAccepted");
	LogDebug(FormatPlayerList(peerInfos));

	// Update num players (for quit messages from cancelled games)
	numPlayers = 1;

	// Send updated status to host
	bool bSuccess = SendStatusUpdate();

	if (!bSuccess) {
		LogError("Error sending updated status to host");
	}

	// Reset network traffic counters
	ResetTrafficCounters();
}


int OPUNetTransportLayer::GetNumPlayers()
{
	return numPlayers;
}


int OPUNetTransportLayer::GetPort()
{
	sockaddr_in address;
	int addressLength = sizeof(address);

	// Get the local socket address
	int errorCode = getsockname(netSocket, (sockaddr*)&address, &addressLength);
	if (errorCode == SOCKET_ERROR) {
		return 0;		// Error
	}

	// Return the port number
	return ntohs(address.sin_port);
}

bool OPUNetTransportLayer::GetAddress(sockaddr_in& addr)
{
	int addressLength = sizeof(addr);

	// Get the local socket address
	int errorCode = getsockname(netSocket, (sockaddr*)&addr, &addressLength);

	// Return true if failed
	return (errorCode == SOCKET_ERROR);
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
		PokeGameServer(PokeStatusCode::GameCancelled);
	}

	// Make sure we don't Cleanup if we haven't done Startup
	if (bInitialized)
	{
		// Release the sockets
		if (netSocket != INVALID_SOCKET) {
			closesocket(netSocket);
		}
		if (hostSocket != INVALID_SOCKET) {
			closesocket(hostSocket);
		}

		// Shutdown Winsock
		WSACleanup();
	}
}

int OPUNetTransportLayer::GetHostPlayerNetID()
{
	return peerInfos[HostPlayerIndex].playerNetID;
}

// Called when the game is starting (but not when cancelled)
void OPUNetTransportLayer::ShutDownInvite()
{
	// Disable game host query replies
	bInvite = false;
	bGameStarted = true;

	// Let the game server know the game is starting
	PokeGameServer(PokeStatusCode::GameStarted);
}

int OPUNetTransportLayer::ReplicatePlayersList()
{
	// Fill in the packet header
	Packet packet;
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(PlayersList);
	packet.header.type = 1;
	// Fill in the packet body
	packet.tlMessage.playersList.commandType = TransportLayerCommand::SetPlayersList;
	packet.tlMessage.playersList.numPlayers = numPlayers;
	// Copy the players list
	for (int i = 0; i < MaxRemotePlayers; i++)
	{
		packet.tlMessage.playersList.netPeerInfo[i].ip = peerInfos[i].address.sin_addr.s_addr;
		packet.tlMessage.playersList.netPeerInfo[i].port = peerInfos[i].address.sin_port;
		packet.tlMessage.playersList.netPeerInfo[i].status = peerInfos[i].status;
		packet.tlMessage.playersList.netPeerInfo[i].playerNetID = peerInfos[i].playerNetID;
	};


	// Send the Player List
	int retVal = SendUntilStatusUpdate(packet, PeerStatus::ReplicateSuccess, 16, 500);


	// Check for errors
	if (retVal == 0)
	{
		// Failed. Inform other players of failure.
		packet.header.sourcePlayerNetID = 0;
		packet.header.destPlayerNetID = 0;
		packet.header.sizeOfPayload = 4;
		packet.header.type = 1;
		packet.tlMessage.tlHeader.commandType = TransportLayerCommand::SetPlayersListFailed;

		// Wait until all clients acknowledge failure
		SendUntilStatusUpdate(packet, PeerStatus::ReplicateFailure, 16, 500);
		return -1;		// Failed
	}


	// Success
	peerInfos[HostPlayerIndex].status = PeerStatus::ReplicateSuccess;
	return 1;
}

// Fills the netIDList with opponent (non-local) playerNetIDs
// Returns the number of entries copied into the supplied array
// or -1 if the output buffer is too small, (after having been filled)
int OPUNetTransportLayer::GetOpponentNetIDList(int netIDList[], int maxNumID)
{
	int j = 0;

	// Copy all non local playerNetIDs
	for (const PeerInfo& peerInfo : peerInfos)
	{
		// Make sure the ID is valid
		if (peerInfo.playerNetID != 0)
		{
			// Make sure it doesn't match the local player
			if (peerInfo.playerNetID != playerNetID)
			{
				// Abort if the output buffer is full
				if (j >= maxNumID) {
					return -1;		// Error. Buffer too small
				}

				// Copy it to the dest buffer
				netIDList[j] = peerInfo.playerNetID;
				j++;
			}
		}
	}

	return j;		// Success
}

void OPUNetTransportLayer::RemovePlayer(int playerNetID)
{
	// Detemine which player to remove
	unsigned int playerIndex = PlayerNetID::GetPlayerIndex(playerNetID);

	// Make sure the player exists
	if (peerInfos[playerIndex].status != PeerStatus::EmptySlot)
	{
		// Remove the player
		peerInfos[playerIndex].Clear();
		// Update player count
		numPlayers--;
	}
}

int OPUNetTransportLayer::Send(Packet& packet)
{
	packet.header.sourcePlayerNetID = playerNetID;
	int packetSize = packet.header.sizeOfPayload + sizeof(packet.header);
	packet.header.checksum = packet.Checksum();

	// Check if this packet should be broadcast
	if (packet.header.destPlayerNetID == 0)
	{
		SendBroadcast(packet, packetSize);
	}
	else
	{
		SendSinglecast(packet, packetSize);
	}

	return true;
}

void OPUNetTransportLayer::SendBroadcast(Packet& packet, int packetSize)
{
	// Send packet to all players
	for (PeerInfo& peerInfo : peerInfos)
	{
		// Make sure the player record is valid
		if (peerInfo.status != PeerStatus::EmptySlot)
		{
			// Don't send to self
			if (peerInfo.playerNetID != playerNetID)
			{
				// Send the packet to current player
				sockaddr_in* address = &peerInfo.address;
				int errorCode = sendto(netSocket, reinterpret_cast<char*>(&packet), packetSize, 0, (sockaddr*)address, sizeof(*address));

				if (errorCode != SOCKET_ERROR)
				{
					trafficCounters.numPacketsSent++;
					trafficCounters.numBytesSent += packetSize;
				}
			}
		}
	}
}

void OPUNetTransportLayer::SendSinglecast(Packet& packet, int packetSize)
{
	PeerInfo& peerInfo = peerInfos[packet.header.destPlayerNetID & 7];

	// Make sure the player record is valid
	if (peerInfo.status != PeerStatus::EmptySlot)
	{
		// Don't send to self
		if (peerInfo.playerNetID != playerNetID)
		{
			sockaddr_in* address = &peerInfo.address;
			int errorCode = sendto(netSocket, reinterpret_cast<char*>(&packet), packetSize, 0, (sockaddr*)address, sizeof(*address));

			if (errorCode != SOCKET_ERROR)
			{
				trafficCounters.numPacketsSent++;
				trafficCounters.numBytesSent += packetSize;
			}
		}
	}
}

int OPUNetTransportLayer::Receive(Packet& packet)
{
	for (;;)
	{
		// Check if we need to return a JoinReturned packet
		if (numJoining != 0)
		{
			// Check each player for joining
			for (PeerInfo& peerInfo : peerInfos)
			{
				// Check if this player is joining
				if (peerInfo.bReturnJoinPacket)
				{
					// Construct the JoinGranted packet
					// Note: This packet is returned as if it was received over the network
					// Note: Required sourcePlayerNetID=0 for: 1=JoinGranted, 3=RemoteStart, 4=SetPlayerList
					packet.header.sourcePlayerNetID = 0;	// Must be 0 to be processed
					packet.header.destPlayerNetID = playerNetID;		// Send fake packet to self
					packet.header.sizeOfPayload = sizeof(JoinReturned);
					packet.header.type = 1;
					packet.tlMessage.tlHeader.commandType = TransportLayerCommand::JoinGranted;
					packet.tlMessage.joinReturned.newPlayerNetID = peerInfo.playerNetID;

					// Mark as returned
					peerInfo.bReturnJoinPacket = false;
					numJoining--;
					return true;		// Return packet for processing
				}
				else
				{
					// Check if partially joined
					if (peerInfo.status == PeerStatus::Joining)
					{
						// Check if timed out
						if (timeGetTime() - PlayerNetID::GetTimeStamp(peerInfo.playerNetID) > JoinTimeOut)
						{
							// Cancel the join, and reclaim the player record
							numPlayers--;
							numJoining--;
							peerInfo.bReturnJoinPacket = false;
							peerInfo.Clear();
						}
					}
				}
			}
		}


		// Try to read from the net socket
		sockaddr_in fromAddress;
		auto numBytes = ReadSocket(netSocket, packet, fromAddress);
		// Check for errors
		if (numBytes == -1)
		{
			// Try to read from the host socket
			numBytes = ReadSocket(hostSocket, packet, fromAddress);
			// Check for errors
			if (numBytes == -1) {
				return false;
			}
		}

		LogDebug("ReadSocket: type = " + std::to_string(packet.header.type)
			+ "  commandType = " + FormatTransportLayerCommandIncludeIndex(packet.tlMessage.tlHeader.commandType)
			+ "  sourcePlayerNetID = " + std::to_string(packet.header.sourcePlayerNetID));

		// Error check the packet
		if (static_cast<std::size_t>(numBytes) < sizeof(PacketHeader)) {
			continue;		// Discard packet
		}
		if (static_cast<std::size_t>(numBytes) < sizeof(PacketHeader) + packet.header.sizeOfPayload) {
			continue;		// Discard packet
		}
		if (packet.header.checksum != packet.Checksum()) {
			continue;		// Discard packet
		}

		// Check for packets with invalid playerNetID
		int sourcePlayerNetID = packet.header.sourcePlayerNetID;
		if (sourcePlayerNetID != 0)
		{
			auto playerIndex = PlayerNetID::GetPlayerIndex(sourcePlayerNetID);

			// Discard packet if playerIndex is invalid
			if (playerIndex >= MaxRemotePlayers) {
				continue;
			}

			int expectedPlayerNetID = peerInfos[playerIndex].playerNetID;
			if (expectedPlayerNetID != 0 && expectedPlayerNetID != sourcePlayerNetID)
			{
				Log("Received packet with bad sourcePlayerNetID: " + std::to_string(sourcePlayerNetID) +
					" from " + FormatAddress(fromAddress));
				Log(" Packet.type = " + std::to_string(packet.header.type));
				Log(" Packet.commandType = " + std::to_string(static_cast<int>(packet.tlMessage.tlHeader.commandType)));
			}
		}

		// Count the received packet
		trafficCounters.numPacketsReceived++;
		trafficCounters.numBytesReceived += packet.header.sizeOfPayload + sizeof(packet.header);

		// Check for unexpected source ports
		CheckSourcePort(packet, fromAddress);

		// Determine if immediate processing is required
		bool bRetVal = packet.header.type == 1;
		if (bRetVal) {
			bRetVal = OnImmediatePacketProcess(packet, fromAddress);
		}

		// Check destination
		if ((packet.header.destPlayerNetID != 0) && (packet.header.destPlayerNetID != playerNetID)) {
			continue;		// Discard packet
		}

		// Check if the packet was unhandled
		if (bRetVal == false)
		{
			// Validate packet makes sense (discard if it doesn't)
			if (ValidatePacket(packet, fromAddress))
			{
				// Non immediate processed packet received. Return packet
				return true;
			}
		}
	}
}

int OPUNetTransportLayer::IsHost()				// IsCurrentGameHost?
{
	return (bInvite && (playerNetID == peerInfos[HostPlayerIndex].playerNetID));
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
	// Get the address and convert it to a string
	sockaddr_in* address = &peerInfos[PlayerNetID::GetPlayerIndex(playerNetID)].address;
	scr_snprintf(addressString, bufferSize, "%i.%i.%i.%i", address->sin_addr.S_un.S_un_b.s_b1, address->sin_addr.S_un.S_un_b.s_b2, address->sin_addr.S_un.S_un_b.s_b3, address->sin_addr.S_un.S_un_b.s_b4);

	return true;
}

int OPUNetTransportLayer::ResetTrafficCounters()
{
	// Clear the TrafficCounters
	std::memset(&trafficCounters, 0, sizeof(trafficCounters));
	trafficCounters.timeOfLastReset = timeGetTime();

	return true;
}

int OPUNetTransportLayer::GetTrafficCounts(TrafficCounters& trafficCounters)
{
	// Copy the TrafficCounter to the supplied buffer
	trafficCounters = this->trafficCounters;

	return true;
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
	std::memset(&peerInfos, 0, sizeof(peerInfos));
	bInvite = false;
	bGameStarted = false;
	std::memset(&hostedGameInfo, 0, sizeof(hostedGameInfo));
	hostedGameInfo.ping = -1;
	ResetTrafficCounters();
	joiningGameInfo = nullptr;
	numJoining = 0;
	randValue = timeGetTime() ^ RandValueXor;
}


// -------------------------------------------

bool OPUNetTransportLayer::InitializeWinsock()
{
	if (!bInitialized)
	{
		// Initialize Winsock
		WORD version = MAKEWORD(2, 2);
		WSADATA wsaData;
		int err = WSAStartup(version, &wsaData);

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

OPUNetTransportLayer::HostAddressCode OPUNetTransportLayer::GetHostAddress(char* hostAddressString, sockaddr_in &hostAddress)
{
	// Check if a specific host address was indicated
	if (hostAddressString == nullptr) {
		return HostAddressCode::NoAddressSpecified;
	}

	// Convert hostAddress string to network address
	// ---------------------------------------------

	// Skip any leading spaces
	while (hostAddressString[0] == ' ' || hostAddressString[0] == '\t') {
		hostAddressString++;
	}

	// Check if a port number was specified
	char* portNumString = strchr(hostAddressString, ':');
	if (portNumString != nullptr)
	{
		// Convert the port number
		int port = atoi(&portNumString[1]);
		// Make sure a number was actually specified  (otherwise use the default port)
		if (port != 0) {
			hostAddress.sin_port = htons(port);
		}
		// Truncate the address string, so lookup succeeds
		portNumString[0] = 0;
	}

	// Check if the address part is empty  (in which case use the default)
	if (hostAddressString[0] == 0)
	{
		if (portNumString != nullptr) {
			portNumString[0] = ':';		// Restore string
		}
		return HostAddressCode::NoAddressSpecified;
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
		if (hostEnt == nullptr)
		{
			if (portNumString != nullptr) {
				portNumString[0] = ':';		// Restore string
			}
			return HostAddressCode::InvalidAddress;
		}
		// Get the host IP address
		hostAddress.sin_addr.S_un.S_addr = *(unsigned long*)hostEnt->h_addr_list[0];
	}

	if (portNumString != nullptr) {
		portNumString[0] = ':';		// Restore string
	}
	return HostAddressCode::Success;
}


// -------------------------------------------

// Returns a new playerNetID
int OPUNetTransportLayer::AddPlayer(const sockaddr_in& from)
{
	// Make sure there is room for a new player
	if (numPlayers >= hostedGameInfo.createGameInfo.startupFlags.maxPlayers) {
		return 0;		// Failed
	}

	// Find an empty slot
	for (int newPlayerIndex = 0; newPlayerIndex < MaxRemotePlayers; newPlayerIndex++)
	{
		// Check if this slot is empty
		if (peerInfos[newPlayerIndex].status == PeerStatus::EmptySlot)
		{
			// Insert player into empty slot
			// -----------------------------
			// Store the source address
			peerInfos[newPlayerIndex].address = from;
			peerInfos[newPlayerIndex].status = PeerStatus::Joining;
			peerInfos[newPlayerIndex].playerNetID = PlayerNetID::SetCurrentTime(newPlayerIndex);
			// Increase connected player count
			numPlayers++;
			numJoining++;

			// Return the new playerNetID
			return peerInfos[newPlayerIndex].playerNetID;	// Success
		}
	}

	// Failed
	return 0;
}


// -------------------------------------------

int OPUNetTransportLayer::ReadSocket(SOCKET sourceSocket, Packet& packet, sockaddr_in& from)
{
	// Check if the host socket is in use
	if (sourceSocket == INVALID_SOCKET) {
		return -1;
	}

	// Check the server port for data
	unsigned long byteCount; // ioctlsocket sets argp to number of bytes read when passed FIONREAD
	int errorCode = ioctlsocket(sourceSocket, FIONREAD, &byteCount);

	if (errorCode == SOCKET_ERROR) {
		return -1;
	}

	if (byteCount == 0) {
		return -1;
	}

	// Read the data
	int fromLen = sizeof(from);
	auto receivedByteCount = recvfrom(sourceSocket, reinterpret_cast<char*>(&packet),
		sizeof(packet), 0, reinterpret_cast<sockaddr*>(&from), &fromLen);

	if (receivedByteCount == SOCKET_ERROR) {
		return -1;
	}

	// Return number of bytes read
	return receivedByteCount;
}


// -------------------------------------------

bool OPUNetTransportLayer::SendTo(Packet& packet, const sockaddr_in& to)
{
	LogDebug("SendTo: Packet.commandType = " + FormatTransportLayerCommandIncludeIndex(packet.tlMessage.tlHeader.commandType));

	// Calculate Packet size
	int packetSize = packet.header.sizeOfPayload + sizeof(packet.header);
	// Calculate Packet checksum
	packet.header.checksum = packet.Checksum();

	// Send the packet
	int errorCode = sendto(netSocket, (char*)&packet, packetSize, 0, (sockaddr*)&to, sizeof(to));

	// Check for errors
	if (errorCode != SOCKET_ERROR)
	{
		// Update traffic counters
		trafficCounters.numPacketsSent++;
		trafficCounters.numBytesSent += packetSize;
	}
	else
	{
		Log("SendTo error: " + FormatAddress(to));
		Log(std::to_string(WSAGetLastError()));
	}

	return (errorCode != SOCKET_ERROR);
}


// -------------------------------------------

bool OPUNetTransportLayer::SendStatusUpdate()
{
	// Fill in a Status Update packet
	Packet packet;
	packet.header.destPlayerNetID = 0;
	packet.header.sourcePlayerNetID = playerNetID;
	packet.header.sizeOfPayload = sizeof(StatusUpdate);
	packet.header.type = 1;
	packet.tlMessage.tlHeader.commandType = TransportLayerCommand::UpdateStatus;
	packet.tlMessage.statusUpdate.newStatus = peerInfos[PlayerNetID::GetPlayerIndex(playerNetID)].status;		// Copy local status

	// Send the new status to the host
	return SendTo(packet, peerInfos[HostPlayerIndex].address);
}


// -------------------------------------------

bool OPUNetTransportLayer::SendUntilStatusUpdate(Packet& packet, PeerStatus untilStatus, int maxTries, int repeatDelay)
{
	// Checksum the packet
	packet.header.checksum = packet.Checksum();

	// Get the opponent playerNetIDList
	int playerNetIDList[MaxRemotePlayers];
	int numPlayers = GetOpponentNetIDList(playerNetIDList, MaxRemotePlayers);

	// Repeat sending packet
	for (int numTries = 0; numTries < maxTries; ++numTries)
	{
		// Haven't yet sent any packets
		bool bStillWaiting = false;
		// Resend packet to all players who haven't updated status
		for (int playerNum = 0; playerNum < numPlayers; playerNum++)
		{
			// Get the index of the current player peer info
			int index = PlayerNetID::GetPlayerIndex(playerNetIDList[playerNum]);
			if (peerInfos[index].status != untilStatus)
			{
				// Must wait for a response from this player
				bStillWaiting = true;
				// Sent packet to this player
				SendTo(packet, peerInfos[index].address);
			}
		}

		// Check for success
		if (bStillWaiting == false) {
			return true;
		}

		// Wait
		Sleep(repeatDelay);

		// Pump the message receive processing
		Packet dummyPacket;
		while(Receive(dummyPacket)) {
		}
	}

	return false;		// Failed
}


// -------------------------------------------

// Returns true if the packet was processed, and false otherwise
bool OPUNetTransportLayer::OnImmediatePacketProcess(Packet& packet, const sockaddr_in& fromAddress)
{
	// Create shorthand reference to known packet type
	TransportLayerMessage& tlMessage = packet.tlMessage;

	// Check if we need to repond to game host queries
	if (bInvite)
	{
		switch (tlMessage.tlHeader.commandType)
		{
		case TransportLayerCommand::JoinRequest:
			OnJoinRequest(packet, fromAddress, tlMessage);
			return true;
		case TransportLayerCommand::HostedGameSearchQuery:
			OnHostedGameSearchQuery(packet, fromAddress, tlMessage);
			return true;
		case TransportLayerCommand::JoinHelpRequest:
			if (OnJoinHelpRequest(packet, fromAddress, tlMessage)) {
				return true;
			}
			break;
		default:  // Silence warnings about unused enumeration value in switch
			break;
		}
	}

	// Check if we need to respond to pre game setup messages
	if (!bGameStarted)
	{
		switch (tlMessage.tlHeader.commandType)
		{
		case TransportLayerCommand::StartGame:
			break;
		case TransportLayerCommand::SetPlayersList:
			return OnSetPlayersList(packet, tlMessage);
		case TransportLayerCommand::SetPlayersListFailed:
			OnSetPlayersListFailed(packet);
			return false; // Return packet for further processing
		case TransportLayerCommand::UpdateStatus:
			OnUpdateStatus(packet, tlMessage);
			return true; // Packet handled
		case TransportLayerCommand::HostedGameSearchReply:
			return OnHostedGameSearchReply(packet, fromAddress);
		default:  // Silence warnings about unused enumeration value in switch
			break;
		}
	}

	return false; // Unhandled (non-immediate) message
}

void OPUNetTransportLayer::OnJoinRequest(Packet& packet, const sockaddr_in& fromAddress, TransportLayerMessage& tlMessage)
{
	// Verify packet size
	if (packet.header.sizeOfPayload != sizeof(JoinRequest)) {
		return; // Packet handled (discard)
	}
	// Check the session identifier
	if (packet.tlMessage.joinRequest.sessionIdentifier != hostedGameInfo.sessionIdentifier) {
		return; // Packet handled (discard)
	}

	int returnPortNum = tlMessage.joinRequest.returnPortNum;

	// Create a reply
	packet.header.sourcePlayerNetID = playerNetID;		// Client will need the Host's ID
	packet.header.sizeOfPayload = sizeof(JoinReply);
	tlMessage.joinReply.newPlayerNetID = AddPlayer(fromAddress);
	// Determine if join was successful
	if (tlMessage.joinReply.newPlayerNetID != 0)
	{
		tlMessage.tlHeader.commandType = TransportLayerCommand::JoinGranted;
		LogDebug("Client join accepted: " + FormatAddress(fromAddress) + ". New Player Net ID: " +
			FormatPlayerNetID(tlMessage.joinReply.newPlayerNetID));

		// Check if a forced return port has been set
		if (returnPortNum != 0)
		{
			LogDebug("Return Port forced to " + std::to_string(returnPortNum));
			// Set the new return port number
			peerInfos[PlayerNetID::GetPlayerIndex(tlMessage.joinReply.newPlayerNetID)].address.sin_port = returnPortNum;
		}
	}
	else
	{
		tlMessage.tlHeader.commandType = TransportLayerCommand::JoinRefused;
		Log("Client join refused: " + FormatAddress(fromAddress));
	}

	// Send the reply
	SendTo(packet, fromAddress);

	return; // Packet handled
}

void OPUNetTransportLayer::OnHostedGameSearchQuery(Packet& packet, const sockaddr_in& fromAddress, TransportLayerMessage& tlMessage)
{
	// Verify packet size
	if (packet.header.sizeOfPayload != sizeof(HostedGameSearchQuery)) {
		return; // Packet handled (discard)
	}

	LogDebug("Game Search Query: " + FormatAddress(fromAddress));

	// Verify Game Identifier
	if (tlMessage.searchQuery.gameIdentifier != gameIdentifier) {
		return; // Packet handled (discard)
	}
	// Verify password
	if (strncmp(tlMessage.searchQuery.password, hostPassword, sizeof(hostPassword)) != 0) {
		return; // Packet handled (discard)
	}

	// Create a reply
	packet.header.sizeOfPayload = sizeof(HostedGameSearchReply);
	tlMessage.tlHeader.commandType = TransportLayerCommand::HostedGameSearchReply;
	tlMessage.searchReply.sessionIdentifier = hostedGameInfo.sessionIdentifier;
	tlMessage.searchReply.createGameInfo = hostedGameInfo.createGameInfo;
	tlMessage.searchReply.hostAddress.sin_addr.s_addr = 0;		// Clear return address  (NAT will obscure it, let a game server or client fix it when the packet is received)

	// Send the reply
	SendTo(packet, fromAddress);

	return; // Packet handled
}

bool OPUNetTransportLayer::OnJoinHelpRequest(const Packet& packet, const sockaddr_in& fromAddress, TransportLayerMessage& tlMessage)
{
	// Verify packet size
	if (packet.header.sizeOfPayload != sizeof(JoinHelpRequest)) {
		return true;		// Packet handled (discard)
	}
	// Check the session identifier
	if (packet.tlMessage.joinRequest.sessionIdentifier != hostedGameInfo.sessionIdentifier) {
		return true;		// Packet handled (discard)
	}

	// Log JoinHelpRequest parameters
	const std::string addressStr = FormatAddress(tlMessage.joinHelpRequest.clientAddr);
	const std::string returnPortStr = std::to_string(tlMessage.joinHelpRequest.returnPortNum);
	LogDebug("JoinHelpRequest: Client: " + addressStr + "  Return Port: " + returnPortStr);

	// Send something to create router mappings
	tlMessage.joinHelpRequest.clientAddr.sin_family = AF_INET;
	if (tlMessage.joinHelpRequest.returnPortNum != 0) {
		tlMessage.joinHelpRequest.clientAddr.sin_port = htons(tlMessage.joinHelpRequest.returnPortNum);
	}
	sendto(netSocket, (char*)&packet, 0, 0, (sockaddr*)&packet.tlMessage.joinHelpRequest.clientAddr, sizeof(packet.tlMessage.joinHelpRequest.clientAddr));

	return false;
}

bool OPUNetTransportLayer::OnSetPlayersList(Packet& packet, const TransportLayerMessage& tlMessage)
{
	// Verify packet size
	if (packet.header.sizeOfPayload != sizeof(PlayersList)) {
		return true;		// Packet handled (discard)
	}

	if (peerInfos[PlayerNetID::GetPlayerIndex(playerNetID)].status == PeerStatus::Normal)
	{
		// Copy the number of players
		numPlayers = tlMessage.playersList.numPlayers;

		// Copy the Player List
		int i;
		for (i = 1; i < MaxRemotePlayers; i++)
		{
			peerInfos[i].address.sin_family = AF_INET;
			peerInfos[i].address.sin_port = tlMessage.playersList.netPeerInfo[i].port;
			peerInfos[i].address.sin_addr.s_addr = tlMessage.playersList.netPeerInfo[i].ip;
			std::memset(peerInfos[i].address.sin_zero, 0, sizeof(peerInfos[i].address.sin_zero));
			peerInfos[i].status = tlMessage.playersList.netPeerInfo[i].status;
			peerInfos[i].playerNetID = tlMessage.playersList.netPeerInfo[i].playerNetID;
		}

		LogDebug("Replicated Players List:");
		LogDebug(FormatPlayerList(peerInfos));

		// Form a new packet to return to the game
		packet.header.sourcePlayerNetID = 0;
		packet.header.sizeOfPayload = 4;
		packet.tlMessage.tlHeader.commandType = TransportLayerCommand::StartGame;

		// Send Status update to host
		SendStatusUpdate();

		return false;		// Return packet for further processing
	}

	// Send Status update to host
	SendStatusUpdate();

	return true;			// Packet handled
}

void OPUNetTransportLayer::OnSetPlayersListFailed(Packet& packet)
{
	peerInfos[HostPlayerIndex].status = PeerStatus::ReplicateFailure;
	peerInfos[PlayerNetID::GetPlayerIndex(playerNetID)].status = PeerStatus::ReplicateFailure;

	// Form a new packet to return to the game
	packet.header.sizeOfPayload = 4;
	packet.header.sourcePlayerNetID = 0;
	packet.tlMessage.tlHeader.commandType = TransportLayerCommand::SetPlayersList;
}

void OPUNetTransportLayer::OnUpdateStatus(const Packet& packet, const TransportLayerMessage& tlMessage)
{
	// Verify packet size
	if (packet.header.sizeOfPayload != sizeof(StatusUpdate)) {
		return;		// Packet handled (discard)
	}

	// Cache which peerInfo struct needs to be updated
	PeerInfo* updatedPeerInfo;
	updatedPeerInfo = &peerInfos[PlayerNetID::GetPlayerIndex(packet.header.sourcePlayerNetID)];

	// Check if we need to mark joining
	if ((updatedPeerInfo->status == PeerStatus::Joining) && (packet.tlMessage.statusUpdate.newStatus == PeerStatus::Normal))
	{
		// Mark this player for returning a join packet
		updatedPeerInfo->bReturnJoinPacket = true;
	}

	// Update the Player Status
	if (updatedPeerInfo->status <= tlMessage.statusUpdate.newStatus) {
		updatedPeerInfo->status = tlMessage.statusUpdate.newStatus;
	}
}

bool OPUNetTransportLayer::OnHostedGameSearchReply(Packet& packet, const sockaddr_in& fromAddress)
{
	LogDebug("Hosted Game Search Reply: " + FormatAddress(fromAddress));

	// Verify packet size
	if (packet.header.sizeOfPayload != sizeof(HostedGameSearchReply)) {
		return true;		// Packet handled (discard)
	}

	// Check the game identifier
	if (packet.tlMessage.searchReply.gameIdentifier != gameIdentifier) {
		return true;		// Packet handled (discard)
	}

	// Update the internal address if needed
	if (packet.tlMessage.searchReply.hostAddress.sin_addr.s_addr == 0)
	{
		// Update the from address to that of the sender  (NAT will hide the real return address from the sender)
		packet.tlMessage.searchReply.hostAddress = fromAddress;
	}
	else
	{
		// Just make sure the address family is correct
		packet.tlMessage.searchReply.hostAddress.sin_family = fromAddress.sin_family;
	}

	return false;			// Return Packet for processing
}

bool OPUNetTransportLayer::PokeGameServer(PokeStatusCode status)
{
	// Find the game server address
	sockaddr_in gameServerAddress;
	int errorCode = GetGameServerAddress(gameServerAddress);
	// Check for errors
	if (errorCode == 0) {
		return false;
	}

	// Fill in the packet header
	Packet packet;
	packet.header.sourcePlayerNetID = 0;
	packet.header.destPlayerNetID = 0;
	packet.header.sizeOfPayload = sizeof(GameServerPoke);
	packet.header.type = 1;
	// Fill in the command type
	packet.tlMessage.tlHeader.commandType = TransportLayerCommand::GameServerPoke;
	packet.tlMessage.gameServerPoke.statusCode = status;
	packet.tlMessage.gameServerPoke.randValue = randValue;
	// Calculate the checksum
	packet.header.checksum = packet.Checksum();

	// **TODO** Set timeout for a server response?
	//  Might need to resend Game Hosted packet if it gets dropped
	//  Or maybe even Game Started or Game Cancelled

	// Send the packet
	return SendTo(packet, gameServerAddress);
}


bool OPUNetTransportLayer::GetGameServerAddress(sockaddr_in &gameServerAddress)
{
	// Get the address string
	char addressString[256];
	GetGameServerAddressString(addressString, sizeof(addressString));

	// Set default address values
	gameServerAddress.sin_family = AF_INET;
	gameServerAddress.sin_port = htons(DefaultGameServerPort);
	std::memset(gameServerAddress.sin_zero, 0, sizeof(gameServerAddress.sin_zero));

	// Convert the address string to a sockaddr_in struct
	auto errorCode = GetHostAddress(addressString, gameServerAddress);
	
	return errorCode == HostAddressCode::Success;
}

void OPUNetTransportLayer::GetGameServerAddressString(char* gameServerAddressString, int maxLength)
{
	// Get the address string
	config.GetString(sectionName, "GameServerAddr", gameServerAddressString, maxLength, "");

	LogDebug("GameServerAddr = " + std::string(gameServerAddressString));
}


void OPUNetTransportLayer::CheckSourcePort(Packet &packet, sockaddr_in &from)
{
	int sourcePlayerNetId = packet.header.sourcePlayerNetID;

	if (sourcePlayerNetId == 0) {
		return;		// Don't try to update if they don't explicitly say who it's from
	}

	if (bGameStarted)
	{
		const int sourcePlayerIndex = PlayerNetID::GetPlayerIndex(sourcePlayerNetId);
		PeerInfo &sourcePlayerPeerInfo = peerInfos[sourcePlayerIndex];
		unsigned short expectedPort = sourcePlayerPeerInfo.address.sin_port;
		unsigned short sourcePort = from.sin_port;

		// Verify source port
		if ((expectedPort != sourcePort) && (expectedPort != 0))
		{
			// Port mismatch. Issue warning
			Log("Packet from player " + std::to_string(sourcePlayerIndex) +
				" (" + FormatAddress(from) + ") received on unexpected port (" +
				std::to_string(ntohs(sourcePort)) + " instead of " +
				std::to_string(ntohs(expectedPort)) +
				") PlayerNetId: " + FormatPlayerNetID(sourcePlayerNetId));
		}
		// Update the source port
		sourcePlayerPeerInfo.address.sin_port = sourcePort;
	}
}

void OPUNetTransportLayer::ClearPlayers()
{
	numPlayers = 0;
	for (PeerInfo& peerInfo : peerInfos)
	{
		peerInfo.Clear();
	}
}
