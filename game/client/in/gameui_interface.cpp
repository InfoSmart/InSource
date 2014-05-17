//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#if !defined( _X360 )
#include <windows.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <io.h>
#include <tier0/dbg.h>
#include <direct.h>

#ifdef SendMessage
#undef SendMessage
#endif

#ifdef PostMessage
#undef PostMessage
#endif
				
#include "ModInfo.h"

#include "FileSystem.h"
#include "Sys_Utils.h"
#include "string.h"
#include "tier0/icommandline.h"

// interface to engine
#include "EngineInterface.h"

#include "VGuiSystemModuleLoader.h"
#include "bitmap/TGALoader.h"

#include "GameConsole.h"
#include "game/client/IGameClientExports.h"
#include "materialsystem/imaterialsystem.h"
#include "matchmaking/imatchframework.h"
#include "ixboxsystem.h"
#include "iachievementmgr.h"
#include "IGameUIFuncs.h"
#include "IEngineVGUI.h"

// vgui2 interface
// note that GameUI project uses ..\vgui2\include, not ..\utils\vgui\include
#include "vgui/Cursor.h"
#include "tier1/KeyValues.h"
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/IScheme.h"
#include "vgui/IVGui.h"
#include "vgui/ISystem.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/PHandle.h"
#include "tier3/tier3.h"
#include "matsys_controls/matsyscontrols.h"
#include "steam/steam_api.h"
#include "protocol.h"

#ifdef _X360
	#include "xbox/xbox_win32stubs.h"
#endif // _X360

#include "tier0/dbg.h"
#include "engine/IEngineSound.h"
#include "gameui_util.h"

//
// InSource
//

#include "gameui_interface.h"
#include "in_gameui_hud.h"
#include "VAwesomium.h"

#ifdef APOCALYPSE
	#include "ap_gameui_hud.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGameUI *g_pGameUI					= NULL;
IEngineVGui *enginevguifuncs		= NULL;
vgui::ISurface *enginesurfacefuncs	= NULL;
IAchievementMgr *achievementmgr		= NULL;

#ifdef _X360
	IXOnline  *xonline = NULL;			// 360 only
#endif

static CGameUI g_GameUI;
static WHANDLE g_hMutex			= NULL;
static WHANDLE g_hWaitMutex		= NULL;

static IGameClientExports *g_pGameClientExports = NULL;

IGameClientExports *GameClientExports()
{
	return g_pGameClientExports;
}

//====================================================================
// Acceso rápido a la instancia
//====================================================================
CGameUI &GameUI()
{
	return g_GameUI;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameUIWeb, IGameUI, GAMEUI_INTERFACE_VERSION, g_GameUI );

//====================================================================
//====================================================================
int __stdcall SendShutdownMsgFunc(WHANDLE hwnd, int lparam)
{
	Sys_PostMessage(hwnd, Sys_RegisterWindowMessage("ShutdownValvePlatform"), 0, 1);
	return 1;
}

//====================================================================
// Constructor
//====================================================================
CGameUI::CGameUI()
{
	g_pGameUI = this;

	m_bActivatedUI			= false;
	m_bIsConsoleUI			= false;
	m_bTryingToLoadFriends	= false;

	m_iPlayGameStartupSound		= 0;
	m_iFriendsLoadPauseFrames	= 0;
}

//====================================================================
// Destructor
//====================================================================
CGameUI::~CGameUI()
{
	g_pGameUI = NULL;
}

