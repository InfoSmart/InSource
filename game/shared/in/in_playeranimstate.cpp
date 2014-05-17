//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_playeranimstate.h"

#include "weapon_inbase.h"

#include "datacache/imdlcache.h"
#include "tier0/vprof.h"

#ifdef CLIENT_DLL
	#include "c_in_player.h"
	#include "bone_setup.h"
	#include "interpolatedvar.h"
#else
	#include "in_player.h"
#endif

//====================================================================
// Crea y configura un nuevo manejador de animaciones
//====================================================================
CInPlayerAnimState *CreatePlayerAnimationState( CIN_Player *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	MultiPlayerMovementData_t pData;
	pData.m_flBodyYawRate	= 720.0f;
	pData.m_flRunSpeed		= ANIM_RUN_SPEED;
	pData.m_flWalkSpeed		= ANIM_WALK_SPEED;
	pData.m_flSprintSpeed	= ANIM_RUN_SPEED;

	CInPlayerAnimState *pAnim = new CInPlayerAnimState( pPlayer, pData );
	return pAnim;
}

//====================================================================
// Constructor
//====================================================================
CInPlayerAnimState::CInPlayerAnimState()
{	
}
CInPlayerAnimState::CInPlayerAnimState( CIN_Player *pPlayer, MultiPlayerMovementData_t &pMovementData ) : CMultiPlayerAnimState( pPlayer, pMovementData )
{
	m_nPlayer = pPlayer;
}

//====================================================================
// Traduce una actividad a otra
//====================================================================
Activity CInPlayerAnimState::TranslateActivity( Activity actBase )
{
	return m_nPlayer->TranslateActivity( actBase );
}

//====================================================================
// Verifica las condiciones del Jugador y actualiza su animación.
//====================================================================
void CInPlayerAnimState::Update()
{
	// Profile the animation update.
	VPROF("CInPlayerAnimState::Update");
	
	// Jugador inválido
	if ( !m_nPlayer )
	{
		DevWarning("[CInPlayerAnimState::Update] Intento de actualización en una entidad que no es un jugador. \n");
		return;
	}

	CStudioHdr *pStudioHdr = m_nPlayer->GetModelPtr();

	if ( !pStudioHdr )
		return;

	// No debemos actualizar la animación del Jugador
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	float eyeYaw		= m_nPlayer->EyeAngles()[YAW];
	float eyePitch		= m_nPlayer->EyeAngles()[PITCH];
	bool bComputePoses	= true;

	// Guardamos los angulos de los ojos
	m_flEyeYaw		= AngleNormalize(eyeYaw);
	m_flEyePitch	= AngleNormalize(eyePitch);

	// Calculamos la animación actual
	ComputeSequences( pStudioHdr );

	// No hemos podido configurar los parametros de movimiento
	if ( !SetupPoseParameters(pStudioHdr) )
		bComputePoses = false;

	// Esta muerto o incapacitado
	if ( !GetPlayer()->IsAlive() || GetPlayer()->IsIncap() )
		bComputePoses = false;

	// Debemos calcular la dirección (voltearnos)
	if ( bComputePoses )
	{
		// Calculamos la dirección hacia donde deben ir las piernas
		ComputePoseParam_MoveYaw( pStudioHdr );

		// Calculamos la dirección hacia donde debe moverse el torso (Arriba/Abajo)
		ComputePoseParam_AimPitch( pStudioHdr );
	}

	// Calculamos la dirección hacia donde debe moverse el torso (Rotación)
	ComputePoseParam_AimYaw( pStudioHdr );

#ifdef CLIENT_DLL 
	float flSeqSpeed = m_nPlayer->GetSequenceGroundSpeed( m_nPlayer->GetSequence() );
 
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	float flSpeed			= vecVelocity.Length2DSqr();
	bool bMoving_OnGround	= flSpeed > 0.01f && m_nPlayer->GetGroundEntity();
 
	flSpeed = bMoving_OnGround ? clamp( (vecVelocity.Length2DSqr() / (flSeqSpeed*flSeqSpeed)), 0.2f, 2.0f ) : 1.0f;
	m_nPlayer->SetPlaybackRate( flSpeed );
#endif
}

//====================================================================
// Calcula la actividad actual
//====================================================================
Activity CInPlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_MP_STAND_IDLE;

	// Primero calculamos la animación en caso de estar:
	// Saltando, agachado, nadando o muriendo.
	if ( 
		HandleJumping(idealActivity) || 
		HandleDucking(idealActivity) || 
		HandleSwimming(idealActivity) || 
		HandleDying(idealActivity) 
	)
	{ }

	// De otra forma calculamos la animación de movimiento.
	else
		HandleMoving( idealActivity );

	ShowDebugInfo();
	return idealActivity;
}

