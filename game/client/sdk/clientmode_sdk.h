#ifndef _INCLUDED_CLIENTMODE_SDK_H
#define _INCLUDED_CLIENTMODE_SDK_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>
#include "GameUI/igameui.h"

#include "ivmodemanager.h"

class CHudViewport;

//=========================================================
// >> ClientModeSDK
//=========================================================
class InClientMode : public ClientModeShared
{
public:
	DECLARE_CLASS( InClientMode, ClientModeShared );

	virtual void	Init();
	virtual void	Update();
	virtual void	InitWeaponSelectionHudElement() { return; }
	virtual void	InitViewport();
	virtual void	Shutdown();

	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	FireGameEvent( IGameEvent *event );

	virtual void	UpdatePostProcessingEffects();
	virtual void	DoPostScreenSpaceEffects( const CViewSetup *pSetup );

protected:
	C_PostProcessController *m_pCurrentPostProcessController;
};

extern IClientMode *GetClientModeNormal();
extern InClientMode* GetInClientMode();

extern vgui::HScheme g_hVGuiCombineScheme;

//=========================================================
// >> InClientModeFullscreen
//=========================================================
class InClientModeFullscreen : public InClientMode
{
public:
	DECLARE_CLASS_SIMPLE( InClientModeFullscreen, InClientMode );

	virtual void InitViewport();
	virtual void Init();

	void Shutdown()
	{
		
	}
};

//=========================================================
// >> CSDKModeManager
//=========================================================
class CSDKModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) { }
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) { }
};

//=========================================================
// This is the viewport that contains all the hud elements
//=========================================================
class CHudViewport : public CBaseViewport
{
public:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		GetHud().InitColors( pScheme );
		SetPaintBackgroundEnabled( false );
	}

	virtual void CreateDefaultPanels() { };
};

//=========================================================
// >> CFullscreenViewport
//=========================================================
class CFullscreenViewport : public CHudViewport
{
private:
	DECLARE_CLASS_SIMPLE( CFullscreenViewport, CHudViewport );

private:
	virtual void InitViewportSingletons()
	{
		SetAsFullscreenViewportInterface();
	}
};

#endif // _INCLUDED_CLIENTMODE_SDK_H
