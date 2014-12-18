//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "c_ap_player.h"

#include "weapon_inbase.h"

#include "c_te_legacytempents.h"

#include "ap_playeranimstate.h"
#include "tier0/memdbgon.h"

#ifdef CAP_Player
	#undef CAP_Player
#endif

//====================================================================
// Información y Red
//====================================================================

IMPLEMENT_CLIENTCLASS_DT( C_AP_Player, DT_ApPlayer, CAP_Player )

	RecvPropFloat( RECVINFO(m_flBloodLevel) ),
	RecvPropInt( RECVINFO(m_iHungryLevel) ),
	RecvPropInt( RECVINFO(m_iThirstLevel) ),
	RecvPropInt( RECVINFO(m_iExpPoints) ),

END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( player, C_AP_Player );

//====================================================================
// Crea el procesador de animaciones
//====================================================================
void C_AP_Player::CreateAnimationState()
{
	m_nAnimation = CreateApPlayerAnimationState( this );
}

//=========================================================
// Procesa los efectos "PostProcessing"
//=========================================================
void C_AP_Player::DoPostProcessingEffects( PostProcessParameters_t &params )
{
	// Solo si no somos infectados
	if ( !IsInfected() )
		BaseClass::DoPostProcessingEffects( params );
}

//====================================================================
//====================================================================
void C_AP_Player::UpdatePoseParams()
{
	if ( GetViewModel() && GetActiveInWeapon() )
	{
		if ( GetActiveInWeapon()->IsIronsighted() )
			GetViewModel()->SetPoseParameter( "ver_aims", 0.0f );

		float flMoveX = GetPoseParameter( LookupPoseParameter("move_x") );
		GetViewModel()->SetPoseParameter( GetViewModel()->LookupPoseParameter("move_x"), flMoveX );
	}
}