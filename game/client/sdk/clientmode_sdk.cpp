#include "cbase.h"
#include "clientmode_sdk.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "cdll_client_int.h"
#include "engine/IEngineSound.h"

#include "panelmetaclassmgr.h"
#include "nb_header_footer.h"

#include "viewpostprocess.h"
#include "input.h"

#include "c_in_player.h"
#include "weapon_inbase.h"

#include "physpropclientside.h"
#include "c_te_legacytempents.h"
#include "c_soundscape.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//====================================================================
//====================================================================

vgui::HScheme g_hVGuiCombineScheme = 0;
static CDllDemandLoader g_GameUI( "gameui" );

static IClientMode *g_pClientMode[ MAX_SPLITSCREEN_PLAYERS ];
InClientMode g_ClientModeNormal[ MAX_SPLITSCREEN_PLAYERS ];

#define SCREEN_FILE	"scripts/vgui_screens.txt"

static CSDKModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

static InClientModeFullscreen g_FullscreenClientMode;

//====================================================================
// Comandos
//====================================================================

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "75", FCVAR_USERINFO, "Sets the base field-of-view.", true, 1.0, true, 75.0 );

ConVar mat_sunrays_enable( "mat_sunrays_enable", "1", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar mat_fxaa_enable( "mat_fxaa_enable", "1", FCVAR_USERINFO | FCVAR_ARCHIVE );

//====================================================================
//====================================================================
IClientMode *GetClientMode()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return g_pClientMode[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//====================================================================
//====================================================================
IClientMode *GetClientModeNormal()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//====================================================================
//====================================================================
InClientMode* GetInClientMode()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//====================================================================
//====================================================================
IClientMode *GetFullscreenClientMode()
{
	return &g_FullscreenClientMode;
}

//====================================================================
// Inicia el sistema
//====================================================================
void InClientMode::Init()
{
	BaseClass::Init();

	gameeventmanager->AddListener( this, "game_newmap", false );
	gameeventmanager->AddListener( this, "game_round_restart", false );

	m_pCurrentPostProcessController = NULL;
}

//====================================================================
//====================================================================
void InClientMode::Update()
{
	BaseClass::Update();

	// Actualizamos los efectos PostProcessing
	UpdatePostProcessingEffects();
}

//====================================================================
// Inicializa el HUD
//====================================================================
void InClientMode::InitViewport()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

//====================================================================
// Apaga el sistema
//====================================================================
void InClientMode::Shutdown()
{

}

//====================================================================
// Prepara al Cliente para el inicio de un nivel
//====================================================================
void InClientMode::LevelInit( const char *newmap )
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

	// clear any DSP effects
	CLocalPlayerFilter filter;
	enginesound->SetRoomType( filter, 0 );
	enginesound->SetPlayerDSP( filter, 0, true );
}

//====================================================================
//====================================================================
void InClientMode::LevelShutdown( void )
{
	BaseClass::LevelShutdown();
}

//====================================================================
// El servidor ha llamado a un evento
//====================================================================
void InClientMode::FireGameEvent( IGameEvent *event )
{
	// Nombre del evento
	const char *eventname = event->GetName();

	// Nuevo mapa
	if ( Q_strcmp( "game_newmap", eventname ) == 0 )
	{
		engine->ClientCmd("exec newmapsettings\n");
	}
	// Reinicio de la ronda
	else if ( Q_strcmp( "game_round_restart", eventname ) == 0 )
	{
		// Recreamos todos los "prop_physics" en cliente
		C_PhysPropClientside::RecreateAll();

		// Limpiamos las entidades temporales
		tempents->Clear();

		// Limpiamos todos los decals (sangre, balas, etc)
		engine->ClientCmd( "r_cleardecals" );

		// Paramos los sonidos
		enginesound->StopAllSounds( true );
		Soundscape_OnStopAllSounds();
	}
	else
	{
		BaseClass::FireGameEvent(event);
	}
}

//====================================================================
// Actualiza los efectos PostProcessing
//====================================================================
void InClientMode::UpdatePostProcessingEffects()
{
	PostProcessParameters_t pProcessParameters;
	C_IN_Player *pPlayer = C_IN_Player::GetLocalInPlayer();

	if ( !pPlayer )
		return;

	if ( !pPlayer->GetActivePostProcessController() )
		return;

	// Obtenemos los parametros del controlador del Jugador
	pProcessParameters = pPlayer->GetActivePostProcessController()->m_PostProcessParameters;

	pPlayer->DoPostProcessingEffects( pProcessParameters );

	SetPostProcessParams( &pProcessParameters );
}

//====================================================================
//====================================================================
void InClientMode::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
}

