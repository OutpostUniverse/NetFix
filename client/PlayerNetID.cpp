#include "PlayerNetID.h"
#include <windows.h>

namespace PlayerNetID
{
	int GetPlayerIndex(int playerNetID)
	{
		return playerNetID & 7;
	}

	int GetTimeStamp(int playerNetID)
	{
		return playerNetID & ~7;
	}

	int SetTimeStamp(int playerNetID, int newTimeStamp)
	{
		return GetPlayerIndex(playerNetID) | GetTimeStamp(newTimeStamp);
	}

	int SetCurrentTime(int playerNetID)
	{
		return SetTimeStamp(playerNetID, timeGetTime());
	}
}
