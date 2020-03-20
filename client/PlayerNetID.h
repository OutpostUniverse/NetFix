#pragma once

// PlayerNetID is a 32 bit bitfied used by OP2Intenral::NetTransportLayer
//  playerIndex : 3;
//  timeStamp : 29;
namespace PlayerNetID
{
	int GetPlayerIndex(int playerNetID);
	int GetTimeStamp(int playerNetID);

	int SetTimeStamp(int playerNetID, int newTimeStamp);
}
