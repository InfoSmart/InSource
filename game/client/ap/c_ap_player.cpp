//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "c_ap_player.h"

#include "ap_playeranimstate.h"
#include "tier0/memdbgon.h"

#ifdef CAP_Player
	#undef CAP_Player
#endif

//====================================================================
// Información y Red
//====================================================================

IMPLEMENT_CLIENTCLASS_DT( C_AP_Player, DT_ApPlayer, CAP_Player )
END_RECV_TABLE()

//====================================================================
// Crea el procesador de animaciones
//====================================================================
void C_AP_Player::CreateAnimationState()
{
	m_nAnimState = CreateApPlayerAnimationState( this );
}