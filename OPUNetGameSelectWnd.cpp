// **TODO**
// Change the text of the network game type button  ("Serial")
// Allow port numbers to be specified when hosting
// Integrate this with Outpost2.exe a little better  (think: next patch)
// Have listed games timeout eventually and remove from list
//  (on receive of new hosted game?)  (the list is cleared if they click search)


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <shlobj.h>
#include <stdio.h>
#include <cstring>
#include <string>

#include "OPUNetGameSelectWnd.h"
#include "OPUNetTransportLayer.h"
#include "Log.h"
#include "resource.h"


extern char sectionName[];


const char* GameTypeName[] = 
{
	"",
	"Colony",
	"Auto Demo",
	"Tutorial",

	"Land Rush",
	"Space Race",
	"Resource Race",
	"Midas",
	"Last One Standing",
};



// Constructor
OPUNetGameSelectWnd::OPUNetGameSelectWnd()
{
	opuNetTransportLayer = nullptr;
	timer = 0;
	searchTickCount = SearchTickInterval - 1;	// Broadcast right away
	joiningGame = nullptr;
	joinAttempt = 0;
	joinAttemptTickCount = 0;
	internalPort = 0;
	externalPort = 0;
	externalIp.S_un.S_addr = INADDR_ANY;
	bReceivedInternal = false;
	bTwoExternal = false;
	numEchoRequestsSent = 0;
	echoTick = EchoTickInterval - 1;	// Check external address right away
}

// Destructor
OPUNetGameSelectWnd::~OPUNetGameSelectWnd()
{
	ClearGamesList();
}


// DialogProc
int OPUNetGameSelectWnd::DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int controlId;
	UINT notifyCode;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		OnInit();
		return true;			// Let system call SetFocus

	case WM_TIMER:
		OnTimer();
		return true;			// Message processed

	case WM_COMMAND:
		// Get the controlId
		controlId = LOWORD(wParam);
		notifyCode = HIWORD(wParam);

		// Figure out what to do
		if (notifyCode == BN_CLICKED)
		{
			switch (controlId)
			{
			case IDC_SearchButton:
				OnClickSearch();
				return true;		// Message processed

			case IDC_JoinButton:
				OnClickJoin();
				return true;		// Message processed

			case IDC_CreateButton:
				OnClickCreate();
				return true;		// Message processed

			case IDC_CancelButton:
				OnClickCancel();
				return true;		// Message processed
			}
		}
		break;

	case WM_NOTIFY:
		// Get the controlId
		controlId = wParam;
		notifyCode = ((NMHDR*)lParam)->code;

		if ((controlId == IDC_GamesList) && (notifyCode == NM_DBLCLK))
		{
			OnClickJoin();
			return true;			// Message processed
		}
		break;

	case WM_DESTROY:
		OnDestroy();
		return false;			// Should return 0 for this message
	case WM_NCDESTROY:
		hWnd = nullptr;
		return false;			// Should return 0 for this message
	};

	return false;				// Message not processed
};