//====================================================================
// Realiza una animación
//====================================================================
void CInPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	Activity iGestureActivity = ACT_INVALID;

	switch( event )
	{
		// Disparo primario
		case PLAYERANIMEVENT_ATTACK_PRIMARY:
		{
			if ( m_pPlayer->GetFlags() & FL_DUCKING )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE );

			iGestureActivity = ACT_VM_PRIMARYATTACK;
			break;
		}

		// Disparo secundario
		case PLAYERANIMEVENT_ATTACK_SECONDARY:
		{
			// Weapon secondary fire.
			if ( m_pPlayer->GetFlags() & FL_DUCKING )
				RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_SECONDARYFIRE);
			else
				RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE);

			iGestureActivity = ACT_VM_SECONDARYATTACK;
			break;
		}

		// 
		case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
		{
			if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );

			break;
		}

		// 
		case PLAYERANIMEVENT_ATTACK_PRE:
		{
			// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
			if ( m_pPlayer->GetFlags() & FL_DUCKING ) 
				iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;

			// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
			else				
				iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;

			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, false);
			break;
		}

		//
		case PLAYERANIMEVENT_ATTACK_POST:
		{
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_POSTFIRE);
			break;
		}

		// Recarga
		case PLAYERANIMEVENT_RELOAD:
		{
			if ( m_pPlayer->GetFlags() & FL_DUCKING )
				RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH);
			else
				RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND);

			break;
		}

		// Recarga
		case PLAYERANIMEVENT_RELOAD_LOOP:
		{
			if ( m_pPlayer->GetFlags() & FL_DUCKING )
				RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_LOOP);
			else
				RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_LOOP);

			break;
		}

		// Finalizar la recarga.
		case PLAYERANIMEVENT_RELOAD_END:
		{
			if ( m_pPlayer->GetFlags() & FL_DUCKING )
				RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_END);
			else
				RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_END);

			break;
		}

		default:
			BaseClass::DoAnimationEvent( event, nData );
		break;
	}

#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if ( iGestureActivity != ACT_INVALID )
	{
		CBaseCombatWeapon *pWeapon = m_pPlayer->GetActiveWeapon();

		if ( pWeapon )
		{
			//pWeapon->EnsureCorrectRenderingModel();
			pWeapon->SendWeaponAnim( iGestureActivity );

			//pWeapon->ResetEventsParity();
			pWeapon->DoAnimationEvents( pWeapon->GetModelPtr() );
		}
	}
#endif
}

//====================================================================
// Teletransporta al jugador
//====================================================================
void CInPlayerAnimState::Teleport( const Vector *pNewOrigin, const QAngle *pNewAngles, CBasePlayer* pPlayer )
{
	QAngle absangles	= pPlayer->GetAbsAngles();
	m_angRender			= absangles;
	m_angRender.x		= m_angRender.z = 0.0f;

	if ( pPlayer )
		m_flCurrentFeetYaw = m_flGoalFeetYaw = m_flEyeYaw = pPlayer->EyeAngles()[YAW];
}

//====================================================================
// Devuelve si el jugador esta nadando.
// Si es así también define la animación para ello.
//====================================================================
bool CInPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	// El nivel de agua no es suficiente
	if ( m_nPlayer->GetWaterLevel() < WL_Waist )
	{
		m_bInSwim = false;
		return false;
	}

	// Nos estamos moviendo mientras nadamos
	if ( GetOuterXYSpeed( ) > ANIM_WALK_SPEED )
		idealActivity = ACT_MP_SWIM;

	// Estamos quietos
	else
		idealActivity = ACT_MP_SWIM_IDLE;

	m_bInSwim = true;
	return true;
}

//====================================================================
// Devuelve si el jugador se esta moviendo.
// Si es así también define la animación para ello.
//====================================================================
bool CInPlayerAnimState::HandleMoving( Activity &idealActivity )
{
	float flSpeed = GetOuterXYSpeed();

	// Estamos corriendo
	if ( flSpeed > ANIM_RUN_SPEED )
		idealActivity = ACT_MP_RUN;

	// Estamos caminando
	else if ( flSpeed > ANIM_WALK_SPEED )
		idealActivity = ACT_MP_WALK;

	return true;
}

//====================================================================
// Devuelve si el jugador esta agachado.
// Si es así también define la animación para ello.
//====================================================================
bool CInPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	// No estamos agachados
	if ( !(m_nPlayer->GetFlags() & FL_DUCKING) )
		return false;

	// Agachados y moviendonos
	if ( GetOuterXYSpeed() > ANIM_WALK_SPEED )
		idealActivity = ACT_MP_CROUCHWALK;

	// Solo agachados
	else
		idealActivity = ACT_MP_CROUCH_IDLE;
		
	return true;
}

