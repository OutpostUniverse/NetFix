#include <OP2Internal.h>
using namespace OP2Internal;

class OPUNetTransportLayer;
struct HostedGameInfo;

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

private:
	OPUNetTransportLayer* opuNetTransportLayer;
	UINT_PTR timer;
	UINT searchTickCount;
	HostedGameInfo* joiningGame;
	char joinRequestPassword[16];
	UINT joinAttempt;
	UINT joinAttemptTickCount;
	USHORT internalPort;
	USHORT externalPort;
	in_addr externalIp;
	bool bReceivedInternal;
	bool bTwoExternal;
	UCHAR numEchoRequestsSent;
	UCHAR echoTick;
};