void OPUNetGameSelectWnd::OnInit()
{
	// Initialize the data fields
	// --------------------------
	char buffer[MaxServerAddressLen];

	// Set the default player name
	GetPlayerName(buffer, false);
	SetDlgItemText(this->hWnd, IDC_PlayerName, buffer);

	// Setup the MaxPlayers combo box
	// Add the MaxPlayer options
	for (int i = 2; i <= 6; ++i)
	{
		// Add the number of player to the combo box
		scr_snprintf(buffer, sizeof(buffer), "%i", i);
		SendDlgItemMessage(this->hWnd, IDC_MaxPlayers, CB_ADDSTRING, 0, (LPARAM)buffer);
	}
	// Select the first item  (maxPlayers = 2)
	SendDlgItemMessage(this->hWnd, IDC_MaxPlayers, CB_SETCURSEL, (WPARAM)0, 0);

	// Setup the GameType combo box
	// Set the GameType display strings
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_ADDSTRING, 0, (LPARAM)"Last One Standing");
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_ADDSTRING, 0, (LPARAM)"Land Rush");
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_ADDSTRING, 0, (LPARAM)"Space Race");
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_ADDSTRING, 0, (LPARAM)"Resouce Race");
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_ADDSTRING, 0, (LPARAM)"Midas");
	// Set the GameType internal values
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_SETITEMDATA, 0, (LPARAM)-8);
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_SETITEMDATA, 1, (LPARAM)-4);
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_SETITEMDATA, 2, (LPARAM)-5);
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_SETITEMDATA, 3, (LPARAM)-6);
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_SETITEMDATA, 4, (LPARAM)-7);
	// Set the selected GameType
	SendDlgItemMessage(this->hWnd, IDC_GameType, CB_SETCURSEL, (WPARAM)0, 0);

	// Setup the ServerAddress combo box
	// Set the maximum string length
	SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_LIMITTEXT, (WPARAM)MaxServerAddressLen, 0);
	// Load the IPAddress history
	for (int i = 0; i < 10; ++i)
	{
		// Form the keyName string (number)
		scr_snprintf(buffer, sizeof(buffer), "%i", i);
		// Load the stored address
		config.GetString("IPHistory", buffer, buffer, sizeof(buffer), "");
		// Make sure a value was actually loaded
		if (buffer[0] != 0)
		{
			// Add the address to the combo box
			SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_ADDSTRING, 0, (LPARAM)buffer);
		}
	}
	// Select the first item
	SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_SETCURSEL, (WPARAM)0, 0);


	// Initialize the List View's Columns
	// ----------------------------------
	LVCOLUMN lvColumn;

	// Set valid struct fields
	lvColumn.mask = LVCF_WIDTH | LVCF_TEXT;
	// Insert each column
	lvColumn.cx = 100;
	lvColumn.pszText = const_cast<char*>("Host");
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	lvColumn.cx = 100;
	lvColumn.pszText = const_cast<char*>("Game Type");
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);
	lvColumn.cx = 57;
	lvColumn.pszText = const_cast<char*>("# Players");
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 2, (LPARAM)&lvColumn);
	lvColumn.cx = 70;
	lvColumn.pszText = const_cast<char*>("IP");
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 3, (LPARAM)&lvColumn);
	lvColumn.cx = 42;
	lvColumn.pszText = const_cast<char*>("Port");
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 4, (LPARAM)&lvColumn);
	lvColumn.cx = 40;
	lvColumn.pszText = const_cast<char*>("Ping");
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 5, (LPARAM)&lvColumn);
	
	// Turn on full row select in the list view
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);



	// Initialize Network objects
	InitNetLayer();


	// Create a timer
	// --------------
	timer = SetTimer(this->hWnd, 0, timerInterval, nullptr);
}


void OPUNetGameSelectWnd::InitNetLayer()
{
	// Initialize Network objects
	// --------------------------

	// Create NetTransportLayer
	opuNetTransportLayer = OPUNetTransportLayer::Create();
	// Check for errors
	if (opuNetTransportLayer == nullptr)
	{
		// Error creating the transport layer
		EndDialog(this->hWnd, true);		// bCancel = true
		return;								// Abort
	}

	// Set the global NetTransportLayer object pointer
	app.netTLayer = opuNetTransportLayer;
}


bool OPUNetGameSelectWnd::InitGurManager()
{
	// Check if the Guaranteed Send Layer already exists
	if (app.gurManager != nullptr)
	{
		// Already exists. Delete the old one first
		delete app.gurManager;
	}

	// Create the Guaranteed Send Layer
	app.gurManager = new GurManager();
	// Check for errors
	if (app.gurManager == nullptr)
	{
		// Failed to create the Guaranteed Send Layer. Inform User
		SetStatusText("Out of memory.  Could not create Guaranteed Send Layer.");
		return false;			// Abort
	}
	// Initialize the Guaranteed Send Layer
	const int errorCode = app.gurManager->Initialize(opuNetTransportLayer);
	// Check for errors
	if (errorCode == 0)
	{
		// Failed to initialize. Inform user
		SetStatusText("Failed to Initialize the Guaranteed Send Layer");
		return false;			// Abort
	}

	return true;		// Success
}


