#include "cbase.h"
#include "vgui_basehtml.h"

#include "steam/steam_api.h"
#include "gameui_interface.h"

#include "in_shareddefs.h"
#include "in_gamerules.h"

using namespace vgui;
using namespace Awesomium;

#ifdef PostMessage
#undef PostMessage
#endif

//====================================================================
// Comandos
//====================================================================

ConVar cl_web_ui( "cl_web_ui", "1" );
ConVar cl_web_think( "cl_web_think", "1" );
ConVar cl_web_think_interval( "cl_web_think_interval", "1.0" );

//====================================================================
// Constructor
//====================================================================
CBaseHTML::CBaseHTML( vgui::Panel *parent, const char *htmlFile ) : BaseClass( parent )
{
	// Elemento padre
	SetParent( parent );
	Init( htmlFile );
}
CBaseHTML::CBaseHTML( VPANEL parent, const char *htmlFile ) : BaseClass( NULL )
{
	// Elemento padre
	SetParent( parent );
	Init( htmlFile );
}

//====================================================================
// Inicia el panel Web
//====================================================================
void CBaseHTML::Init( const char *htmlFile )
{
	m_iWidth	= 0;
	m_iHeight	= 0;

	// Tamaño del panel
	SetSize( ScreenWidth(), ScreenHeight() );

	// Configuración del panel
	SetVisible( true );
	SetEnabled( true );
	
	SetProportional( true );
	SetPaintBorderEnabled( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( IsPC() );

	BaseClass::Init();

	// Información
	m_pSteamID		= NULL;
	m_flNextThink	= gpGlobals->curtime + 3.0f;

	// Abrimos la dirección local
	const char *pAddress = VarArgs("file://%s%s", engine->GetGameDirectory(), htmlFile);
	OpenURL( pAddress );
}

void CBaseHTML::SetSize( int wide, int tall )
{
	BaseClass::SetSize( wide, tall );
	return;

	if ( wide != m_iWidth || tall != m_iHeight )
	{
		Panel::SetSize( wide, tall );

		SetWide( wide );
		SetTall( tall );

		if ( m_iWidth != 0 )
		{
			PerformLayout();
		}

		m_iWidth	= wide;
		m_iHeight	= tall;
	}
}

//====================================================================
// Pensamiento
//====================================================================
void CBaseHTML::OnThink()
{
	// No podemos mostrar este panel
	if ( !ShouldPaint() )
		return;

	BaseClass::OnThink();
	WebThink();
}

//====================================================================
//====================================================================
void CBaseHTML::SetProperty( const char *pKeyname, JSValue pValue, JSObject pObject )
{
	JSValue pOldValue = pObject.GetProperty( WSLit(pKeyname) );

	//if ( FStrEq(ToString(pOldValue.ToString()).c_str(), ToString(pValue.ToString()).c_str()) )
		//return;

	pObject.SetProperty( WSLit(pKeyname), pValue );
}

//====================================================================
//====================================================================
void CBaseHTML::WebThink()
{
	if ( !cl_web_think.GetBool() )
		return;

	float flNextThink = cl_web_think_interval.GetFloat();

	// Información de la partida
	SetProperty( "isInGame", JSValue(engine->IsInGame()), m_pGameObject );
	SetProperty( "isPaused", JSValue(engine->IsPaused()), m_pGameObject );
	SetProperty( "maxClients", JSValue(engine->GetMaxClients()), m_pGameObject );
	SetProperty( "isLoading", JSValue(GameUI().m_bIsLoading), m_pGameObject );

	if ( InRules )
		SetProperty( "gameMode", JSValue(InRules->GameMode()), m_pGameObject );
	
	// Obtenemos al Jugador
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	// Existe, estamos en una partida
	if ( pPlayer )
	{
		// Información actual del Jugador
		C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
		int iHealth					= pPlayer->GetHealth();
		int iAmmo					= 0;
		int iAmmoTotal				= 0;

		// Tenemos un arma
		if ( pWeapon )
		{
			iAmmo		= pWeapon->Clip1();
			iAmmoTotal	= pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
		}

		// Actualizamos la información básica
		SetProperty( "health", JSValue(iHealth), m_pPlayerObject );
		SetProperty( "clip1Ammo", JSValue(iAmmo), m_pPlayerObject );
		SetProperty( "clip1TotalAmmo", JSValue(iAmmoTotal), m_pPlayerObject );
	}

	// Información de la API de Steam
	if ( IsPC() && steamapicontext )
	{
		// ID de Steam
		if ( !m_pSteamID )
		{
			// Intentamos obtener la ID
			if ( steamapicontext->SteamUser() )
			{
				CSteamID steamID	= steamapicontext->SteamUser()->GetSteamID();
				uint64 mySteamID	= steamID.ConvertToUint64();
				m_pSteamID			= VarArgs("%llu", mySteamID);
			}

			// ID cargada, se lo dejamos a JavaScript...
			if ( m_pSteamID )
			{
				SetProperty( "playerID", JSValue(WSLit(m_pSteamID)), m_pSteamObject );
				ExecuteJavaScript( "Helper.loadUserInfo()", "" );
			}
		}

		// Utilidades e información
		if ( steamapicontext->SteamUtils() )
		{
			uint32 serverTime	= steamapicontext->SteamUtils()->GetServerRealTime();
			const char *country = steamapicontext->SteamUtils()->GetIPCountry();
			uint8 battery		= steamapicontext->SteamUtils()->GetCurrentBatteryPower();
			uint32 computer		= steamapicontext->SteamUtils()->GetSecondsSinceComputerActive();

			SetProperty( "serverTime", JSValue(WSLit(VarArgs("%llu", serverTime))), m_pSteamObject );
			SetProperty( "country", JSValue(country), m_pSteamObject );
			SetProperty( "batteryPower", JSValue(WSLit(VarArgs("%llu", battery))), m_pSteamObject );
			SetProperty( "computerActive", JSValue(WSLit(VarArgs("%llu", computer))), m_pSteamObject );
		}
	}

	// Pensamiento en JS
	ExecuteJavaScript( "think()", "" );

	// Pensamiento nuevamente en...
	m_flNextThink = gpGlobals->curtime + flNextThink;
}

//====================================================================
//====================================================================
bool CBaseHTML::ShouldPaint()
{
	// La página sigue cargando...
	if ( m_WebView->IsLoading() )
		return false;

	// Aún no es hora
	if ( gpGlobals->curtime <= m_flNextThink )
		return false;

	return cl_web_ui.GetBool();
}

//====================================================================
//====================================================================
void CBaseHTML::Paint()
{
	if ( !ShouldPaint() )
	{
		m_WebView->PauseRendering();
		return;
	}

	m_WebView->ResumeRendering();
	BaseClass::Paint();
}

//====================================================================
//====================================================================
void CBaseHTML::PaintBackground()
{
	BaseClass::PaintBackground();

	// Invisible
	SetBgColor( Color(0,0,0,0) );
	SetPaintBackgroundType( 0 );
}

//====================================================================
// La página web ha terminado de cargar
//====================================================================
void CBaseHTML::OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url )
{
	try
	{
		m_pSteamID = NULL;

		// Creamos el objeto JavaScript "Game"
		JSValue pGame		= caller->CreateGlobalJavascriptObject( WSLit("Game") );
		JSValue pSteam		= caller->CreateGlobalJavascriptObject( WSLit("Steam") );
		JSValue pPlayer		= caller->CreateGlobalJavascriptObject( WSLit("Player") );

		m_pGameObject	= pGame.ToObject();
		m_pSteamObject	= pSteam.ToObject();
		m_pPlayerObject	= pPlayer.ToObject();

		// Métodos	
		m_pGameObject.SetCustomMethod( WSLit("emitSound"), false);
		m_pGameObject.SetCustomMethod( WSLit("runCommand"), false);
		m_pGameObject.SetCustomMethod( WSLit("reload"), false);
		m_pGameObject.SetCustomMethod( WSLit("log"), false);
		m_pGameObject.SetCustomMethod( WSLit("getCommand"), true );

		// Variables
		m_pPlayerObject.SetProperty( WSLit("health"), JSValue(0) );
		m_pPlayerObject.SetProperty( WSLit("clip1Ammo"), JSValue(0) );
		m_pPlayerObject.SetProperty( WSLit("clip1TotalAmmo"), JSValue(0) );
		m_pPlayerObject.SetProperty( WSLit("info"), JSValue(0) );

		m_pSteamObject.SetProperty( WSLit("playerID"), JSValue(0) );
		m_pSteamObject.SetProperty( WSLit("serverTime"), JSValue(0) );
		m_pSteamObject.SetProperty( WSLit("country"), JSValue(WSLit("NULL")) );
		m_pSteamObject.SetProperty( WSLit("batteryPower"), JSValue(255) );
		m_pSteamObject.SetProperty( WSLit("computerActive"), JSValue(0) );

		m_pGameObject.SetProperty( WSLit("Steam"), JSValue(m_pSteamObject) );
		m_pGameObject.SetProperty( WSLit("Player"), JSValue(m_pPlayerObject) );

		m_pGameObject.SetProperty( WSLit("gameMode"), JSValue(GAME_MODE_NONE) );
		m_pGameObject.SetProperty( WSLit("isInGame"), JSValue(false) );
		m_pGameObject.SetProperty( WSLit("isPaused"), JSValue(false) );
		m_pGameObject.SetProperty( WSLit("maxClients"), JSValue(32) );
		m_pGameObject.SetProperty( WSLit("isLoaded"), JSValue(false) );
	}
	catch( ... )
	{
		Warning("[CBaseHTML::OnDocumentReady] ERROR! \n");
	}

	BaseClass::OnDocumentReady( caller, url );
}

