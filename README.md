# NetFixClient

The NetFixClient is a module for Outpost 2 that provides an updated network layer to allow players to play from behind home routers.

This module is meant to be loaded by the [op2ext](https://github.com/OutpostUniverse/op2ext) module loader, using configuration from the `outpost2.ini` settings file.

This project can be used in conjunction with the [NetFixServer](https://github.com/OutpostUniverse/NetFixServer) game server project. The NetFixServer can be used to provide a game list, and introduce players by provided assistance with NAT traversal during game join. Typically, the NetFixServer runs on OPU servers, though you could try running your own with appropriate edits to the `outpost2.ini` settings.

Use of the NetFixServer is optional. The NetFixClient was designed to work even without an operational game server, albeit in a degraded mode. Without the server running, there is typically no automatic game list. Instead, users can enter the IP address of a game host to see and join their hosted game.

## NetFix In Game Menu Options

#### Common Info
 - **Name:** In game alias.
 - **Password:** Password required to join game's lobby.

#### Game Create Info
 - **Max Players:** Maximum number of players in game. Restricts from selecting missions with less than max player count.
 - **Game Type:** Restricts from selecting missions of different types.

#### Game Join Info
 - **Server Address:** The address to find NetFixServer or a specific host. Default is to leave blank and NetFixClient will search address listed in .ini file.
 - **Games:** A list of games being hosted on the NetFixServer. If an IP address not containing NetFixServer is used, will only display a single game intiated by the host.

#### Bottom Buttons  
 - **Search:** Sends a single request to NetFixServer for a list of games and waits for a responsse.
 - **Join:** Join the selected game.
 - **Create:** Create a new game using the Common Info and Game Create Info
 - **Cancel:** Return to the main menu.

### .ini settings
 - **Dll:** Relative path of the NetFix dll from the Outpost 2 executable
 - **GameServerAddr:** Default server address. Can contain an IP address or a URL.
 - **ProtocolIndex:** ???

## Known Limitations

If the game server is not operational, the host may need to setup port forwarding to host from behind a router. The [NetHelper](https://github.com/OutpostUniverse/NetHelper) project should be able to do this for you automatically with most home routers. Without port forwarding, nor a game server to introduce players, other players may be unable to see or join a hosted game.

Some routers have very restrictive filtering rules, which may prevent the NetFixClient from succeeding with NAT traversal. To start a game, all players must have a direct line of communication with each other. This direct communication between all players is established when the host starts the game. This is after all players have joined the game. Before game start, players are only in direct communication with the host. If any player has a particularly restrictive router, it may prevent the game from starting. Again, the NetHelper module should provide some assistance here.