void OPUNetGameSelectWnd::CleanupGurManager()
{
	if (opuNetTransportLayer->GetNumPlayers() > 0)
	{
		// Send a Quit message
		app.NetShutdown(true);

		// Reinitialize the network layer
		InitNetLayer();
	}
	else
	{
		// Release the GurManager
		delete app.gurManager;
		app.gurManager = nullptr;
	}
}


void OPUNetGameSelectWnd::ClearGamesList()
{
	// Prepare a LVITEM struct
	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iSubItem = 0;

	// Get the number of list items to delete
	const int itemCount = SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEMCOUNT, 0, 0);

	// Release all the games info
	for (int i = 0; i < itemCount; ++i)
	{
		// Get the item data
		item.iItem = i;
		if (SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEM, 0, (LPARAM)&item))
		{
			// Retrieve the HostedGameInfo pointer
			 HostedGameInfo* hostedGameInfo = (HostedGameInfo*)item.lParam;
			// Make sure the parameter was set
			if (hostedGameInfo != nullptr) {
				delete hostedGameInfo;	// Free the memory
			}
		}
	}

	// Clear the list view
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_DELETEALLITEMS, 0, 0);
}


void OPUNetGameSelectWnd::AddServerAddress(const char* address)
{
	// Check if the address already exists in the combo box
	const int index = SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_FINDSTRINGEXACT, 1, (LPARAM)address);
	if (index != CB_ERR)
	{
		// Remove the old address
		SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_DELETESTRING, index, 0);
	}

	// Add the address to the combo box
	SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_INSERTSTRING, 0, (LPARAM)address);

	// Make sure the new item is selected
	SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_SETCURSEL, 0, 0);
}


void OPUNetGameSelectWnd::SetStatusText(const char* text)
{
	SendDlgItemMessage(this->hWnd, IDC_StatusBar, WM_SETTEXT, 0, (LPARAM)text);
}


void OPUNetGameSelectWnd::OnDestroy()
{
	// Kill the timer
	if (timer != 0) {
		KillTimer(this->hWnd, timer);
	}

	// Save the PlayerName
	char keyValue[MaxServerAddressLen];
	GetDlgItemText(this->hWnd, IDC_PlayerName, keyValue, sizeof(keyValue));
	config.SetString("Game", "Name", keyValue);

	// Save the ServerAddress list
	for (int i = 0; i < 10; ++i)
	{
		// Form the keyName string (number)
		char keyName[16];
		scr_snprintf(keyName, sizeof(keyName), "%i", i);

		// Get the Server Address
		const int retVal = SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_GETLBTEXT, (WPARAM)i, (LPARAM)keyValue);
		// Check for errors
		if (retVal != CB_ERR)
		{
			// Save the string to the file
			config.SetString("IPHistory", keyName, keyValue);
		}
		else
		{
			// Remove the string from the file
			config.SetString("IPHistory", keyName, nullptr);
		}
	}

	// Clear the games list
	ClearGamesList();
}


