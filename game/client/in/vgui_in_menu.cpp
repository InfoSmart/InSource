//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"
#include "vgui_in_menu.h"

#include "ienginevgui.h"
#include "filesystem.h"

#include "steam/steam_api.h"

using namespace vgui;
using namespace Awesomium;

#ifdef PostMessage
	#undef PostMessage
#endif

static CDllDemandLoader g_GameUIDLL( "GameUI" );
static IGameUI *gameui;

#define MENU_HTML_FILE "\\resource\\html\\menu.html"

//=========================================================
// Carga el GameUI
//=========================================================
bool LoadGameUI()
{
	// El GameUI no ha sido cargado
	if ( !gameui )
	{
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();

		if ( gameUIFactory )
			gameui = (IGameUI *)gameUIFactory( GAMEUI_INTERFACE_VERSION, NULL );
	}

	if ( !gameui )
		return false;

	return true;
}

//=========================================================
// Constructor
//=========================================================
CBaseHTML::CBaseHTML( VPANEL parent, const char *name, const char *htmlFile ) : BaseClass( (Panel*)parent, name )
{
	// Elemento padre
	SetParent( parent );

	// Tamaño del panel
	SetSize( ScreenWidth(), ScreenHeight() );

	// Configuración del panel
	SetProportional( true );
	SetVisible( false );
	SetPaintBackgroundEnabled( true );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	// Configuración del navegador

	// Abrimos la dirección local
	const char *pFile = VarArgs("file://%s%s", engine->GetGameDirectory(), htmlFile);
	OpenURL( pFile );
}

//=========================================================
// Pensamiento
//=========================================================
void CBaseHTML::Think()
{
	BaseClass::Think();

	// La página sigue cargando...
	if ( m_WebView->IsLoading() )
		return;

	// Variables globales
	ExecuteJavaScript( VarArgs("Helper.SetInGame(%i)", engine->IsInGame()), "");
	ExecuteJavaScript( VarArgs("Helper.SetIsPaused(%i)", engine->IsPaused()), "");
	ExecuteJavaScript( VarArgs("Helper.SetMaxClients(%i)", engine->GetMaxClients()), "");

	// Información del usuario
	if ( steamapicontext->SteamUser() )
	{
		CSteamID pSteamID	= steamapicontext->SteamUser()->GetSteamID();
		uint64 iID			= pSteamID.ConvertToUint64();

		if ( iID )
			ExecuteJavaScript( VarArgs("Helper.SetSteamID('%llu')", iID), "");
	}
}

//=========================================================
//=========================================================
void CBaseHTML::PaintBackground()
{
	BaseClass::PaintBackground();

	// Invisible
	SetBgColor( Color(0,0,0,0) );
	SetPaintBackgroundType( 0 );
}

//=========================================================
// La página web ha sido cargada
//=========================================================
void CBaseHTML::OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url )
{
	// Creamos el objeto JavaScript "Game"
	JSValue pResult = caller->CreateGlobalJavascriptObject( WSLit("Game") );

	// Creamos los métodos disponibles en el objeto
	JSObject &pObject = pResult.ToObject();
	pObject.SetCustomMethod( WSLit("RunCommand"), false);
	pObject.SetCustomMethod( WSLit("MenuCommand"), false);
	pObject.SetCustomMethod( WSLit("GetInfo"), false);
	pObject.SetCustomMethod( WSLit("Reload"), false);
	pObject.SetCustomMethod( WSLit("Log"), false);

	BaseClass::OnDocumentReady( caller, url );
}

//=========================================================
// [Evento] Recibimos un método de un objeto
//=========================================================
void CBaseHTML::OnMethodCall(Awesomium::WebView* pCaller, unsigned int pObjectID, const Awesomium::WebString& pMethod, const Awesomium::JSArray& args)
{
	// Ejecutar comando
	if ( pMethod == WSLit("RunCommand") )
	{
		engine->ClientCmd( ToString(args[0].ToString()).c_str() );
		DevMsg("[AWE] Ejecutando comando: %s \n", ToString(args[0].ToString()).c_str() );
	}

	// Ejecutar comando del GameUI
	if ( pMethod == WSLit("MenuCommand") )
	{
		gameui->SendMainMenuCommand( ToString(args[0].ToString()).c_str() );
	}

	// Solicitar información del jugador
	if ( pMethod == WSLit("GetInfo") )
	{
		/*CSteamID *pSteamID		= new CSteamID();
		C_BasePlayer *pPlayer	= C_BasePlayer::GetLocalPlayer();
		int iID					= 0;
		const char *pUsername	= "";

		if ( pPlayer )
		{
			// @TODO: Hacer funcionar.
			pPlayer->GetSteamID(pSteamID);
			pID			= pSteamID->ConvertToUint64();
			pUsername	= pPlayer->GetPlayerName();
		}

		pCaller->ExecuteJavascript( WSLit(VarArgs("SetInfo(true, %i, '%s', %i);", engine->IsInGame(), pUsername, pID)), WSLit("") );*/
	}

	// Recarga la página
	if ( pMethod == WSLit("Reload") )
	{
		// Por ahora esta bien esto
		pCaller->ExecuteJavascript( WSLit("document.location.reload();"), WSLit("") );	
	}

	// Log
	if ( pMethod == WSLit("Log") )
	{
		DevMsg("[AWE] %s \n", ToString(args[0].ToString()).c_str() );
	}

	BaseClass::OnMethodCall( pCaller, pObjectID, pMethod, args );
}

//=================================================================================================
//=================================================================================================

//=========================================================
// >> CSMenu
// Clase para cargar el nuevo menú en HTML
//=========================================================
class CSMenu : public ISMenu
{
public:
	CBaseHTML *m_hBaseHTML;

	CSMenu()
	{
		m_hBaseHTML = NULL;
	}
 
	void Create( VPANEL parent )
	{
		m_hBaseHTML = new CBaseHTML( parent, "VGUIMenu", MENU_HTML_FILE );

		// Sobrescribimos el menú el Juego
		if ( LoadGameUI() )
			gameui->SetMainMenuOverride( GetPanel() );
	}
 
	void Destroy()
	{
		if ( m_hBaseHTML )
		{
			m_hBaseHTML->SetParent( (Panel *)NULL );
			delete m_hBaseHTML;
		}
	}

	void SetVisible( bool bVisible )
	{
		if ( m_hBaseHTML )
			m_hBaseHTML->SetVisible( bVisible );
	}

	vgui::VPANEL GetPanel( void )
	{
		if ( !m_hBaseHTML )
			return NULL;

		return m_hBaseHTML->GetVPanel();
	}
 
};
 
static CSMenu g_SMenu;
ISMenu *SMenu = ( ISMenu * )&g_SMenu;