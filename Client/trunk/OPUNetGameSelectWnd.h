#include "..\API\ForcedExports\ForcedExports.h"
using namespace OP2ForcedExport;

class OPUNetTransportLayer;
struct HostedGameInfo;

const int MaxServerAddressLen = 128;
const int timerInterval = 50;
const int SearchTickInterval = 60;
const int JoinAttemptInterval = 20;
const int MaxJoinAttempt = 3;


class OPUNetGameSelectWnd : public IDlgWnd
{
public:
	// Virtual functions
	virtual ~OPUNetGameSelectWnd();
	virtual int DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Constructor
	OPUNetGameSelectWnd();

private:
	// Member functions
	// ----------------

	// Button click handlers
	void OnClickSearch();
	void OnClickJoin();
	void OnClickCreate();
	void OnClickCancel();
	// Other event handlers
	void OnInit();
	void OnDestroy();
	void OnTimer();
	void OnReceive(Packet &packet);
	void OnJoinAccepted();
	// Member functions
	void InitNetLayer();
	bool InitGurManager();
	void CleanupGurManager();
	void ClearGamesList();
	void SetGameListItem(int itemIndex, HostedGameInfo* hostedGameInfo);
	void AddServerAddress(const char* address);
	void SetStatusText(const char* text);

private:
	// Member variables
	OPUNetTransportLayer* opuNetTransportLayer;
	UINT timer;
	UINT searchTickCount;
	HostedGameInfo* joiningGame;
	char password[16];
	UINT joinAttempt;
	UINT joinAttemptTickCount;
};