void OPUNetGameSelectWnd::OnTimer()
{
	// Make sure a network object exists
	if (opuNetTransportLayer == nullptr) {
		return;
	}

	// Periodically search for games
	searchTickCount++;
	if (searchTickCount >= SearchTickInterval)
	{
		// Reset the tick count
		searchTickCount = 0;

		// First check a game server for games (if info is setup)
		char addrString[128];
		opuNetTransportLayer->GetGameServerAddressString(addrString, sizeof(addrString));
		if (addrString[0] != 0)
		{
			// Check the game server for a list of games
			opuNetTransportLayer->SearchForGames(addrString, DefaultGameServerPort);
		}
		else
		{
			// Game server not available. Broadcast a search query  (Broadcast to LAN)
			opuNetTransportLayer->SearchForGames(nullptr, config.GetInt(sectionName, "ClientPort", DefaultClientPort));
		}
	}

	if ((joinAttempt > 0) && (joiningGame != nullptr))
	{
		joinAttemptTickCount++;
		if (joinAttemptTickCount >= JoinAttemptInterval)
		{
			// Reset the tick count
			joinAttemptTickCount = 0;

			if (joinAttempt > MaxJoinAttempt)
			{
				joinAttempt = 0;
				joiningGame = nullptr;

				SetStatusText("Game join failed");
			}
			else
			{
				joinAttempt++;
				// Resend the Join request
				opuNetTransportLayer->JoinGame(*joiningGame, password);
			}
		}
	}

	if ((externalPort == 0) && (numEchoRequestsSent < MaxEchoAttempt))
	{
		echoTick++;
		if (echoTick >= EchoTickInterval)
		{
			if (numEchoRequestsSent == 0)
			{
				internalPort = opuNetTransportLayer->GetPort();
			}

			// Reset the tick count
			echoTick = 0;
			numEchoRequestsSent++;

			// Request external address
			opuNetTransportLayer->GetExternalAddress();
		}
	}

	// Check for network replies
	Packet packet;
	while (opuNetTransportLayer->Receive(packet))
	{
		// Process the packet
		OnReceive(packet);
	}
}