//====================================================================
// Inicia el sistema y carga todas las librerías
//====================================================================
void CGameUI::Initialize( CreateInterfaceFn factory )
{
	MEM_ALLOC_CREDIT();
	ConnectTier1Libraries( &factory, 1 );
	ConnectTier2Libraries( &factory, 1 );
	ConVar_Register( FCVAR_CLIENTDLL );
	ConnectTier3Libraries( &factory, 1 );

	enginesound = (IEngineSound *)factory(IENGINESOUND_CLIENT_INTERFACE_VERSION, NULL);
	engine		= (IVEngineClient *)factory( VENGINE_CLIENT_INTERFACE_VERSION, NULL );
	bik			= (IBik*)factory( BIK_INTERFACE_VERSION, NULL );

	#ifndef _X360
		SteamAPI_InitSafe();
		steamapicontext->Init();
	#endif

	// ¿Estamos usando el UI para una consola?
	CGameUIConVarRef var( "gameui_xbox" );
	m_bIsConsoleUI = var.IsValid() && var.GetBool();

	vgui::VGui_InitInterfacesList( "GameUI", &factory, 1 );
	vgui::VGui_InitMatSysInterfacesList( "GameUI", &factory, 1 );

	// Cargamos el idioma
	g_pVGuiLocalize->AddFile( "Resource/gameui_%language%.txt", "GAME", true );

	// Cargamos información del MOD (gameinfo)
	ModInfo().LoadCurrentGameInfo();

	// Idioma para kb_act.lst
	g_pVGuiLocalize->AddFile( "Resource/valve_%language%.txt", "GAME", true );

	bool bFailed = false;

	// Empezamos a cargar las librerías del motor
	enginevguifuncs		= (IEngineVGui *)factory( VENGINE_VGUI_VERSION, NULL );
	enginesurfacefuncs	= (vgui::ISurface *)factory(VGUI_SURFACE_INTERFACE_VERSION, NULL);
	gameuifuncs			= (IGameUIFuncs *)factory( VENGINE_GAMEUIFUNCS_VERSION, NULL );
	xboxsystem			= (IXboxSystem *)factory( XBOXSYSTEM_INTERFACE_VERSION, NULL );

	#ifdef _X360
		xonline = (IXOnline *)factory( XONLINE_INTERFACE_VERSION, NULL );
	#endif

	bFailed = !enginesurfacefuncs || !gameuifuncs || !enginevguifuncs || !xboxsystem ||
#ifdef _X360
		!xonline ||
#endif
	!g_pMatchFramework;

	// Hemos fallado
	if ( bFailed )
	{
		Error( "[CGameUI::Initialize] Failed to get necessary interfaces \n" );
	}

	// Creamos el panel Web, esto controlara todo el UI
	vgui::VPANEL rootpanel = enginevguifuncs->GetPanel( PANEL_GAMEUIDLL );
	m_nWebPanelUI = new CGameUIPanelWeb( rootpanel );

	vgui::VPANEL hudpanel = enginevguifuncs->GetPanel( PANEL_INGAMESCREENS );

	#ifdef APOCALYPSE
		new CAP_GameHUD( hudpanel );
	#else
		new CGameHUDWeb( hudPanel );
	#endif
}

//====================================================================
//====================================================================
void CGameUI::PostInit()
{

}

//====================================================================
// Conecta el UI con las librerías del motor/steam
//====================================================================
void CGameUI::Connect( CreateInterfaceFn gameFactory )
{
	g_pGameClientExports	= (IGameClientExports *)gameFactory(GAMECLIENTEXPORTS_INTERFACE_VERSION, NULL);
	achievementmgr			= engine->GetAchievementMgr();

	if ( !g_pGameClientExports )
	{
		Error("CGameUI::Initialize() failed to get necessary interfaces\n");
	}

	m_GameFactory = gameFactory;
}

