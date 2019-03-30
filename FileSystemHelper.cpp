#include "FileSystemHelper.h"
#include "op2ext.h"
#include <algorithm>

std::string GetOutpost2Directory()
{
	std::string buffer;

	// Pass size of 0 to get exact buffer size
	std::size_t bufferSize = GetGameDir_s(&buffer[0], 0);

	buffer.resize(bufferSize);
	GetGameDir_s(&buffer[0], bufferSize);

	// String concatinations (string1 + string2) will not provide expected results
	// if the std::string explicity has a null terminator
	buffer.erase(std::find(buffer.begin(), buffer.end(), '\0'), buffer.end());

	return buffer;
}
