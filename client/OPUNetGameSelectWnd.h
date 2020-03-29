#include "OPUNetTransportLayer.h"
#include <OP2Internal.h>

using namespace OP2Internal;

const int MaxServerAddressLen = 128;
const int timerInterval = 50;
const int SearchTickInterval = 60;
const int JoinAttemptInterval = 20;
const int MaxJoinAttempt = 3;
const int EchoTickInterval = 20;
const int MaxEchoAttempt = 3;


class OPUNetGameSelectWnd : public IDlgWnd
{
public:
	virtual ~OPUNetGameSelectWnd() override;
	virtual int DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	OPUNetGameSelectWnd();

private:
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
	void OnReceiveHostedGameSearchReply(Packet& packet);
	void OnReceiveJoinGranted(Packet& packet);
	void OnReceiveJoinRefused(Packet& packet);
	bool OnReceiveJoin(Packet& packet);
	void OnReceiveEchoExternalAddress(Packet& packet);
	void OnJoinAccepted();

	// Member functions
	void InitNetLayer();
	bool InitGurManager();
	void CleanupGurManager();
	void ClearGamesList();
	void SetGameListItem(int itemIndex, HostedGameInfo* hostedGameInfo);
	void AddServerAddress(const char* address);
	void SetStatusText(const char* text);

	void CreateServerAddressToolTip();

	OPUNetTransportLayer* opuNetTransportLayer = nullptr;
	UINT_PTR timer = 0;
	UINT searchTickCount = SearchTickInterval - 1;	// Broadcast right away
	HostedGameInfo* joiningGame = nullptr;
	char joinRequestPassword[16] = { };
	UINT joinAttempt = 0;
	UINT joinAttemptTickCount = 0;
	Port internalPort = 0;
	Port externalPort = 0;
	in_addr externalIp;
	bool bReceivedInternal = false;
	bool bTwoExternal = false;
	UCHAR numEchoRequestsSent = 0;
	UCHAR echoTick = EchoTickInterval - 1;	// Check external address right away
};
