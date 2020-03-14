#include <OP2Internal.h>
using namespace OP2Internal;

#include "OPUNetGameSelectWnd.h"
#include "resource.h"


extern HINSTANCE hInstance;



class OPUNetGameProtocol : public NetGameProtocol
{
public:
	// Virtual member functions

	virtual bool IsEnabled() override {
		return 1;
	};

	virtual bool DoStart() override {
		// Construct game select window
		OPUNetGameSelectWnd opuNetGameSelectWnd;
		// Use a base class reference to force indirect calls through the virtual function table
		IDlgWnd& protocolMainScreen = opuNetGameSelectWnd;

		// Enable Skinning
		app.mainWnd->PreCreateDlg();
		// Create the multiplayer setup dialog (this must go through the virtual function table)
		int retVal = protocolMainScreen.DoModal(MAKEINTRESOURCE(IDD_MultiSetupDialog), hInstance);
		// Disable skinning
		app.mainWnd->PostCreateDlg();

		// Return if the game was cancelled (or should be starting)
		return (retVal != 0);
	};

	virtual bool F1() override {
		return 1;
	};

	virtual const char* GetProtocolName() override {
		return "OPU.Net";
	};
};

