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

#include "OPUNetGameSelectWnd.h"
#include "OPUNetTransportLayer.h"
#include "resource.h"


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
	opuNetTransportLayer = 0;
	timer = 0;
	searchTickCount = SearchTickInterval - 1;	// Broadcast right away
	joiningGame = 0;
}

// Destructor
OPUNetGameSelectWnd::~OPUNetGameSelectWnd()
{
	// **TODO** Release list of hosted games
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
		hWnd = 0;
		return false;			// Should return 0 for this message
	};

	return false;				// Message not processed
};


void OPUNetGameSelectWnd::OnInit()
{
	// Initialize the data fields
	// --------------------------
	int i;
	char buffer[MaxServerAddressLen];

	// Set the default player name
	GetPlayerName(buffer, false);
	SetDlgItemText(this->hWnd, IDC_PlayerName, buffer);

	// Setup the MaxPlayers combo box
	// Add the MaxPlayer options
	for (i = 2; i <= 6; i++)
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
	for (i = 0; i < 10; i++)
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
	lvColumn.pszText = "Host";
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	lvColumn.cx = 100;
	lvColumn.pszText = "Game Type";
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);
	lvColumn.cx = 57;
	lvColumn.pszText = "# Players";
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 2, (LPARAM)&lvColumn);
	lvColumn.cx = 70;
	lvColumn.pszText = "IP";
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 3, (LPARAM)&lvColumn);
	lvColumn.cx = 42;
	lvColumn.pszText = "Port";
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 4, (LPARAM)&lvColumn);
	lvColumn.cx = 40;
	lvColumn.pszText = "Ping";
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_INSERTCOLUMN, 5, (LPARAM)&lvColumn);
	
	// Turn on full row select in the list view
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);



	// Initialize Network objects
	InitNetLayer();


	// Create a timer
	// --------------
	timer = SetTimer(this->hWnd, 0, timerInterval, 0);
}


void OPUNetGameSelectWnd::InitNetLayer()
{
	// Initialize Network objects
	// --------------------------

	// Create NetTransportLayer
	opuNetTransportLayer = OPUNetTransportLayer::Create();
	// Check for errors
	if (opuNetTransportLayer == 0)
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
	int errorCode;

	// Check if the Guaranteed Send Layer already exists
	if (app.gurManager != 0)
	{
		// Already exists. Delete the old one first
		delete app.gurManager;
	}

	// Create the Guaranteed Send Layer
	app.gurManager = new GurManager();
	// Check for errors
	if (app.gurManager == 0)
	{
		// Failed to create the Guaranteed Send Layer. Inform User
		SetStatusText("Out of memory.  Could not create Guaranteed Send Layer.");
		return false;			// Abort
	}
	// Initialize the Guaranteed Send Layer
	errorCode = app.gurManager->Initialize(opuNetTransportLayer);
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
		app.gurManager = 0;
	}
}


void OPUNetGameSelectWnd::ClearGamesList()
{
	int i;
	int numItems;
	LVITEM item;
	HostedGameInfo* hostedGameInfo;

	// Prepare a LVITEM struct
	item.mask = LVIF_PARAM;
	item.iSubItem = 0;

	// Get the number of list items to delete
	numItems = SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEMCOUNT, 0, 0);

	// Release all the games info
	for (i = 0; i < numItems; i++)
	{
		// Get the item data
		item.iItem = i;
		if (SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEM, 0, (LPARAM)&item))
		{
			// Retrieve the HostedGameInfo pointer
			hostedGameInfo = (HostedGameInfo*)item.lParam;
			// Make sure the parameter was set
			if (hostedGameInfo != 0)
				delete hostedGameInfo;	// Free the memory
		}
	}

	// Clear the list view
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_DELETEALLITEMS, 0, 0);
}