//====================================================================
//====================================================================
void CInPlayerAnimState::GrabEarAnimation()
{
	// TODO
}

//====================================================================
// Inicializa los parametros de detección de animación.
//====================================================================
bool CInPlayerAnimState::SetupPoseParameters( CStudioHdr *pStudioHdr )
{
	// Ya hemos inicializado esto.
	if ( m_bPoseParameterInit )
		return true;

	// Save off the pose parameter indices.
	if ( !pStudioHdr )
		return false;

	// Parámetros de movimiento (pies)
	m_PoseParameterData.m_iMoveX = GetBasePlayer( )->LookupPoseParameter( pStudioHdr, ANIM_MOVE_X );
	m_PoseParameterData.m_iMoveY = GetBasePlayer( )->LookupPoseParameter( pStudioHdr, ANIM_MOVE_Y );

	if ( ( m_PoseParameterData.m_iMoveX < 0 ) || ( m_PoseParameterData.m_iMoveY < 0 ) )
		return false;

	// Movimiento del torso (arriba/abajo)
	m_PoseParameterData.m_iAimPitch = GetBasePlayer()->LookupPoseParameter(pStudioHdr, ANIM_AIM_PITCH);

	if ( m_PoseParameterData.m_iAimPitch < 0 )
		return false;

	// Movimiento del torso (rotación)
	m_PoseParameterData.m_iAimYaw = GetBasePlayer()->LookupPoseParameter(pStudioHdr, ANIM_AIM_YAW);

	if ( m_PoseParameterData.m_iAimYaw < 0 )
		return false;

	m_bPoseParameterInit = true;
	return true;
}

//====================================================================
// 
//====================================================================
void CInPlayerAnimState::ComputePoseParam_AimPitch(CStudioHdr *pStudioHdr)
{
	// Get the view pitch.
	float flAimPitch = m_flEyePitch;
	//m_angAiming.x = Approach(flAimPitch, m_angAiming.x, gpGlobals->frametime * 110.0f);

	// Set the aim pitch pose parameter and save.
	GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iAimPitch, flAimPitch);
	m_DebugAnimData.m_flAimPitch = flAimPitch;
}

//====================================================================
//====================================================================
void CInPlayerAnimState::ComputePoseParam_AimYaw(CStudioHdr *pStudioHdr)
{
	// Get the movement velocity.
	Vector vecVelocity;
	GetOuterAbsVelocity(vecVelocity);

	// Check to see if we are moving.
	bool bMoving = ( vecVelocity.Length() > 1.0f ) ? true : false;

	// If we are moving or are prone and undeployed.
	if ( bMoving || m_bForceAimYaw )
	{
		// The feet match the eye direction when moving - the move yaw takes care of the rest.
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	// Else if we are not moving.
	else
	{
		// Initialize the feet.
		if ( m_PoseParameterData.m_flLastAimTurnTime <= 0.0f )
		{
			m_flGoalFeetYaw	= m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		}
		// Make sure the feet yaw isn't too far out of sync with the eye yaw.
		// TODO: Do something better here!
		else
		{
			float flYawDelta = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

			if ( fabs( flYawDelta ) > 45.0f )
			{
				float flSide = ( flYawDelta > 0.0f ) ? -1.0f : 1.0f;
				m_flGoalFeetYaw += ( 45.0f * flSide );
			}
		}
	}

	// Fix up the feet yaw.
	m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );
	if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
	{
		if ( m_bForceAimYaw )
		{
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
		}
		else
		{
			ConvergeYawAngles( m_flGoalFeetYaw, 720.0f, gpGlobals->frametime, m_flCurrentFeetYaw );
			m_flLastAimTurnTime = gpGlobals->curtime;
		}
	}

	// Rotate the body into position.
	if ( !GetPlayer()->IsIncap() )
		m_angRender[YAW] = m_flCurrentFeetYaw;

	// Find the aim(torso) yaw base on the eye and feet yaws.
	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize( flAimYaw );

	// Set the aim yaw and save.
	//m_angAiming.y = Approach(flAimYaw, m_angAiming.y, gpGlobals->frametime * 110.0f);
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, flAimYaw );
	m_DebugAnimData.m_flAimYaw	= flAimYaw;

	// Turn off a force aim yaw - either we have already updated or we don't need to.
	m_bForceAimYaw = false;

#ifndef CLIENT_DLL
	if ( !GetPlayer()->IsIncap() )
	{
		QAngle angle = GetBasePlayer()->GetAbsAngles();
		angle[YAW] = m_flCurrentFeetYaw;

		GetBasePlayer()->SetAbsAngles( angle );
	}
#endif
}