//====================================================================
// Debemos configurar el GameUI
//====================================================================
void CGameUI::Start()
{
	// determine Steam location for configuration
	if ( !FindPlatformDirectory( m_szPlatformDir, sizeof( m_szPlatformDir ) ) )
		return;

	if ( IsPC() )
	{
		// setup config file directory
		char szConfigDir[512];
		Q_strncpy( szConfigDir, m_szPlatformDir, sizeof( szConfigDir ) );
		Q_strncat( szConfigDir, "config", sizeof( szConfigDir ), COPY_ALL_CHARACTERS );

		Msg( "Steam config directory: %s\n", szConfigDir );

		g_pFullFileSystem->AddSearchPath( szConfigDir, "CONFIG" );
		g_pFullFileSystem->CreateDirHierarchy( "", "CONFIG" );

		// user dialog configuration
		vgui::system()->SetUserConfigFile("InGameDialogConfig.vdf", "CONFIG");
		g_pFullFileSystem->AddSearchPath( "platform", "PLATFORM" );
	}

	// Idiomas
	g_pVGuiLocalize->AddFile( "Resource/platform_%language%.txt");
	g_pVGuiLocalize->AddFile( "Resource/vgui_%language%.txt");

	// Hasta ahora no hemos tenido ningún problema
	Sys_SetLastError( SYS_NO_ERROR );

	// Se trata de una PC, debemos cargar las librerías de Steam
	if ( IsPC() )
	{
		g_hMutex		= Sys_CreateMutex( "ValvePlatformUIMutex" );
		g_hWaitMutex	= Sys_CreateMutex( "ValvePlatformWaitMutex" );

		if ( g_hMutex == NULL || g_hWaitMutex == NULL || Sys_GetLastError() == SYS_ERROR_INVALID_HANDLE )
		{
			// error, can't get handle to mutex
			if ( g_hMutex )
			{
				Sys_ReleaseMutex(g_hMutex);
			}

			if ( g_hWaitMutex )
			{
				Sys_ReleaseMutex(g_hWaitMutex);
			}

			g_hMutex		= NULL;
			g_hWaitMutex	= NULL;

			Error("Steam Error: Could not access Steam, bad mutex\n");
			return;
		}

		unsigned int waitResult = Sys_WaitForSingleObject( g_hMutex, 0 );

		if ( !(waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED) )
		{
			// mutex locked, need to deactivate Steam (so we have the Friends/ServerBrowser data files)
			// get the wait mutex, so that Steam.exe knows that we're trying to acquire ValveTrackerMutex
			waitResult = Sys_WaitForSingleObject(g_hWaitMutex, 0);

			if ( waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED )
			{
				Sys_EnumWindows(SendShutdownMsgFunc, 1);
			}
		}

		// Retrasamos la reproducción de música 2 frames
		m_iPlayGameStartupSound = 2;

		// Establecemos que tenemos pendiente la carga de las ventanas de Amigos y el explorador de servidores
		m_bTryingToLoadFriends		= true;
		m_iFriendsLoadPauseFrames	= 1;
	}
}

//====================================================================
// El GameUI debe apagarse
//====================================================================
void CGameUI::Shutdown()
{
	// notify all the modules of Shutdown
	g_VModuleLoader.ShutdownPlatformModules();

	// unload the modules them from memory
	g_VModuleLoader.UnloadPlatformModules();

	// Liberamos los datos guardados
	ModInfo().FreeModInfo();

	// release platform mutex
	// close the mutex
	if ( g_hMutex )
	{
		Sys_ReleaseMutex(g_hMutex);
	}

	if ( g_hWaitMutex )
	{
		Sys_ReleaseMutex( g_hWaitMutex );
	}

	Awesomium::WebCore *pWebCore = Awesomium::WebCore::instance();

	// Apagamos la instancia/proceso
	if ( pWebCore && VAwesomium::m_iNumberOfViews <= 0 )
	{
		pWebCore->Shutdown();
	}

	steamapicontext->Clear();

	ConVar_Unregister();
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();
}