void OPUNetGameSelectWnd::AddServerAddress(const char* address)
{
	int index;

	// Check if the address already exists in the combo box
	index = SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_FINDSTRINGEXACT, 1, (LPARAM)address);
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
	int i;
	int retVal;
	char keyName[16];
	char keyValue[MaxServerAddressLen];

	// Kill the timer
	if (timer != 0)
		KillTimer(this->hWnd, timer);

	// Save the PlayerName
	GetDlgItemText(this->hWnd, IDC_PlayerName, keyValue, sizeof(keyValue));
	config.SetString("Game", "Name", keyValue);

	// Save the ServerAddress list
	for (i = 0; i < 10; i++)
	{
		// Form the keyName string (number)
		scr_snprintf(keyName, sizeof(keyName), "%i", i);

		// Get the Server Address
		retVal = SendDlgItemMessage(this->hWnd, IDC_ServerAddress, CB_GETLBTEXT, (WPARAM)i, (LPARAM)keyValue);
		// Check for errors
		if (retVal != CB_ERR)
		{
			// Save the string to the file
			config.SetString("IPHistory", keyName, keyValue);
		}
		else
		{
			// Remove the string from the file
			config.SetString("IPHistory", keyName, 0);
		}
	}

	// Clear the games list
	ClearGamesList();
}


void OPUNetGameSelectWnd::OnTimer()
{
	Packet packet;

	// Make sure a network object exists
	if (opuNetTransportLayer == 0)
		return;			// Abort

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
			opuNetTransportLayer->SearchForGames(0, DefaultClientPort);
		}
	}

	// Check for network replies
	while (opuNetTransportLayer->Receive(&packet))
	{
		// Process the packet
		OnReceive(packet);
	}
}


void OPUNetGameSelectWnd::OnReceive(Packet &packet)
{
	int numGames;
	int i;
	HostedGameInfo* hostedGameInfo;
	LVITEM item;

	// Make sure the packet is of the correct format
	if (packet.header.type != 1)
		return;			// Abort

	// Determine which message type was received
	switch(packet.tlMessage.tlHeader.commandType)
	{
	case tlcHostedGameSearchReply:		// [Custom format]
		// Verify packet size
		if (packet.header.sizeOfPayload != sizeof(HostedGameSearchReply))
			return;						// Discard packet
		// Check the game identifier
		if (packet.tlMessage.searchReply.gameIdentifier != gameIdentifier)
			return;						// Discard Packet

		// Check if we already know about this game
		// ----------------------------------------
		// Prepare a LVITEM struct
		item.mask = LVIF_PARAM;
		item.iSubItem = 0;
		// Search the list of games
		numGames = SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEMCOUNT, 0, 0);
		for (i = 0; i < numGames; i++)
		{
			// Specify exact item to retrieve
			item.iItem = i;
			// Get the item data
			if (SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEM, 0, (LPARAM)&item))
			{
				// Retrieve the HostedGameListItem pointer
				hostedGameInfo = (HostedGameInfo*)item.lParam;
				// Make sure we have a valid pointer
				if (hostedGameInfo != 0)
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
		if (hostedGameInfo == 0)
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

	case tlcJoinGranted:		// [Fall through]
	case tlcJoinRefused:
		// Verify packet size
		if (packet.header.sizeOfPayload != sizeof(JoinReply))
			return;						// Discard packet
		// Make sure we've requested to join a game
		if (joiningGame == 0)
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
		joiningGame = 0;
		return;							// Packet handled
	}
}


void OPUNetGameSelectWnd::SetGameListItem(int index, HostedGameInfo* hostedGameInfo)
{
	int i;
	LVITEM item;
	char buffer[32];
	in_addr ip;

	// Fill in new List Item fields
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
	i = -(hostedGameInfo->createGameInfo.startupFlags.missionType);
	if ((i < 0) || (i > 8))
		i = 0;
	scr_snprintf(buffer, sizeof(buffer), "%s", GameTypeName[i]);
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETITEM, 0, (LPARAM)&item);
	// Num Players
	item.iSubItem = 2;
	scr_snprintf(buffer, sizeof(buffer), "%i", hostedGameInfo->createGameInfo.startupFlags.maxPlayers);
	SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_SETITEM, 0, (LPARAM)&item);
	// IP address
	item.iSubItem = 3;
	ip = hostedGameInfo->address.sin_addr;
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
	int errorCode;
	int hostPlayerNetID;
	char playerName[16];
	MultiplayerPreGameSetupWnd multiplayerPreGameSetupWnd;


	// Stop the update timer
	KillTimer(this->hWnd, 0);
	timer = 0;


	SetStatusText("Join accepted");


	// Initialize the Guaranteed Send Layer
	errorCode = InitGurManager();
	// Check for errors
	if (errorCode == 0)
	{
		return;		// Abort
	}


	SetStatusText("");


	// Get the host player name
	hostPlayerNetID = app.netTLayer->GetHostPlayerNetID();
	// Get the Player Name
	SendDlgItemMessage(this->hWnd, IDC_PlayerName, WM_GETTEXT, sizeof(playerName), (LPARAM)&playerName);


	// Show Pre-Game Setup window
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
		timer = SetTimer(this->hWnd, 0, timerInterval, 0);
		searchTickCount = SearchTickInterval - 1;	// Broadcast right away

		// Send the player Quit message
		CleanupGurManager();
	}
}


