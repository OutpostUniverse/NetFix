#include "..\API\ForcedExports\ForcedExports.h"
using namespace OP2ForcedExport;

#include "OPUNetGameSelectWnd.h"
#include "resource.h"


extern HINSTANCE hInstance;



class OPUNetGameProtocol : public NetGameProtocol
{
public:
	// Member variables
	OPUNetGameSelectWnd opuNetGameSelectWnd;

public:
	// Virtual member functions
	virtual bool IsEnabled() {return 1;};
	virtual bool DoStart()
	{
		int retVal;

		// Enable Skinning
		app.mainWnd->PreCreateDlg();
		// Create the multiplayer setup dialog
		retVal = opuNetGameSelectWnd.DoModal(MAKEINTRESOURCE(IDD_MultiSetupDialog), hInstance);
		// Disable skinning
		app.mainWnd->PostCreateDlg();

		// Return if the game was cancelled (or should be starting)
		return (retVal != 0);
	};
	virtual bool F1() {return 1;};
	virtual const char* GetProtocolName() {return "OPU.Net";};
};

