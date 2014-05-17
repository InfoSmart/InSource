//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"

#include "ai_basenpc.h"
#include "ai_navigator.h"
#include "ai_route.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//====================================================================
// Comandos
//====================================================================

ConVar pigeon_health( "sk_pigeon_health", "1" );

//====================================================================
// Un pajaro
//====================================================================
class CPigeon : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CPigeon, CAI_BaseNPC );

	// Principales
	virtual void Spawn();
	virtual void Precache();

	Class_T Classify();

	virtual void HandleAnimEvent( animevent_t *pEvent );
	virtual void OnChangeActivity( Activity eNewActivity );

	virtual bool OverrideMove( float flInterval );
};

//====================================================================
// Configuración
//====================================================================

#define PIGEON_BLOOD		BLOOD_COLOR_RED
#define PIGEON_MODEL		"models/pigeon.mdl"

//====================================================================
// Tareas
//====================================================================
enum
{
	//TASK_INFECTED_LOW_ATTACK
};

//====================================================================
// Tareas programadas
//====================================================================
enum
{
	//SCHED_INFECTED_MELEE_ATTACK1
};

//====================================================================
// Condiciones
//====================================================================
enum
{
	//COND_INFECTED_CAN_RELAX
};

//====================================================================
// Animaciones
//====================================================================

Activity ACT_CROW_TAKEOFF;
Activity ACT_CROW_SOAR;
Activity ACT_CROW_LAND;

int AE_CROW_TAKEOFF;
int AE_CROW_HOP;
int AE_CROW_FLY;

//====================================================================
// Información
//====================================================================

LINK_ENTITY_TO_CLASS( npc_pigeon, CPigeon );

//====================================================================
// Nacimiento
//====================================================================
void CPigeon::Spawn()
{
	// Guardamos en caché...
	Precache();

	// Establecemos el modelo
	SetModel( PIGEON_MODEL );

	// Color de sangre
	SetBloodColor( PIGEON_BLOOD );

	// Tamaño
	SetHullType( HULL_TINY );
	SetHullSizeNormal();

	// Navegación
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

	// Capacidades
	CapabilitiesClear();

	// Más información
	m_iHealth		= pigeon_health.GetInt();
	m_flFieldOfView = VIEW_FIELD_FULL;

	AddSpawnFlags( SF_NPC_FADE_CORPSE );
	SetViewOffset( Vector(6, 0, 11) );

	NPCInit();
}

//====================================================================
// Guardado de objetos necesarios en caché
//====================================================================
void CPigeon::Precache()
{
	PrecacheModel( PIGEON_MODEL );
}

//====================================================================
// Devuelve la clase de NPC
//====================================================================
Class_T CPigeon::Classify()
{
	return CLASS_EARTH_FAUNA;
}

//====================================================================
// Recibe un evento de animación
//====================================================================
void CPigeon::HandleAnimEvent( animevent_t *pEvent )
{
	//
	//
	//
	if ( pEvent->Event() == AE_CROW_TAKEOFF )
	{
		if ( GetNavigator()->GetPath()->GetCurWaypoint() )
			Takeoff( GetNavigator()->GetCurWaypointPos() );

		return;
	}

	//
	//
	//
	if ( pEvent->Event() == AE_CROW_HOP )
	{
		SetGroundEntity( NULL );

		UTIL_SetOrigin( this, GetLocalOrigin() + Vector(0 , 0 , 1) );

		float flHopDistance		= ( m_vSavePosition - GetLocalOrigin() ).Length();
		float flGravity			= GetGravity();

		if ( flGravity <= 1 )
			flGravity = 1;

		float flHeight	= 0.25 * flHopDistance;
		float flSpeed	= sqrt( 2 * flGravity * flHeight );
		float flTime	= flSpeed / flGravity;

		// Scale the sideways velocity to get there at the right time
		Vector vecJumpDir = m_vSavePosition - GetLocalOrigin();
		vecJumpDir = vecJumpDir / flTime;

		// Don't jump too far/fast.
		//
		float flDistance = vecJumpDir.Length();

		if ( flDistance > 650 )
		{
			vecJumpDir = vecJumpDir * ( 650.0 / flDistance );
		}

		//EmitSound( "NPC_Crow.Hop" );
		SetAbsVelocity( vecJumpDir );

		return;
	}

	//
	//
	//
	if ( pEvent->Event() == AE_CROW_FLY )
	{
		SetActivity( ACT_FLY );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

void CPigeon::OnChangeActivity( Activity eNewActivity )
{
	BaseClass::OnChangeActivity( eNewActivity );

	if ( eNewActivity == ACT_FLY )
	{
		SetCycle( RandomFloat( 0.0, 0.75 ) );
	}
}

bool CPigeon::OverrideMove( float flInterval )
{
	if ( GetNavigator()->GetPath()->CurWaypointNavType() == NAV_FLY && GetNavigator()->GetNavType() != NAV_FLY )
		SetNavType( NAV_FLY );

	if ( IsFlying() )
	{
		if ( GetNavigator()->GetPath()->GetCurWaypoint() )
		{
			if ( m_flLastStuckCheck <= gpGlobals->curtime )
			{
				if ( m_vLastStoredOrigin == GetAbsOrigin() )
				{
					if ( GetAbsVelocity() == vec3_origin )
					{
						float flDamage = m_iHealth;

						CTakeDamageInfo	dmgInfo( this, this, flDamage, DMG_GENERIC );
						GuessDamageForce( &dmgInfo, vec3_origin - Vector( 0, 0, 0.1 ), GetAbsOrigin() );

						TakeDamage( dmgInfo );
						return false;
					}
					else
					{
						m_vLastStoredOrigin = GetAbsOrigin();
					}
				}
				else
				{
					m_vLastStoredOrigin = GetAbsOrigin();
				}

				m_flLastStuckCheck = gpGlobals->curtime + 1.0f;
			}

			if ( m_bReachedMoveGoal )
			{
				SetIdealActivity( (Activity)ACT_CROW_LAND );
				SetFlyingState( FlyState_Landing );
				TaskMovementComplete();
			}
			else
			{
				SetIdealActivity( ACT_FLY );
				MoveCrowFly( flInterval );
			}

		}
		else if ( !GetTask() || GetTask()->iTask == TASK_WAIT_FOR_MOVEMENT )
		{
			SetSchedule( SCHED_CROW_IDLE_FLY );
			SetFlyingState( FlyState_Flying );
			SetIdealActivity ( ACT_FLY );
		}

		return true;
	}

	return false;
}