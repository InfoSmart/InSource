//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_gameui_hud.h"

#include "gameui_interface.h"
#include "steam/steam_api.h"
#include "hud_macros.h"
#include "clientmode.h"
#include "input.h"

#include <vgui/ILocalize.h>

using namespace Awesomium;

#define HTML_FILE	"\\resource\\html\\hud.html"

CGameHUDWeb *HUDWeb = NULL;

//====================================================================
// Constructor
//====================================================================
CGameHUDWeb::CGameHUDWeb( vgui::Panel *parent ) : BaseClass( parent, HTML_FILE )
{
	Init();
}
CGameHUDWeb::CGameHUDWeb( vgui::VPANEL parent ) : BaseClass( parent, HTML_FILE )
{
	Init();
}

//====================================================================
//====================================================================
void CGameHUDWeb::Init()
{
	HUDWeb = this;
	MakePopup( false );

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
}

//====================================================================
// La página web ha terminado de cargar
//====================================================================
void CGameHUDWeb::OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url )
{
	m_iWeaponCount = 0;

	// Creamos el objeto JavaScript "Game"
	JSValue pGameHUD = caller->CreateGlobalJavascriptObject( WSLit("GameHUD") );

	// Creamos los métodos disponibles en el objeto
	m_pGameHUDObject = pGameHUD.ToObject();
	m_pGameHUDObject.SetCustomMethod( WSLit("selectWeapon"), false );

	// Variables
	m_pGameHUDObject.SetProperty( WSLit("hiddenBits"), JSValue(0) );
	m_pGameHUDObject.SetProperty( WSLit("playerWeapons"), JSValue() );

	BaseClass::OnDocumentReady( caller, url );
}

//====================================================================
// [Evento] Recibimos un método de un objeto
//====================================================================
void CGameHUDWeb::OnMethodCall( Awesomium::WebView* pCaller, unsigned int pObjectID, const Awesomium::WebString& pMethod, const Awesomium::JSArray& args )
{
	//
	// Seleccionamos un arma
	//
	if ( pMethod == WSLit("selectWeapon") )
	{
		// Obtenemos al Jugador
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		if ( !pPlayer )
			return;

		// Obtenemos el arma que ha seleccionado
		int iWeaponSlot = args[0].ToInteger();

		// Por cada arma que tenga el Jugador
		for ( int i = 0; i < pPlayer->WeaponCount(); i++ )
		{
			CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);

			if ( !pWeapon )
				continue;

			// No es la que queremos
			if ( pWeapon->GetWeaponID() != iWeaponSlot )
				continue;

			// La hemos encontrado
			::input->MakeWeaponSelection( pWeapon );
			break;
		}

		return;
	}

	BaseClass::OnMethodCall( pCaller, pObjectID, pMethod, args );
}

//====================================================================
// Pensamiento
//====================================================================
void CGameHUDWeb::WebThink()
{
	// Variables
	//m_pGameHUDObject.SetProperty( WSLit("hiddenBits"), JSValue(GetHudHiddenBits()) );

	//
	//UpdateWeaponSelection();

	// Think
	//ExecuteJavaScript( "HudThink()", "" );

	BaseClass::WebThink();
}

//====================================================================
//====================================================================
void CGameHUDWeb::UpdateWeaponSelection()
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	int iTotalWeapons	= 0;
	int iSlotActive		= 0;

	// Por cada arma que tenga el Jugador
	for ( int i = 0; i < pPlayer->WeaponCount(); i++ )
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);

		if ( !pWeapon )
			continue;

		// Es la que tengo ahora mismo
		if ( pWeapon == pPlayer->GetActiveWeapon() )
			iSlotActive = pWeapon->GetWeaponID();

		ExecuteJavaScript( VarArgs("HUD.WeaponSelection.addWeapon('%s', %i)", pWeapon->GetClassname(), pWeapon->GetWeaponID()), "");
		++iTotalWeapons;
	}

	// Nuestras armas han cambiado
	if ( iTotalWeapons != m_iWeaponCount )
	{
		// Dibujamos las nuevas armas y seleccionamos la activa
		ExecuteJavaScript( "HUD.WeaponSelection.draw()", "" );
		ExecuteJavaScript( UTIL_VarArgs("HUD.WeaponSelection.selectWeapon(%i)", iSlotActive), "" );

		m_iWeaponCount = iTotalWeapons;
	}
}

//====================================================================
//====================================================================
int CGameHUDWeb::GetHudHiddenBits()
{
	// No local player yet?
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return 0;

	return pPlayer->m_Local.m_iHideHUD;
}

//====================================================================
//====================================================================
bool CGameHUDWeb::ShouldPaint()
{
	if ( !engine->IsInGame() )
		return false;

	if ( engine->IsPaused() )
		return false;

	if ( GameUI().m_nWebPanelUI->m_bIsPaused )
		return false;

	// No local player yet?
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer( /*m_nSplitScreenSlot*/ );

	if ( !pPlayer )
		return false;

	// Get current hidden flags
	int iHideHud = pPlayer->m_Local.m_iHideHUD;

	// Hide all hud elements if we're blurring the background, since they don't blur properly
	if ( GetClientMode()->GetBlurFade() )
		return false;

	// Everything hidden?
	if ( iHideHud & HIDEHUD_ALL )
		return false;

	return BaseClass::ShouldPaint();
}

//=============================================================================================================================

/*
inline void __CmdFunc_InSlot1()
{
	HUDWeb->ExecuteJavaScript( "HUD.WeaponSelection.selectWeapon(1)", "" );
}

inline void __CmdFunc_InSlot2()
{
	HUDWeb->ExecuteJavaScript( "HUD.WeaponSelection.selectWeapon(2)", "" );
}

inline void __CmdFunc_InNextWeapon()
{
	HUDWeb->ExecuteJavaScript( "HUD.WeaponSelection.nextWeapon()", "" );
}

inline void __CmdFunc_InPrevWeapon()
{
	HUDWeb->ExecuteJavaScript( "HUD.WeaponSelection.prevWeapon()", "" );
}

inline void __CmdFunc_InLastWeapon()
{
}

HOOK_COMMAND( in_slot1, InSlot1 );
HOOK_COMMAND( in_slot2, InSlot2 );
HOOK_COMMAND( in_invnext, InNextWeapon );
HOOK_COMMAND( in_invprev, InPrevWeapon );
HOOK_COMMAND( in_lastinv, InLastWeapon );
*/