void OPUNetGameSelectWnd::OnReceive(Packet &packet)
{
	// Make sure the packet is of the correct format
	if (packet.header.type != 1) {
		return;
	}

	// Determine which message type was received
	switch(packet.tlMessage.tlHeader.commandType)
	{
	case tlcHostedGameSearchReply: {		// [Custom format]
		// Verify packet size
		if (packet.header.sizeOfPayload != sizeof(HostedGameSearchReply)) {
			return;						// Discard packet
		}
		// Check the game identifier
		if (packet.tlMessage.searchReply.gameIdentifier != gameIdentifier) {
			return;						// Discard Packet
		}

		// Check if we already know about this game
		// ----------------------------------------
		// Prepare a LVITEM struct
		LVITEM item;
		item.mask = LVIF_PARAM;
		item.iSubItem = 0;

		// Search the list of games
		HostedGameInfo* hostedGameInfo;
		const int gameCount = SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEMCOUNT, 0, 0);
		for (int i = 0; i < gameCount; ++i)
		{
			// Specify exact item to retrieve
			item.iItem = i;
			// Get the item data
			if (SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEM, 0, (LPARAM)&item))
			{
				// Retrieve the HostedGameListItem pointer
				hostedGameInfo = (HostedGameInfo*)item.lParam;
				// Make sure we have a valid pointer
				if (hostedGameInfo != nullptr)
				{
					// Check if it's the same game
					//if (hostedGameInfo->sessionIdentifier == packet.tlMessage.searchReply.sessionIdentifier)
					// Check if it's the same host
					if (memcmp(&hostedGameInfo->address, &packet.tlMessage.searchReply.hostAddress, sizeof(sockaddr_in)) == 0)
					{
						// Matching game found. Update game info
						hostedGameInfo->createGameInfo = packet.tlMessage.searchReply.createGameInfo;
						hostedGameInfo->sessionIdentifier = packet.tlMessage.searchReply.sessionIdentifier;
						hostedGameInfo->ping = timeGetTime() - packet.tlMessage.searchReply.timeStamp;
						// Update the display
						SetGameListItem(i, hostedGameInfo);
						return;					// Packet handled
					}
				}
			}
		}

		// New hosted game found 
		// ---------------------
		// Allocate space to store info
		hostedGameInfo = new HostedGameInfo;
		// Check for allocation errors
		if (hostedGameInfo == nullptr)
		{
			// Out of memory. Inform user
			SetStatusText("Out of memory");
			return;						// Abort
		}

		// Copy the packet info
		hostedGameInfo->createGameInfo = packet.tlMessage.searchReply.createGameInfo;
		hostedGameInfo->ping = timeGetTime() - packet.tlMessage.searchReply.timeStamp;
		hostedGameInfo->sessionIdentifier = packet.tlMessage.searchReply.sessionIdentifier;
		hostedGameInfo->address = packet.tlMessage.searchReply.hostAddress;

		// Add a new List Item to the List View control (Games List)
		SetGameListItem(-1, hostedGameInfo);

		return;							// Packet handled
	}
	case tlcJoinGranted:		// [Fall through]
	case tlcJoinRefused:
		// Verify packet size
		if (packet.header.sizeOfPayload != sizeof(JoinReply)) {
			return;						// Discard packet
		}
		// Make sure we've requested to join a game
		if (joiningGame == nullptr)
		{
			// **DEBUG**
			SetStatusText("Unexpected Join reply received");
			return;						// Discard packet
		}
		// Check the session identifier
		if (packet.tlMessage.joinReply.sessionIdentifier != joiningGame->sessionIdentifier)
		{
			// **DEBUG**
			SetStatusText("Join reply received with wrong Session ID");
			return;						// Discard packet
		}

		// Check for JoinGranted
		if (packet.tlMessage.tlHeader.commandType == tlcJoinGranted)
		{
			// Join Accepted
			// -------------
			// Inform Transport Layer
			opuNetTransportLayer->OnJoinAccepted(packet);

			// Raise the event
			OnJoinAccepted();
		}
		else
		{
			// Session full. Inform User
			SetStatusText("Join Failed:  The requested game is full");
		}

		// Reset joining game status
		joiningGame = nullptr;
		return;							// Packet handled
	case tlcEchoExternalAddress:
		// Verify packet size
		if (packet.header.sizeOfPayload != sizeof(EchoExternalAddress)) {
			return;						// Discard packet
		}
		// Verify packet came from game server **TODO**

		// Check where the information came from
		if (packet.tlMessage.echoExternalAddress.replyPort == internalPort)
		{
			bReceivedInternal = true;
		}
		// Check if we've received a different external port than previous
		if (ntohs(packet.tlMessage.echoExternalAddress.addr.sin_port) != externalPort)
		{
			if (externalPort != 0)
			{
				bTwoExternal = true;	// Received two distinct external ports
			}
		}
		// Record external information
		externalIp = packet.tlMessage.echoExternalAddress.addr.sin_addr;
		if ((externalPort == 0) || (externalPort == internalPort))
		{
			externalPort = ntohs(packet.tlMessage.echoExternalAddress.addr.sin_port);
		}

		// Confine scope of `std::string text` to within a single switch case
		{
			// Build new net info text string
			std::string text = "External IP: " + FormatIP4Address(externalIp.s_addr) + ":" + std::to_string(externalPort);
			// Check if internal address received
			if (bReceivedInternal)
			{
				// Not quite true, since internal port might be random (no hosting, but possibly still open)
				//text += " (Direct Host Capable)";
			}
			if (bTwoExternal)
			{
				text += "\nWarning: Address and Port-Dependent Mapping detected\nYou may have difficulty joining games.";
			}
			// Update net info text
			SendDlgItemMessage(this->hWnd, IDC_NetInfo, WM_SETTEXT, 0, (long)text.c_str());
		}

		return;							// Packet handled
	default:  // Silence warnings about unused enumeration value in switch
		break;
	}
}