//====================================================================
// Llamado en cada frame
//====================================================================
void CGameUI::RunFrame()
{
	int wide, tall;

#if defined( TOOLFRAMEWORK_VGUI_REFACTOR )
	// resize the background panel to the screen size
	vgui::VPANEL clientDllPanel = enginevguifuncs->GetPanel( PANEL_ROOT );

	int x, y;
	vgui::ipanel()->GetPos( clientDllPanel, x, y );
	vgui::ipanel()->GetSize( clientDllPanel, wide, tall );
	staticPanel->SetBounds( x, y, wide,tall );
#else
	vgui::surface()->GetScreenSize( wide, tall );
	m_nWebPanelUI->SetWide( wide );
	m_nWebPanelUI->SetTall( tall );
	m_nWebPanelUI->SetSize( wide, tall );
#endif

	// Run frames
	g_VModuleLoader.RunFrame();

	// Reproducimos la música de fondo
	// Iván: Con el GameUI web no es necesario esto, pero lo dejo por si acaso...
	/*if ( m_iPlayGameStartupSound > 0 )
	{
		--m_iPlayGameStartupSound;

		if ( !m_iPlayGameStartupSound )
			PlayGameStartupSound();
	}*/

	// Debemos cargar la librería de servidores y amigos
	if ( IsPC() && m_bTryingToLoadFriends && m_iFriendsLoadPauseFrames-- < 1 && g_hMutex && g_hWaitMutex )
	{
		unsigned int waitResult = Sys_WaitForSingleObject( g_hMutex, 0 );

		if ( waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED )
		{
			// Hemos tenido acceso, cargamos
			m_bTryingToLoadFriends = false;
			g_VModuleLoader.LoadPlatformModules( &m_GameFactory, 1, false );

			// release the wait mutex
			Sys_ReleaseMutex( g_hWaitMutex );

			// notify the game of our game name
			const char *fullGamePath	= engine->GetGameDirectory();
			const char *pathSep			= strrchr( fullGamePath, '/' );

			if ( !pathSep )
			{
				pathSep = strrchr( fullGamePath, '\\' );
			}

			if ( pathSep )
			{
				KeyValues *pKV = new KeyValues("ActiveGameName" );
				pKV->SetString( "name", pathSep + 1 );
				pKV->SetInt( "appid", engine->GetAppID() );
				KeyValues *modinfo = new KeyValues("ModInfo");

				if ( modinfo->LoadFromFile( g_pFullFileSystem, "gameinfo.txt" ) )
				{
					pKV->SetString( "game", modinfo->GetString( "game", "" ) );
				}

				modinfo->deleteThis();				
				g_VModuleLoader.PostMessageToAllModules( pKV );
			}

			// Al parecer se encuentra dentro de un servidor, notificamos
			if ( m_iGameIP )
			{
				SendConnectedToGameMessage();
			}
		}
	}
}

//====================================================================
// [Evento] El Jugador ha hecho pausa
//====================================================================
void CGameUI::OnGameUIActivated()
{
	bool bWasActive = m_bActivatedUI;
	m_bActivatedUI	= true;

	if ( !bWasActive )
	{
		SetGameUIActiveSplitScreenPlayerSlot( engine->GetActiveSplitScreenPlayerSlot() );
	}

	// Lo pasamos al UI web
	GetWebPanel()->OnGameUIActivated();

	// Pausamos el servidor (si es pausable)
	engine->ClientCmd_Unrestricted( "setpause nomsg" );
}

//====================================================================
// [Evento] El Jugador ha quitado la pausa
//====================================================================
void CGameUI::OnGameUIHidden()
{
	bool bWasActive = m_bActivatedUI;
	m_bActivatedUI	= false;

	// unpause the game when leaving the UI
	engine->ClientCmd_Unrestricted( "unpause nomsg" );

	if ( bWasActive )
	{
		SetGameUIActiveSplitScreenPlayerSlot( 0 );
	}

	// Lo pasamos al UI web
	GetWebPanel()->OnGameUIHidden();
}

//====================================================================
//====================================================================
void CGameUI::OLD_OnConnectToServer( const char *game, int IP, int port )
{
	// No se debe llamar a esta función ¡esta obsoleta!
	OnConnectToServer2( game, IP, port, port );
}

