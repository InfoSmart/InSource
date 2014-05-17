#include "cbase.h"
#include "vgui_basehtml.h"

#include "steam/steam_api.h"
#include "gameui_interface.h"

using namespace vgui;
using namespace Awesomium;

#ifdef PostMessage
#undef PostMessage
#endif

//====================================================================
// Comandos
//====================================================================

static ConVar cl_web_think( "cl_web_think", "1" );
static ConVar cl_web_think_interval( "cl_web_think_interval", "1.0" );

//====================================================================
// Constructor
//====================================================================
CBaseHTML::CBaseHTML( vgui::Panel *parent, const char *name, const char *htmlFile ) : BaseClass( parent, name )
{
	// Elemento padre
	SetParent( parent );
	Init( htmlFile );
}
CBaseHTML::CBaseHTML( VPANEL parent, const char *name, const char *htmlFile ) : BaseClass( NULL, name )
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
	// Tamaño del panel
	SetSize( ScreenWidth(), ScreenHeight() );

	// Configuración del panel
	SetProportional( true );
	SetVisible( true );
	SetPaintBorderEnabled( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( IsPC() );

	// Información
	m_pSteamID		= NULL;
	m_flNextThink	= gpGlobals->curtime + 3.0f;

	// Abrimos la dirección local
	const char *pFile = VarArgs("file://%s%s", engine->GetGameDirectory(), htmlFile);
	OpenURL( pFile );
}

//====================================================================
// Pensamiento
//====================================================================
void CBaseHTML::Think()
{
	// No podemos mostrar este panel
	if ( !CanPaint() )
		return;

	BaseClass::Think();

	// La página sigue cargando...
	if ( m_WebView->IsLoading() )
		return;

	// Aún no es hora
	if ( gpGlobals->curtime <= m_flNextThink )
		return;

	WebThink();
}

//====================================================================
//====================================================================
void CBaseHTML::WebThink()
{
	if ( !cl_web_think.GetBool() )
		return;

	float flNextThink = cl_web_think_interval.GetFloat();

	// Información de la partida
	m_pGameObject.SetProperty( WSLit("isInGame"), JSValue( engine->IsInGame() ) );
	m_pGameObject.SetProperty( WSLit("isPaused"), JSValue( engine->IsPaused() ) );
	m_pGameObject.SetProperty( WSLit("maxClients"), JSValue( engine->GetMaxClients() ) );
	m_pGameObject.SetProperty( WSLit("isLoading"), JSValue( GameUI().m_bIsLoading ) );
	
	// Obtenemos al Jugador
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	// Existe, estamos en una partida
	if ( pPlayer )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

		// Información anterior
		int iOldHealth		= m_pGameObject.GetProperty( WSLit("playerHealth") ).ToInteger();
		int iOldAmmo		= m_pGameObject.GetProperty( WSLit("playerClip1Ammo") ).ToInteger();
		int iOldAmmoTotal	= m_pGameObject.GetProperty( WSLit("playerClip1TotalAmmo") ).ToInteger();

		// Información actual del Jugador
		int iHealth		= pPlayer->GetHealth();
		int iAmmo		= 0;
		int iAmmoTotal	= 0;

		// Tenemos un arma
		if ( pWeapon )
		{
			iAmmo		= pWeapon->Clip1();
			iAmmoTotal	= pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
		}

		// La información ha cambiado, pensemos un poco más rápido
		if ( iAmmo != iOldAmmo || iHealth != iOldHealth || iAmmoTotal != iOldAmmoTotal )
		{
			flNextThink = 0.2f;
		}

		// Pasamos información básica
		m_pGameObject.SetProperty( WSLit("playerHealth"), JSValue(iHealth) );
		m_pGameObject.SetProperty( WSLit("playerClip1Ammo"), JSValue(iAmmo) );
		m_pGameObject.SetProperty( WSLit("playerClip1TotalAmmo"), JSValue(iAmmoTotal) );
	}

	// Todavía no hemos cargado la información de Steam
	if ( !m_pSteamID && IsPC() )
	{
		if ( steamapicontext->SteamUser() )
		{
			CSteamID pSteamID	= steamapicontext->SteamUser()->GetSteamID();
			uint64 iSteamID		= pSteamID.ConvertToUint64();
			m_pSteamID			= VarArgs("%llu", iSteamID);
		}

		// ID de Steam del Jugador
		if ( m_pSteamID )
			m_pGameObject.SetProperty( WSLit("playerSteamID"), JSValue(m_pSteamID) );
	}

	// Pensamiento en JS
	ExecuteJavaScript( "Think()", "" );

	// Pensamiento nuevamente en...
	m_flNextThink = gpGlobals->curtime + flNextThink;
}

//====================================================================
//====================================================================
void CBaseHTML::Paint()
{
	if ( !CanPaint() )
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
	// Creamos el objeto JavaScript "Game"
	JSValue pGame = caller->CreateGlobalJavascriptObject( WSLit("Game") );

	// Creamos los métodos disponibles en el objeto
	m_pGameObject = pGame.ToObject();
	m_pGameObject.SetCustomMethod( WSLit("emitSound"), false);
	m_pGameObject.SetCustomMethod( WSLit("runCommand"), false);
	m_pGameObject.SetCustomMethod( WSLit("reload"), false);
	m_pGameObject.SetCustomMethod( WSLit("log"), false);

	// Variables
	m_pGameObject.SetProperty( WSLit("playerHealth"), JSValue(0) );
	m_pGameObject.SetProperty( WSLit("playerClip1Ammo"), JSValue(0) );
	m_pGameObject.SetProperty( WSLit("playerClip1TotalAmmo"), JSValue(0) );
	m_pGameObject.SetProperty( WSLit("playerSteamID"), JSValue(0) );

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
		engine->ClientCmd( ToString(args[0].ToString()).c_str() );
		DevMsg("[AWE] Ejecutando comando: %s \n", ToString(args[0].ToString()).c_str() );
	}

	// Recarga la página
	if ( pMethod == WSLit("reload") )
	{
		// Por ahora esta bien esto
		pCaller->ExecuteJavascript( WSLit("document.location.reload();"), WSLit("") );	
	}

	// Log
	if ( pMethod == WSLit("log") )
	{
		Msg("[AWE] %s \n", ToString(args[0].ToString()).c_str() );
	}

	BaseClass::OnMethodCall( pCaller, pObjectID, pMethod, args );
}