void OPUNetGameSelectWnd::SetGameListItem(int index, HostedGameInfo* hostedGameInfo)
{
	char buffer[32];

	// Fill in new List Item fields
	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_PARAM;
	item.iItem = index;
	item.iSubItem = 0;
	item.pszText = hostedGameInfo->createGameInfo.gameCreatorName;
	item.lParam = (LPARAM)hostedGameInfo;

	// Check if we are adding a new item
	if (index < 0)
	{
		// Get the number of List Items (so we can add to the end of the list)
		item.iItem = SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEMCOUNT, 0, 0);
		// Add the new List Item to the GamesList
		index = SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTITEM, 0, (LPARAM)&item);
	}
	else
	{
		// Update the Game Creator Name column of an existing item
		SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETITEM, 0, (LPARAM)&item);
	}

	// Fill in the Sub-Items
	item.mask = LVIF_TEXT;
	item.iItem = index;
	item.pszText = buffer;
	// Game Type
	item.iSubItem = 1;
	int i = -(hostedGameInfo->createGameInfo.startupFlags.missionType);
	if ((i < 0) || (i > 8)) {
		i = 0;
	}
	scr_snprintf(buffer, sizeof(buffer), "%s", GameTypeName[i]);
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETITEM, 0, (LPARAM)&item);
	// Num Players
	item.iSubItem = 2;
	scr_snprintf(buffer, sizeof(buffer), "%i", hostedGameInfo->createGameInfo.startupFlags.maxPlayers);
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETITEM, 0, (LPARAM)&item);
	// IP address
	item.iSubItem = 3;
	in_addr ip = hostedGameInfo->address.sin_addr;
	scr_snprintf(buffer, sizeof(buffer), "%i.%i.%i.%i", ip.S_un.S_un_b.s_b1, ip.S_un.S_un_b.s_b2, ip.S_un.S_un_b.s_b3, ip.S_un.S_un_b.s_b4);
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETITEM, 0, (LPARAM)&item);
	// Port
	item.iSubItem = 4;
	scr_snprintf(buffer, sizeof(buffer), "%i", ntohs(hostedGameInfo->address.sin_port));
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETITEM, 0, (LPARAM)&item);
	// Ping
	item.iSubItem = 5;
	scr_snprintf(buffer, sizeof(buffer), "%i", hostedGameInfo->ping);
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETITEM, 0, (LPARAM)&item);
}


void OPUNetGameSelectWnd::OnJoinAccepted()
{
	// Stop the update timer
	KillTimer(this->hWnd, 0);
	timer = 0;


	SetStatusText("Join accepted");


	// Initialize the Guaranteed Send Layer
	int errorCode = InitGurManager();
	// Check for errors
	if (errorCode == 0)
	{
		return;		// Abort
	}


	SetStatusText("");


	// Get the host player name
	int hostPlayerNetID = app.netTLayer->GetHostPlayerNetID();
	// Get the Player Name.
	char playerName[16];
	SendDlgItemMessage(this->hWnd, IDC_PlayerName, WM_GETTEXT, sizeof(playerName), (LPARAM)&playerName);


	// Show Pre-Game Setup window
	MultiplayerPreGameSetupWnd multiplayerPreGameSetupWnd;
	errorCode = multiplayerPreGameSetupWnd.ShowJoinGame(playerName, hostPlayerNetID, 0);


	// Check if a game is starting
	if (errorCode != false)
	{
		// Exit to start the game
		EndDialog(this->hWnd, false);		// bCancel  [0 = Start, 1 = Cancel]
	}
	else
	{
		// Game cancelled. Restart the update timer
		timer = SetTimer(this->hWnd, 0, timerInterval, nullptr);
		searchTickCount = SearchTickInterval - 1;	// Broadcast right away

		// Send the player Quit message
		CleanupGurManager();
	}
}


void OPUNetGameSelectWnd::OnClickSearch()
{
	// Clear the current games list
	ClearGamesList();

	// Set the status text
	SetStatusText("Searching for games...");

	// Get the server address
	char serverAddress[MaxServerAddressLen];
	SendDlgItemMessage(this->hWnd, IDC_ServerAddress, WM_GETTEXT, (WPARAM)sizeof(serverAddress), (LPARAM)serverAddress);
	// Request games list from server
	const int errorCode = opuNetTransportLayer->SearchForGames(serverAddress, config.GetInt(sectionName, "ClientPort", DefaultClientPort));

	// Check if the request was successfully sent
	if (errorCode != 0)
	{
		// Save the host address
		AddServerAddress(serverAddress);
	}
	else
	{
		// Inform user of error
		SetStatusText("Error searching for games (check server address)");
	}
}