void OPUNetGameSelectWnd::OnClickSearch()
{
	int errorCode;
	char serverAddress[MaxServerAddressLen];

	// Clear the current games list
	ClearGamesList();

	// Set the status text
	SetStatusText("Searching for games...");

	// Get the server address
	SendDlgItemMessage(this->hWnd, IDC_ServerAddress, WM_GETTEXT, (WPARAM)sizeof(serverAddress), (LPARAM)serverAddress);
	// Request games list from server
	errorCode = opuNetTransportLayer->SearchForGames(serverAddress, DefaultClientPort);

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
	char password[16];
	LVITEM item;


	// Get the game to join
	// --------------------
	// Prepare a LVITEM struct
	item.mask = LVIF_PARAM;
	item.iSubItem = 0;
	// Get the selected game to join
	item.iItem = SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETSELECTIONMARK, 0, 0);
	// Get the item data
	if (SendDlgItemMessage(this->hWnd, IDC_GamesList, LVM_GETITEM, 0, (LPARAM)&item))
		joiningGame = (HostedGameInfo*)item.lParam;	// Retrieve the HostedGameListItem pointer
	else
		joiningGame = 0;		// Clear the joining game pointer

	// Check if we have a bad pointer
	if (joiningGame == 0)
	{
		// No game selected  (or selected item is not a game)
		SetStatusText("Please select a game to join");
		return;				// Abort
	}


	// Get the join password
	SendDlgItemMessage(this->hWnd, IDC_Password, WM_GETTEXT, sizeof(password), (LPARAM)password);


	// Set the status text
	SetStatusText("Sending Join request...");


	// Send the Join request
	opuNetTransportLayer->JoinGame(*joiningGame, password);
}


void OPUNetGameSelectWnd::OnClickCreate()
{
	// Host
	int errorCode;
	MultiplayerPreGameSetupWnd multiplayerPreGameSetupWnd;
	HostGameParameters hostGameParameters;
	char password[16];
	int maxPlayers;
	int gameType;
	int gameTypeIndex;


	// Stop the update timer
	KillTimer(this->hWnd, 0);
	timer = 0;


	// Clear the status text
	SetStatusText("");


	// Get the game host values
	SendDlgItemMessage(this->hWnd, IDC_Password, WM_GETTEXT, sizeof(password), (LPARAM)password);
	maxPlayers = SendDlgItemMessage(this->hWnd, IDC_MaxPlayers, CB_GETCURSEL, 0, 0) + 2;
	gameTypeIndex = SendDlgItemMessage(this->hWnd, IDC_GameType, CB_GETCURSEL, 0, 0);
	gameType = SendDlgItemMessage(this->hWnd, IDC_GameType, CB_GETITEMDATA, (WPARAM)gameTypeIndex, 0);

	// Setup HostGameParameters
	SendDlgItemMessage(this->hWnd, IDC_PlayerName, WM_GETTEXT, sizeof(hostGameParameters.gameCreatorName), (LPARAM)&hostGameParameters.gameCreatorName);
	hostGameParameters.startupFlags.maxPlayers = maxPlayers;
	hostGameParameters.startupFlags.missionType = gameType;


	// Try to Host
	errorCode = opuNetTransportLayer->HostGame(DefaultClientPort, password, hostGameParameters.gameCreatorName, maxPlayers, gameType);
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
	errorCode = multiplayerPreGameSetupWnd.ShowHostGame(&hostGameParameters);


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
		timer = SetTimer(this->hWnd, 0, timerInterval, 0);
		searchTickCount = SearchTickInterval - 1;	// Broadcast right away
	}
}


void OPUNetGameSelectWnd::OnClickCancel()
{
	// Just quit without sending a Quit message
	app.NetShutdown(false);

	EndDialog(this->hWnd, true);			// 0 = Start, 1 = Cancel
}