//====================================================================
// [Evento] El Jugador ha sido desconectado de la partida
//====================================================================
void CGameUI::OnDisconnectFromServer( uint8 eSteamLoginFailure )
{
	// Restauramos la información
	m_iGameIP				= 0;
	m_iGameConnectionPort	= 0;
	m_iGameQueryPort		= 0;

	// Notificamos a los modulos
	g_VModuleLoader.PostMessageToAllModules(new KeyValues("DisconnectedFromGame"));

	// Lo pasamos al UI web
	GetWebPanel()->OnDisconnectFromServer( eSteamLoginFailure );
}

//====================================================================
// [Evento] Nos hemos conectado a un servidor
//====================================================================
void CGameUI::OnConnectToServer2( const char *game, int IP, int connectionPort, int queryPort )
{
	// Establecemos información del servidor
	m_iGameIP				= IP;
	m_iGameConnectionPort	= connectionPort;
	m_iGameQueryPort		= queryPort;

	// Notificamos
	SendConnectedToGameMessage();
}

//====================================================================
// [Evento] Hemos empezado a cargar un servidor/mapa
//====================================================================
void CGameUI::OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog )
{
	m_bIsLoading = true;

	// Notificamos
	g_VModuleLoader.PostMessageToAllModules( new KeyValues( "LoadingStarted" ) );

	// Lo pasamos al panel web
	m_nWebPanelUI->OnLevelLoadingStarted( levelName, bShowProgressDialog );

	// Don't play the start game sound if this happens before we get to the first frame
	m_iPlayGameStartupSound = 0;
}

//====================================================================
// [Evento] Se ha terminado la carga
//====================================================================
void CGameUI::OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason )
{
	m_bIsLoading = false;

	// Notificamos
	g_VModuleLoader.PostMessageToAllModules( new KeyValues( "LoadingFinished" ) );

	// Lo pasamos al panel web
	m_nWebPanelUI->OnLevelLoadingFinished( bError, failureReason, extendedReason );
}

//====================================================================
// [Evento] Indica el porcentaje de carga del mapa y un
// estado acerca de "que esta cargando"
//====================================================================
bool CGameUI::UpdateProgressBar( float progress, const char *statusText )
{
	// Lo pasamos al panel web
	return m_nWebPanelUI->UpdateProgressBar( progress, statusText );
}

//====================================================================
//====================================================================
bool CGameUI::SetShowProgressText( bool show )
{
	// Lo pasamos al panel web
	return m_nWebPanelUI->SetShowProgressText( show );
}

//====================================================================
//====================================================================
void CGameUI::SetProgressLevelName( const char *levelName )
{
	// Lo pasamos al panel web
	MEM_ALLOC_CREDIT();
	m_nWebPanelUI->SetProgressLevelName( levelName );
}

//====================================================================
//====================================================================
void CGameUI::SetLoadingBackgroundDialog( vgui::VPANEL panel )
{
	Msg( "[SetLoadingBackgroundDialog] \n" );
	// Todo es procesado por el panel web
}

//====================================================================
//====================================================================
void CGameUI::NeedConnectionProblemWaitScreen()
{
	Msg( "[NeedConnectionProblemWaitScreen] \n" );
}

//====================================================================
//====================================================================
void CGameUI::ShowPasswordUI( char const *pchCurrentPW )
{
	Msg( "[ShowPasswordUI] %s \n", pchCurrentPW );
}

//=============================================================================================================================

bool CGameUI::IsInLevel()
{
	const char *levelName = engine->GetLevelName();

	if ( levelName && levelName[0] && !engine->IsLevelMainMenuBackground() )
		return true;

	return false;
}

bool CGameUI::IsInMultiplayer()
{
	return ( IsInLevel() && engine->GetMaxClients() > 1 );
}

bool CGameUI::IsConsoleUI()
{
	return m_bIsConsoleUI;
}