//====================================================================
//====================================================================
void InClientModeFullscreen::InitViewport()
{
	// Skip over BaseClass!!!
	BaseClass::BaseClass::InitViewport();
	
	m_pViewport = new CFullscreenViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

//====================================================================
//====================================================================
void InClientModeFullscreen::Init()
{
	CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();
	
	if ( gameUIFactory )
	{
		IGameUI *pGameUI = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL );
		
		// Se ha cargado la librería con éxito
		if ( pGameUI  )
		{
			// TODO!
			//pGameUI->SetLoadingBackgroundDialog( pPanel->GetVPanel() );
		}		
	}

	// Skip over BaseClass!!!
	BaseClass::Init();

	// Load up the combine control panel scheme
	if ( !g_hVGuiCombineScheme )
	{
		g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme" );
	
		if ( !g_hVGuiCombineScheme )
		{
			Warning( "Couldn't load combine panel scheme!\n" );
		}
	}
}

//====================================================================
//====================================================================
void CSDKModeManager::Init()
{
	for ( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		g_pClientMode[i] = GetClientModeNormal();
	}

	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );
	//GetClientVoiceMgr()->SetHeadLabelOffset( 40 );
}

//====================================================================
//====================================================================
void CSDKModeManager::LevelInit( const char *newmap )
{
	for ( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelInit( newmap );
	}
}

//====================================================================
//====================================================================
void CSDKModeManager::LevelShutdown( void )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelShutdown();
	}
}

extern void UpdateScreenEffectTexture();

PRECACHE_REGISTER_BEGIN( GLOBAL, ModPostProcessing )
	PRECACHE( MATERIAL, "shaders/postproc_fxaa" )
	PRECACHE( MATERIAL, "shaders/postproc_sunrays" )
PRECACHE_REGISTER_END()

//====================================================================
//====================================================================
void DoModPostProcessing( IMatRenderContext *pRenderContext, int x, int y, int w, int h )
{
	ConVarRef mat_postprocess_enable( "mat_postprocess_enable" );

	// No queremos estos efectos
	if ( !mat_postprocess_enable.GetBool() )
		return;

	C_IN_Player *pPlayer = C_IN_Player::GetLocalInPlayer();

	if ( !pPlayer )
		return;

	C_BaseInWeapon *pWeapon = pPlayer->GetActiveInWeapon();

	//
	// FXAA
	//
	if ( mat_fxaa_enable.GetBool() )
	{
		static IMaterial *pMatFXAA = materials->FindMaterial( "shaders/postproc_fxaa", TEXTURE_GROUP_OTHER );
		if ( pMatFXAA )
		{
			UpdateScreenEffectTexture();
			pRenderContext->DrawScreenSpaceRectangle( pMatFXAA, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
		}
	}

	//
	// Sun Rays
	//
	if ( mat_sunrays_enable.GetBool() )
	{
		static IMaterial *pMatSunRays = materials->FindMaterial( "shaders/postproc_sunrays", TEXTURE_GROUP_OTHER );
		if ( pMatSunRays )
		{
			UpdateScreenEffectTexture();
			pRenderContext->DrawScreenSpaceRectangle( pMatSunRays, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
		}
	}

	// Tenemos un arma
	if ( pWeapon )
	{
		// Tenemos la mira de acero
		/*if ( pWeapon->IsIronsighted() )
		{
			static IMaterial *pGaussian = materials->FindMaterial( "shaders/ppe_gaussian_blur", TEXTURE_GROUP_OTHER );

			if ( pGaussian )
			{
				UpdateScreenEffectTexture();
				pRenderContext->DrawScreenSpaceRectangle( pGaussian, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
			}
		}*/
	}
}