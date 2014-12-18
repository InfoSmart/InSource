//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef VGUI_IN_MENU_H
#define VGUI_IN_MENU_H

#pragma once

#include "vgui_basehtml.h"

#include "GameUI/IGameUI.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/PHandle.h"
#include "convar.h"

#include "in_gameui_menu.h"
#include "in_gameui_hud.h"

class IGameClientExports;
class CCommand;

//=========================================================
// CGameUI
// >> La interfaz del usuario
//=========================================================
class CGameUI : public IGameUI
{
public:
	CGameUI();
	~CGameUI();

	virtual void Initialize( CreateInterfaceFn factory );
	virtual void CreateGameUI();
	virtual void PostInit();
	virtual void Connect( CreateInterfaceFn gameFactory );

	virtual void Start();
	virtual void Shutdown();
	virtual void RunFrame();

	virtual void OnGameUIActivated();
	virtual void OnGameUIHidden();

	virtual void OLD_OnConnectToServer( const char *game, int IP, int port );

	virtual void OnDisconnectFromServer( uint8 eSteamLoginFailure );
	virtual void OnConnectToServer2( const char *game, int IP, int connectionPort, int queryPort );
	
	virtual void OnDisconnectFromServer_OLD( uint8 eSteamLoginFailure, const char *username ) { }
	virtual void OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog );
	virtual void OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason );

	virtual bool UpdateProgressBar( float progress, const char *statusText );
	virtual bool SetShowProgressText( bool show );
	virtual void SetProgressLevelName( const char *levelName );
	virtual void SetProgressOnStart() { }

	virtual void SetLoadingBackgroundDialog( vgui::VPANEL panel );

	virtual void NeedConnectionProblemWaitScreen();
	virtual void ShowPasswordUI( char const *pchCurrentPW );

	//=========================================================

	virtual bool IsInLevel();
	virtual bool IsInMultiplayer();
	virtual bool IsConsoleUI();
	virtual bool IsLoading() { return m_bIsLoading; }

	virtual void PlayGameStartupSound();
	virtual bool FindPlatformDirectory( char *platformDir, int bufferSize );
	
	virtual void ActivateGameUI();
	virtual void HideGameUI();
	virtual void PreventEngineHideGameUI();

	virtual void SendConnectedToGameMessage();
	virtual void ValidateCDKey() { };

	virtual CGameUIPanelWeb *GetGameUI() { return m_nWebPanelUI; }

public:
	bool m_bActivatedUI : 1;
	bool m_bIsConsoleUI : 1;
	bool m_bTryingToLoadFriends : 1;
	bool m_bIsLoading : 1;

	int m_iPlayGameStartupSound;
	int m_iFriendsLoadPauseFrames;

	int m_iGameIP;
	int m_iGameConnectionPort;
	int m_iGameQueryPort;

	CreateInterfaceFn m_GameFactory;
	CGameUIPanelWeb *m_nWebPanelUI;
	CGameHUDWeb *m_nWebPanelHUD;

	char m_szPlatformDir[MAX_PATH];
};

// Purpose: singleton accessor
extern CGameUI &GameUI();

// expose client interface
extern IGameClientExports *GameClientExports();

#endif