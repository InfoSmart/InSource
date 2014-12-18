//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ap_gameui_hud.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/ILocalize.h>

#include "iclientmode.h"
#include "c_in_player.h"

#include "c_ap_player.h"
#include "item_object.h"

using namespace vgui;
using namespace Awesomium;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//====================================================================
// Constructor
//====================================================================
CAP_GameHUD::CAP_GameHUD( vgui::VPANEL parent ) : BaseClass( parent )
{
}

//====================================================================
// Constructor
//====================================================================
void CAP_GameHUD::Init()
{
	BaseClass::Init();

	// Iván: Por ahora solo la mitad de la pantalla. Para ahorrar FPS
	SetSize( ScreenWidth(), (int)(ScreenHeight() / 2)+30 );
	m_iItemsCount = 0;
}

//====================================================================
// Pensamiento
//====================================================================
void CAP_GameHUD::WebThink()
{
	// Actualizamos el inventario
	UpdateInventory();

	// Obtenemos al Jugador
	C_AP_Player *pPlayer = dynamic_cast<C_AP_Player *>( C_BasePlayer::GetLocalPlayer() );

	if ( pPlayer )
	{
		// Actualizamos la información
		m_pGameObject.SetProperty( WSLit("playerBlood"), JSValue(pPlayer->GetBloodLevel()) );
		m_pGameObject.SetProperty( WSLit("playerHungry"), JSValue(pPlayer->GetHungryLevel()) );
		m_pGameObject.SetProperty( WSLit("playerThirst"), JSValue(pPlayer->GetThirstLevel()) );
	}

	BaseClass::WebThink();
}

//====================================================================
//====================================================================
void CAP_GameHUD::UpdateInventory()
{
	C_IN_Player *pPlayer = C_IN_Player::GetLocalInPlayer();

	if ( !pPlayer )
		return;

	// ¡No hay inventario!
	if ( !pPlayer->GetInventory() )
		return;

	int iItemsCount = 0;

	// Por cada slot en el inventario
	for ( int i = 0; i < MAX_INVENTORY_ITEMS; i++ )
	{
		CItemObject *pItem = dynamic_cast<CItemObject*>( pPlayer->GetInventory()->GetItem(i) );

		if ( !pItem )
		{
			ExecuteJavaScript( VarArgs("HUD.Inventory.addItem(%i, null)", i), "");
			continue;
		}

		ExecuteJavaScript( VarArgs("HUD.Inventory.addItem(%i, '%s')", i, pItem->ObjectName()), "");
		++iItemsCount;
	}

	// Nuestro inventario tiene nuevos o menos objetos
	if ( iItemsCount != m_iItemsCount )
	{
		ExecuteJavaScript( "HUD.Inventory.draw()", "" );
		m_iItemsCount = iItemsCount;
	}
}

//====================================================================
// La página web ha terminado de cargar
//====================================================================
void CAP_GameHUD::OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url )
{
	BaseClass::OnDocumentReady( caller, url );

	// Creamos los métodos 
	m_pGameHUDObject.SetCustomMethod( WSLit("dropItem"), false );
	m_pGameHUDObject.SetCustomMethod( WSLit("useItem"), false );

	// Variables
	m_pGameObject.SetProperty( WSLit("playerBlood"), JSValue(0) );
	m_pGameObject.SetProperty( WSLit("playerHungry"), JSValue(0) );
	m_pGameObject.SetProperty( WSLit("playerThirst"), JSValue(0) );
}

//====================================================================
// [Evento] Recibimos un método de un objeto
//====================================================================
void CAP_GameHUD::OnMethodCall( Awesomium::WebView* pCaller, unsigned int pObjectID, const Awesomium::WebString& pMethod, const Awesomium::JSArray& args )
{
	C_IN_Player *pPlayer = C_IN_Player::GetLocalInPlayer();

	//
	// Queremos tirar un objeto del inventario
	//
	if ( pMethod == WSLit("dropItem") )
	{
		int iSlot = args[0].ToInteger();
		
		if ( pPlayer && pPlayer->GetInventory() )
		{
			engine->ClientCmd( VarArgs("drop_item %i", iSlot) );
		}

		return;
	}

	//
	// Queremos usar un objeto del inventario
	//
	if ( pMethod == WSLit("useItem") )
	{
		int iSlot = args[0].ToInteger();

		if ( pPlayer && pPlayer->GetInventory() )
		{
			engine->ClientCmd( VarArgs("use_item %i", iSlot) );
		}

		return;
	}

	BaseClass::OnMethodCall( pCaller, pObjectID, pMethod, args );
}

//====================================================================
//====================================================================
inline void __CmdFunc_ToggleInventory()
{
	HUDWeb->ExecuteJavaScript( "HUD.Inventory.toggle()", "" );

	if ( HUDWeb->IsMouseInputEnabled() )
	{
		HUDWeb->SetMouseInputEnabled( false );
	}
	else
	{
		HUDWeb->SetMouseInputEnabled( true );
	}
}

ConCommand in_toggle_inventory("in_toggle_inventory", __CmdFunc_ToggleInventory, "" );