//====================================================================
// [Evento] Recibimos un método de un objeto
//====================================================================
void CBaseHTML::OnMethodCall( Awesomium::WebView* pCaller, unsigned int pObjectID, const Awesomium::WebString& pMethod, const Awesomium::JSArray& args )
{
	if ( pMethod == WSLit("emitSound") )
	{
		C_BasePlayer *pPlayer	= C_BasePlayer::GetLocalPlayer();
		const char *pSoundName	= ToString(args[0].ToString()).c_str();

		DevMsg("[AWE] Reproduciendo: %s \n", ToString(args[0].ToString()).c_str() );

		// Si el Juego ha iniciado podemos usar el método del Jugador
		if ( pPlayer )
		{
			pPlayer->EmitSound( pSoundName );
		}

		// De otra forma usamos el comando "play"
		// Ojo: Solo acepta rutas hacia archivos, no SoundScripts
		else
		{
			const char *pCommand = UTIL_VarArgs( "play %s", pSoundName );
			engine->ClientCmd_Unrestricted( pCommand );
		}
	}

	// Ejecutar comando
	if ( pMethod == WSLit("runCommand") )
	{
		engine->ClientCmd_Unrestricted( ToString(args[0].ToString()).c_str() );
		DevMsg("[GameUI] Ejecutando comando: %s \n", ToString(args[0].ToString()).c_str() );
	}

	// Recarga la página
	if ( pMethod == WSLit("reload") )
	{
		// Por ahora esta bien esto
		pCaller->ExecuteJavascript( WSLit("document.location.reload();"), WSLit("") );	
	}

	// Log
	if ( pMethod == WSLit("log") )
		Msg("[GameUI] %s \n", ToString(args[0].ToString()).c_str() );
	if ( pMethod == WSLit("warn") )
		Warning("[GameUI] %s \n", ToString(args[0].ToString()).c_str() );

	BaseClass::OnMethodCall( pCaller, pObjectID, pMethod, args );
}

//====================================================================
// [Evento] Recibimos un método de un objeto
//====================================================================
JSValue CBaseHTML::OnMethodCallWithReturnValue( Awesomium::WebView* pCaller, unsigned int pObjectID, const Awesomium::WebString& pMethod, const Awesomium::JSArray& args )
{
	if ( pMethod == WSLit("getCommand") )
	{
		ConVarRef command( ToString(args[0].ToString()).c_str() );

		if ( !command.IsValid() )
			return JSValue("");

		return JSValue( WSLit(command.GetString()) );
	}

	return JSValue(-1);
}