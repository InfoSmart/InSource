//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_gameui_menu.h"

#include "gameui_interface.h"
#include "steam/steam_api.h"

#include <vgui/ILocalize.h>

#define HTML_FILE	"\\resource\\html\\gameui.html"

//====================================================================
// Constructor
//====================================================================
CGameUIPanelWeb::CGameUIPanelWeb( vgui::VPANEL parent ) : BaseClass( parent, HTML_FILE )
{
	MakePopup( false );
	m_bIsPaused = false;
}

//====================================================================
//====================================================================
bool CGameUIPanelWeb::ShouldPaint()
{
	if ( engine->IsInGame() && !m_bIsPaused )
		return false;

	return BaseClass::ShouldPaint();
}

//====================================================================
//====================================================================
void CGameUIPanelWeb::Paint()
{
	//CBaseHTML::SetSize( ScreenWidth(), ScreenHeight() );
	BaseClass::Paint();
}

//====================================================================
// [Evento] El Jugador ha hecho pausa
//====================================================================
void CGameUIPanelWeb::OnGameUIActivated()
{
	ExecuteJavaScript( VarArgs("Helper.setIsClientPaused(%i)", engine->IsInGame()), "");
	m_bIsPaused = engine->IsInGame();
}

//====================================================================
// [Evento] El Jugador ha quitado la pausa
//====================================================================
void CGameUIPanelWeb::OnGameUIHidden()
{
	ExecuteJavaScript( "Helper.setIsClientPaused(0)", "");
	m_bIsPaused = false;
}

//====================================================================
//====================================================================
void CGameUIPanelWeb::OnDisconnectFromServer( uint8 eSteamLoginFailure )
{
	switch ( eSteamLoginFailure )
	{
		//
		case STEAMLOGINFAILURE_BADTICKET:
		{
			break;
		}

		//
		case STEAMLOGINFAILURE_NOSTEAMLOGIN:
		{
			break;
		}

		//
		case STEAMLOGINFAILURE_VACBANNED:
		{
			break;
		}

		//
		case STEAMLOGINFAILURE_LOGGED_IN_ELSEWHERE:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

//====================================================================
// [Evento] Hemos empezado a cargar un servidor/mapa
//====================================================================
void CGameUIPanelWeb::OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog )
{
	ExecuteJavaScript( "GameUI.showLoadingPanel()", "");
}

//====================================================================
// [Evento] Se ha terminado la carga
//====================================================================
void CGameUIPanelWeb::OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason )
{
	// Esto ocultara el panel y avisará de cualquier error
	ExecuteJavaScript( VarArgs("GameUI.hideLoadingPanel(%i, '%s', '%s')", bError, failureReason, extendedReason), "");
}

//====================================================================
// [Evento] Indica el porcentaje de carga del mapa y un
// estado acerca de "que esta cargando"
//====================================================================
bool CGameUIPanelWeb::UpdateProgressBar( float progress, const char *statusText )
{
	// No estamos cargando ningún nivel
	if ( !GameUI().IsLoading() )
		return false;

	// Tratamos de traducir el mensaje
	LocalizeStringIndex_t pIndex = g_pVGuiLocalize->FindIndex( statusText );

	if ( pIndex != LOCALIZE_INVALID_STRING_INDEX )
		statusText = g_pVGuiLocalize->GetNameByIndex( pIndex );

	// progress es un valor de 0 a 1, aquí lo convertimos a un valor entre 0 a 100
	int iProgress = ( progress * 100 );

	// Mandamos el JavaScript
	ExecuteJavaScript( VarArgs("GameUI.updateProgressBar(%i, '%s')", iProgress, statusText), "");

	// TODO: Enviar "true" hara algo especial?
	return false;
}

//====================================================================
// TODO
//====================================================================
bool CGameUIPanelWeb::SetShowProgressText( bool show )
{
	Msg( "[SetShowProgressText] %i \n", show );
	return false;
}

//====================================================================
// TODO
//====================================================================
void CGameUIPanelWeb::SetProgressLevelName( const char *levelName )
{
	Msg( "[SetProgressLevelName] %i \n", levelName );
}