void CGameUI::PlayGameStartupSound()
{
	if ( IsX360() )
		return;

	if ( CommandLine()->FindParm( "-nostartupsound" ) )
		return;

	FileFindHandle_t fh;
	CUtlVector<char *> fileNames;

	// Obtenemos todos los archivos que se llamen gamestartup
	char path[ 512 ];
	Q_snprintf( path, sizeof( path ), "sound/ui/gamestartup*.mp3" );
	Q_FixSlashes( path );

	char const *fn = g_pFullFileSystem->FindFirstEx( path, "MOD", &fh );

	if ( fn )
	{
		do
		{
			char ext[ 10 ];
			Q_ExtractFileExtension( fn, ext, sizeof(ext) );

			if ( !Q_stricmp( ext, "mp3" ) )
			{
				char temp[ 512 ];
				Q_snprintf( temp, sizeof( temp ), "ui/%s", fn );

				char *found = new char[ strlen( temp ) + 1 ];
				Q_strncpy( found, temp, strlen( temp ) + 1 );

				Q_FixSlashes( found );
				fileNames.AddToTail( found );
			}
	
			fn = g_pFullFileSystem->FindNext( fh );
		}
		while ( fn );

		g_pFullFileSystem->FindClose( fh );
	}

	// Al parecer si hay varios archivo de música que reproducir
	if ( fileNames.Count() > 0 )
	{
		// Seleccionamos uno al azar
		SYSTEMTIME SystemTime;
		GetSystemTime( &SystemTime );
		int index = SystemTime.wMilliseconds % fileNames.Count();

		if ( fileNames.IsValidIndex(index) && fileNames[index] )
		{
			char found[ 512 ];

			// Empezamos a reproducir
			// escape chars "*#" make it stream, and be affected by snd_musicvolume
			Q_snprintf( found, sizeof(found), "play *#%s", fileNames[index] );
			engine->ClientCmd_Unrestricted( found );
		}

		fileNames.PurgeAndDeleteElements();
	}
}

bool CGameUI::FindPlatformDirectory( char *platformDir, int bufferSize )
{
	platformDir[0] = '\0';

	if ( platformDir[0] == '\0' )
	{
		// we're not under steam, so setup using path relative to game
		if ( IsPC() )
		{
			if ( ::GetModuleFileName( ( HINSTANCE )GetModuleHandle( NULL ), platformDir, bufferSize ) )
			{
				char *lastslash = strrchr(platformDir, '\\'); // this should be just before the filename
				if ( lastslash )
				{
					*lastslash = 0;
					Q_strncat(platformDir, "\\platform\\", bufferSize, COPY_ALL_CHARACTERS );
					return true;
				}
			}
		}
		else
		{
			// xbox fetches the platform path from exisiting platform search path
			// path to executeable is not correct for xbox remote configuration
			if ( g_pFullFileSystem->GetSearchPath( "PLATFORM", false, platformDir, bufferSize ) )
			{
				char *pSeperator = strchr( platformDir, ';' );
				if ( pSeperator )
					*pSeperator = '\0';
				return true;
			}
		}

		Error( "Unable to determine platform directory\n" );
		return false;
	}

	return ( platformDir[0] != 0 );
}

void CGameUI::ActivateGameUI()
{
	engine->ExecuteClientCmd("gameui_activate");
	SetGameUIActiveSplitScreenPlayerSlot( engine->GetActiveSplitScreenPlayerSlot() );
}

void CGameUI::HideGameUI()
{
	engine->ExecuteClientCmd("gameui_hide");
}

void CGameUI::PreventEngineHideGameUI()
{
	engine->ExecuteClientCmd("gameui_preventescape");
}

void CGameUI::SendConnectedToGameMessage()
{
	MEM_ALLOC_CREDIT();
	KeyValues *kv = new KeyValues( "ConnectedToGame" );

	kv->SetInt( "ip", m_iGameIP );
	kv->SetInt( "connectionport", m_iGameConnectionPort );
	kv->SetInt( "queryport", m_iGameQueryPort );

	g_VModuleLoader.PostMessageToAllModules( kv );
}