//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ap_playeranimstate.h"

#include "datacache/imdlcache.h"
#include "tier0/vprof.h"

#ifdef CLIENT_DLL
	#include "c_ap_player.h"
#else
	#include "ap_player.h"
#endif

//=========================================================
// Crea y configura un nuevo manejador de animaciones
//=========================================================
CAPPlayerAnimState *CreateApPlayerAnimationState( CAP_Player *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	MultiPlayerMovementData_t pData;
	pData.m_flBodyYawRate	= 120.0f;
	pData.m_flRunSpeed		= ANIM_RUN_SPEED;
	pData.m_flWalkSpeed		= ANIM_WALK_SPEED;
	pData.m_flSprintSpeed	= ANIM_RUN_SPEED;

	CAPPlayerAnimState *pAnim = new CAPPlayerAnimState( pPlayer, pData );
	return pAnim;
}

//=========================================================
// Constructor
//=========================================================
CAPPlayerAnimState::CAPPlayerAnimState( CAP_Player *pPlayer, MultiPlayerMovementData_t &pMovementData ) : BaseClass( pPlayer, pMovementData )
{
	m_nApPlayer = pPlayer;
}

//====================================================================
// Verifica las condiciones del Jugador y actualiza su animación.
//====================================================================
bool CAPPlayerAnimState::ShouldUpdateAnimState()
{
	// Somos invisibles, no hay razón para actualizar nuestra animación
	if ( GetBasePlayer()->IsEffectActive( EF_NODRAW ) )
		return false;

	return true;
}

//====================================================================
// Calcula la actividad actual
//====================================================================
Activity CAPPlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_MP_STAND_IDLE;

	// Primero calculamos la animación principal
	if ( 
		HandleIncap(idealActivity) ||
		HandleFalling(idealActivity) ||
		HandleJumping(idealActivity) || 
		HandleDucking(idealActivity) || 
		HandleSwimming(idealActivity) || 
		HandleDying(idealActivity) 
	)
	{ }

	// De otra forma calculamos la animación de movimiento
	else
		HandleMoving( idealActivity );

	ShowDebugInfo();
	return idealActivity;
}

//====================================================================
//====================================================================
bool CAPPlayerAnimState::HandleIncap( Activity &idealActivity )
{
	// No estas incapacitado
	if ( !GetPlayer()->IsIncap() )
		return false;

	// Estas ¡¡cayendo......!!
	if ( GetPlayer()->IsWaitingGroundDeath() )
	{
		idealActivity = ACT_TERROR_FALL;
	}

	// Estas colgando por tu vida
	else if ( GetPlayer()->IsClimbingIncap() )
	{
		float climbingHold	= GetPlayer()->GetClimbingHold();
		idealActivity		= ACT_TERROR_LEDGE_HANG_FIRM;

		if ( climbingHold < 100 )
		{
			idealActivity = ACT_TERROR_LEDGE_HANG_DANGLE;
		}
		else if ( climbingHold < 250 )
		{
			idealActivity = ACT_TERROR_LEDGE_HANG_WEAK;
		}
	}
	else
	{
		// Incapacitado
		idealActivity = ACT_IDLE_INCAP_PISTOL;

		// Te estan ayudando
		if ( GetPlayer()->GetHelpProgress() > 0 )
		{
			idealActivity = ACT_TERROR_INCAP_TO_STAND;
		}
	}

	return true;
}

//====================================================================
//====================================================================
bool CAPPlayerAnimState::HandleFalling( Activity &idealActivity )
{
	// Estamos cayendo
	if ( m_bFalling )
	{
		// Hemos tocado el suelo o el agua, paramos la animación
		if ( GetPlayer()->IsInGround() || GetPlayer()->GetWaterLevel() >= WL_Waist )
		{
			m_bFalling = false;
			RestartMainSequence();
		}
	}

	// No estamos en el suelo ni en agua ¡estamos cayendo!
	if ( !GetPlayer()->IsInGround() && GetPlayer()->GetWaterLevel() < WL_Waist )
		m_bFalling = true;

	if ( m_bFalling )
	{
		idealActivity = ACT_FALL;
		return true;
	}

	return false;
}