void OPUNetGameSelectWnd::OnClickJoin()
{
	// Get the game to join
	// --------------------
	// Prepare a LVITEM struct
	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iSubItem = 0;
	// Get the selected game to join
	item.iItem = SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETSELECTIONMARK, 0, 0);
	// Get the item data
	if (SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEM, 0, (LPARAM)&item)) {
		joiningGame = (HostedGameInfo*)item.lParam;	// Retrieve the HostedGameListItem pointer
	}
	else {
		joiningGame = nullptr;		// Clear joining game
	}

	// Check if we have a bad pointer
	if (joiningGame == nullptr)
	{
		// No game selected  (or selected item is not a game)
		SetStatusText("Please select a game to join");
		return;				// Abort
	}


	// Get the join password
	SendDlgItemMessage(this->hWnd, IDC_Password, WM_GETTEXT, sizeof(password), (LPARAM)password);


	// Set the status text
	SetStatusText("Sending Join request...");


	joinAttempt = 1;
	// Send the Join request
	opuNetTransportLayer->JoinGame(*joiningGame, password);
}


void OPUNetGameSelectWnd::OnClickCreate()
{
	char password[16];


	// Stop the update timer
	KillTimer(this->hWnd, 0);
	timer = 0;


	// Clear the status text
	SetStatusText("");


	// Get the game host values
	SendDlgItemMessage(this->hWnd, IDC_Password, WM_GETTEXT, sizeof(password), (LPARAM)password);
	const int maxPlayers = SendDlgItemMessage(this->hWnd, IDC_MaxPlayers, CB_GETCURSEL, 0, 0) + 2;
	const int gameTypeIndex = SendDlgItemMessage(this->hWnd, IDC_GameType, CB_GETCURSEL, 0, 0);
	const int gameType = SendDlgItemMessage(this->hWnd, IDC_GameType, CB_GETITEMDATA, (WPARAM)gameTypeIndex, 0);

	// Setup HostGameParameters
	HostGameParameters hostGameParameters;
	SendDlgItemMessage(this->hWnd, IDC_PlayerName, WM_GETTEXT, sizeof(hostGameParameters.gameCreatorName), (LPARAM)&hostGameParameters.gameCreatorName);
	hostGameParameters.startupFlags.maxPlayers = maxPlayers;
	hostGameParameters.startupFlags.missionType = gameType;


	// Try to Host
	int errorCode = opuNetTransportLayer->HostGame(config.GetInt(sectionName, "HostPort", DefaultClientPort), password, hostGameParameters.gameCreatorName, maxPlayers, gameType);
	// Check for errors
	if (errorCode == false)
	{
		// Error binding to server listen port. Inform user
		SetStatusText("Could not Bind to the server port.");

		return;
	}


	// Initialize the Guaranteed Send Layer
	errorCode = InitGurManager();
	// Check for errors
	if (errorCode == 0)
	{
		return;		// Abort
	}

	
	// Show Pre-Game Setup window
	MultiplayerPreGameSetupWnd multiplayerPreGameSetupWnd;
	errorCode = multiplayerPreGameSetupWnd.ShowHostGame(hostGameParameters);


	// Check if a game is starting
	if (errorCode != 0)
	{
		// Exit to start the game
		EndDialog(this->hWnd, false);		// bCancel  [0 = Start, 1 = Cancel]
	}
	else
	{
		// Game cancelled
		// --------------
		SetStatusText("Cancelling game...");

		// Cancel the game (inform other players if needed)
		CleanupGurManager();

		SetStatusText("Game Cancelled");

		// Restart the update timer
		timer = SetTimer(this->hWnd, 0, timerInterval, nullptr);
		searchTickCount = SearchTickInterval - 1;	// Broadcast right away
	}
}


void OPUNetGameSelectWnd::OnClickCancel()
{
	// Just quit without sending a Quit message
	app.NetShutdown(false);

	EndDialog(this->hWnd, true);			// 0 = Start, 1 = Cancel
}
