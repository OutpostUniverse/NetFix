# Microsoft Developer Studio Project File - Name="OP2Script" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OP2Script - Win32 Release MinSize
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OP2Script.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OP2Script.mak" CFG="OP2Script - Win32 Release MinSize"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OP2Script - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinSize"
# PROP BASE Intermediate_Dir "ReleaseMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinSize"
# PROP Intermediate_Dir "ReleaseMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /D "OP2_NO_EXPORTS" /FAs /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 wsock32.lib user32.lib winmm.lib ole32.lib /nologo /base:"0x14000000" /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /map /debug
# Begin Target

# Name "OP2Script - Win32 Release MinSize"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CustomDialogScript.rc
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\OPUNetGameSelectWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\OPUNetTransportLayer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\OPUNetGameProtocol.h
# End Source File
# Begin Source File

SOURCE=.\OPUNetGameSelectWnd.h
# End Source File
# Begin Source File

SOURCE=.\OPUNetTransportLayer.h
# End Source File
# End Group
# Begin Group "API"

# PROP Default_Filter ""
# Begin Group "Forced Exports"

# PROP Default_Filter ""
# Begin Group "Network"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\API\ForcedExports\Network\GameNetLayer.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Network\GurManager.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Network\NetGameProtocol.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Network\NetGameProtocolTCPNetGameProtocol.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Network\NetProtocolManager.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Network\NetProtocolManagerSIGSNetProtocolManager.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Network\NetTransportLayer.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Network\Packet.h
# End Source File
# End Group
# Begin Group "Resource Management"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\API\ForcedExports\ResourceManagement\AdaptiveHuffmanTree.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\ResourceManagement\CConfig.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\ResourceManagement\MusicManager.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\ResourceManagement\PrtGraphics.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\ResourceManagement\SoundEffectsManager.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\ResourceManagement\StreamIO.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\ResourceManagement\TechFileParser.h
# End Source File
# End Group
# Begin Group "User Interface"

# PROP Default_Filter ""
# Begin Group "UICommand"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UICommand\UICommand.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UICommand\UICommandAttackCommand.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UICommand\UICommandBuildCommand.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UICommand\UICommandMouseCommand.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UICommand\UICommandProduceCommand.h
# End Source File
# End Group
# Begin Group "UIElement"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElement.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementButton.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementButtonGraphicalButton.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementButtonGraphicalButtonBayButton.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementButtonGraphicalButtonMiscButton.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementButtonGraphicalButtonUICommandButton.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementButtonGraphicalButtonViewButton.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementButtonGraphicalButtonViewButtonBuildButton.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementButtonGraphicalButtonViewButtonReportPageButton.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\UIElement\UIElementListBox.h
# End Source File
# End Group
# Begin Group "CommandPaneView"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\CommandPaneView\CommandPaneView.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\CommandPaneView\CommandPaneViewBuildingStorageBaysView.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\CommandPaneView\CommandPaneViewReportView.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\CommandPaneView\CommandPaneViewReportViewLabView.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\CommandPaneView\CommandPaneViewUnitView.h
# End Source File
# End Group
# Begin Group "IWnd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWnd.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndIDlgWnd.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndIDlgWndMessageBoxWnd.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndIDlgWndMultiplayerPreGameSetupWnd.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndPane.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndPaneCommand.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndPaneDetail.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndPaneMiniMap.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndTFrame.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\IWnd\IWndTFrameDans_RULE_UIFrame.h
# End Source File
# End Group
# Begin Group "Filter"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Filter\Filter.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Filter\FilterGroupFilter.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Filter\FilterGroupFilterDetailPaneFilter.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Filter\FilterHotKeyFilter.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Filter\FilterNode.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Filter\FilterSubFilter.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Filter\FilterSubFilterMouseCommandFilter.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Filter\FilterUIElementFilter.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\GFXSurface.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\GFXSurfaceGFXCDSSurface.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\GFXSurfaceGFXCDSSurfaceGFXClippedSurface.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\GFXSurfaceGFXMemSurface.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\SelectedUnits.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\UserInterface\Viewport.h
# End Source File
# End Group
# Begin Group "Game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\API\ForcedExports\Game\LevelDLL.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\Map.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\MiniMap.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\Morale.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\PathFinder.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\Player.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\RandomNumberGenerator.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\Research.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\ScStub.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\ScStubCreator.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\Sheet.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\TApp.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\TerrainManager.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\TethysGame.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\Unit.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\UnitPlayerUnit.h
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\Game\UnitTypeInfo.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\API\ForcedExports\ForcedExports.asm
# Begin Custom Build
InputDir=\Documents and Settings\dstevens\My Documents\Coding\Outpost2\API\ForcedExports
InputPath=..\API\ForcedExports\ForcedExports.asm
InputName=ForcedExports

"$(InputDir)\Build\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	NASMw -f win32 -o "$(InputDir)\Build\$(InputName).obj" "$(InputPath)"

# End Custom Build
# End Source File
# Begin Source File

SOURCE=..\API\ForcedExports\ForcedExports.h
# End Source File
# End Group
# End Group
# End Target
# End Project
