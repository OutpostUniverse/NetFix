#include "PlayerNetID.h"
#include <windows.h>
#include <stdexcept>

namespace PlayerNetID
{
	int GetPlayerIndex(int playerNetID)
	{
		int playerIndex = playerNetID & 7;

		if (playerIndex > 5) {
			throw std::runtime_error("Maximum allowed player index is 5");
		}

		return playerIndex;
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
