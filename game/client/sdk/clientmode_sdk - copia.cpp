#include "cbase.h"
#include "clientmode_sdk.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "cdll_client_int.h"
#include "engine/IEngineSound.h"

#include "sdk_loading_panel.h"
#include "sdk_logo_panel.h"
#include "ivmodemanager.h"
#include "panelmetaclassmgr.h"
#include "nb_header_footer.h"

#include "viewpostprocess.h"

#include "c_in_player.h"

#include "input.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
//=========================================================

vgui::HScheme g_hVGuiCombineScheme = 0;
vgui::DHANDLE<CSDK_Logo_Panel> g_hLogoPanel;

static IClientMode *g_pClientMode[ MAX_SPLITSCREEN_PLAYERS ];
static CDllDemandLoader g_GameUI( "gameui" );
ClientModeSDK g_ClientModeNormal[ MAX_SPLITSCREEN_PLAYERS ];

#define SCREEN_FILE	"scripts/vgui_screens.txt"

//=========================================================
// Comandos
//=========================================================

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "75", FCVAR_USERINFO, "Sets the base field-of-view.", true, 1.0, true, 75.0 );

//=========================================================
//=========================================================
IClientMode *GetClientMode()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return g_pClientMode[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//=========================================================
// Inicia el sistema
//=========================================================
void ClientModeSDK::Init()
{
	BaseClass::Init();
	gameeventmanager->AddListener( this, "game_newmap", false );

	m_pCurrentPostProcessController = NULL;
}

//=========================================================
//=========================================================
void ClientModeSDK::Update()
{
	BaseClass::Update();

	// Actualizamos los efectos PostProcessing
	UpdatePostProcessingEffects();
}

//=========================================================
//=========================================================
void ClientModeSDK::InitViewport()
{
	//m_pViewport = new CHudViewport();
	//m_pViewport->Start( gameuifuncs, gameeventmanager );
}

//=========================================================
// Apaga el sistema
//=========================================================
void ClientModeSDK::Shutdown()
{
	/*if ( SDKBackgroundMovie() )
	{
		SDKBackgroundMovie()->ClearCurrentMovie();
	}

	DestroySDKLoadingPanel();

	if ( g_hLogoPanel.Get() )
		delete g_hLogoPanel.Get();*/
}

//=========================================================
// Prepara al Cliente para el inicio de un nivel
//=========================================================
void ClientModeSDK::LevelInit( const char *newmap )
{
	// Reiniciamos la luz de ambiente
	static ConVarRef mat_ambient_light_r( "mat_ambient_light_r" );
	static ConVarRef mat_ambient_light_g( "mat_ambient_light_g" );
	static ConVarRef mat_ambient_light_b( "mat_ambient_light_b" );

	if ( mat_ambient_light_r.IsValid() )
		mat_ambient_light_r.SetValue( "0" );

	if ( mat_ambient_light_g.IsValid() )
		mat_ambient_light_g.SetValue( "0" );

	if ( mat_ambient_light_b.IsValid() )
		mat_ambient_light_b.SetValue( "0" );

	BaseClass::LevelInit( newmap );

	// sdk: make sure no windows are left open from before
	//SDK_CloseAllWindows();

	// clear any DSP effects
	CLocalPlayerFilter filter;
	enginesound->SetRoomType( filter, 0 );
	enginesound->SetPlayerDSP( filter, 0, true );

	// TODO: Mover a un mejor lugar
	::input->CAM_ToFirstPerson();
}

//=========================================================
//=========================================================
void ClientModeSDK::LevelShutdown( void )
{
	BaseClass::LevelShutdown();

	// sdk:shutdown all vgui windows
	//SDK_CloseAllWindows();
}

//=========================================================
// >> CSDKModeManager
//=========================================================
class CSDKModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CSDKModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

//=========================================================
//=========================================================
void CSDKModeManager::Init()
{
	for ( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		g_pClientMode[ i ] = GetClientModeNormal();
	}

	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );
	//GetClientVoiceMgr()->SetHeadLabelOffset( 40 );
}

//=========================================================
//=========================================================
void CSDKModeManager::LevelInit( const char *newmap )
{
	for ( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelInit( newmap );
	}
}

//=========================================================
//=========================================================
void CSDKModeManager::LevelShutdown( void )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelShutdown();
	}
}

//=========================================================
//=========================================================
IClientMode *GetClientModeNormal()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//=========================================================
//=========================================================
ClientModeSDK* GetClientModeSDK()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//=========================================================
// these vgui panels will be closed at various times (e.g. when the level ends/starts)
//=========================================================
static char const *s_CloseWindowNames[] =
{
	"InfoMessageWindow",
	"SkipIntro",
};

//=========================================================
// This is the viewport that contains all the hud elements
//=========================================================
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		GetHud().InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual void CreateDefaultPanels() 
	{ 
		/* don't create any panels yet*/ 
	};
};

//=========================================================
// >> FullscreenSDKViewport
//=========================================================
class FullscreenSDKViewport : public CHudViewport
{
private:
	DECLARE_CLASS_SIMPLE( FullscreenSDKViewport, CHudViewport );

private:
	virtual void InitViewportSingletons()
	{
		SetAsFullscreenViewportInterface();
	}
};

//=========================================================
//=========================================================
class ClientModeSDKFullscreen : public	ClientModeSDK
{
	DECLARE_CLASS_SIMPLE( ClientModeSDKFullscreen, ClientModeSDK );
public:
	virtual void InitViewport()
	{
		// Skip over BaseClass!!!
		BaseClass::BaseClass::InitViewport();
		m_pViewport = new FullscreenSDKViewport();
		m_pViewport->Start( gameuifuncs, gameeventmanager );
	}
	virtual void Init()
	{
		CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();
		if ( gameUIFactory )
		{
			IGameUI *pGameUI = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL );
			if ( NULL != pGameUI )
			{
				// insert stats summary panel as the loading background dialog
				CSDK_Loading_Panel *pPanel = GSDKLoadingPanel();
				pPanel->InvalidateLayout( false, true );
				pPanel->SetVisible( false );
				pPanel->MakePopup( false );
				pGameUI->SetLoadingBackgroundDialog( pPanel->GetVPanel() );

				// add ASI logo to main menu
				CSDK_Logo_Panel *pLogo = new CSDK_Logo_Panel( NULL, "ASILogo" );
				vgui::VPANEL GameUIRoot = enginevgui->GetPanel( PANEL_GAMEUIDLL );
				pLogo->SetParent( GameUIRoot );
				g_hLogoPanel = pLogo;
			}		
		}

		// Skip over BaseClass!!!
		BaseClass::BaseClass::Init();

		// Load up the combine control panel scheme
		if ( !g_hVGuiCombineScheme )
		{
			g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme" );
			if (!g_hVGuiCombineScheme)
			{
				Warning( "Couldn't load combine panel scheme!\n" );
			}
		}
	}
	void Shutdown()
	{
		DestroySDKLoadingPanel();
		if (g_hLogoPanel.Get())
		{
			delete g_hLogoPanel.Get();
		}
	}
};

//=========================================================
//=========================================================
static ClientModeSDKFullscreen g_FullscreenClientMode;
IClientMode *GetFullscreenClientMode()
{
	return &g_FullscreenClientMode;
}

//=========================================================
//=========================================================
void ClientModeSDK::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();

	if ( Q_strcmp( "asw_mission_restart", eventname ) == 0 )
	{
		SDK_CloseAllWindows();
	}
	else if ( Q_strcmp( "game_newmap", eventname ) == 0 )
	{
		engine->ClientCmd("exec newmapsettings\n");
	}
	else
	{
		BaseClass::FireGameEvent(event);
	}
}

//=========================================================
// Close all ASW specific VGUI windows that the player might have open
//=========================================================
void ClientModeSDK::SDK_CloseAllWindows()
{
	SDK_CloseAllWindowsFrom(GetViewport());
}

//=========================================================
// recursive search for matching window names
//=========================================================
void ClientModeSDK::SDK_CloseAllWindowsFrom(vgui::Panel* pPanel)
{
	if (!pPanel)
		return;

	int num_names = NELEMS(s_CloseWindowNames);

	for (int k=0;k<pPanel->GetChildCount();k++)
	{
		Panel *pChild = pPanel->GetChild(k);
		if (pChild)
		{
			SDK_CloseAllWindowsFrom(pChild);
		}
	}

	// When VGUI is shutting down (i.e. if the player closes the window), GetName() can return NULL
	const char *pPanelName = pPanel->GetName();
	if ( pPanelName != NULL )
	{
		for (int i=0;i<num_names;i++)
		{
			if ( !strcmp( pPanelName, s_CloseWindowNames[i] ) )
			{
				pPanel->SetVisible(false);
				pPanel->MarkForDeletion();
			}
		}
	}
}

//=========================================================
//=========================================================
void ClientModeSDK::UpdatePostProcessingEffects()
{
	C_PostProcessController *pNewPostProcessController = NULL;
	C_IN_Player *pPlayer = C_IN_Player::GetLocalInPlayer();

	// Obtenemos el controlador del Jugador
	if ( pPlayer )
	{
		pNewPostProcessController = pPlayer->GetActivePostProcessController();
	}

	if ( !pNewPostProcessController )
		return;

	// Figure out new endpoints for parameter lerping
	if ( pNewPostProcessController != m_pCurrentPostProcessController )
	{
		/*m_LerpStartPostProcessParameters = m_CurrentPostProcessParameters;
		m_LerpEndPostProcessParameters = pNewPostProcessController ? pNewPostProcessController->m_PostProcessParameters : PostProcessParameters_t();
		m_pCurrentPostProcessController = pNewPostProcessController;

		float flFadeTime = pNewPostProcessController ? pNewPostProcessController->m_PostProcessParameters.m_flParameters[ PPPN_FADE_TIME ] : 0.0f;
		if ( flFadeTime <= 0.0f )
		{
			flFadeTime = 0.001f;
		}
		m_PostProcessLerpTimer.Start( flFadeTime );*/
	}

	SetPostProcessParams( &pNewPostProcessController->m_PostProcessParameters );
}

//=========================================================
//=========================================================
void ClientModeSDK::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{

}