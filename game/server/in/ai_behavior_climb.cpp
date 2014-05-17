//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ai_behavior_climb.h"

#include "ai_moveprobe.h"

//====================================================================
// Comandos
//====================================================================

ConVar ai_climb_behavior_enable( "ai_climb_behavior_enable", "1" );
ConVar ai_climb_behavior_front_distance( "ai_climb_behavior_front_distance", "20" );
ConVar ai_climb_behavior_interval( "ai_climb_behavior_interval", "1.0" );

#define CLIMB_ENABLE	ai_climb_behavior_enable.GetBool()
#define FRONT_DISTANCE	ai_climb_behavior_front_distance.GetInt()
#define CLIMB_INTERVAL	ai_climb_behavior_interval.GetFloat()


//====================================================================
// Información y Red
//====================================================================

BEGIN_DATADESC( CAI_ClimbBehavior )
END_DATADESC()

int m_iClimbHeights[] =
{
	36,
	48,
	60,
	72,
	84,
	96,
	108,
	120,
	132,
	144,
	156,
	168,

	38,
	50,
	70,
	115,
	130,
	150,
	166,
};

#define CLIMB_ACTIVITY( height ) ACT_TERROR_CLIMB_##height##_FROM_STAND

//====================================================================
// Animaciones
//====================================================================

Activity ACT_TERROR_CLIMB_24_FROM_STAND;
Activity ACT_TERROR_CLIMB_36_FROM_STAND;
Activity ACT_TERROR_CLIMB_48_FROM_STAND;
Activity ACT_TERROR_CLIMB_60_FROM_STAND;
Activity ACT_TERROR_CLIMB_72_FROM_STAND;
Activity ACT_TERROR_CLIMB_84_FROM_STAND;
Activity ACT_TERROR_CLIMB_96_FROM_STAND;
Activity ACT_TERROR_CLIMB_108_FROM_STAND;
Activity ACT_TERROR_CLIMB_120_FROM_STAND;
Activity ACT_TERROR_CLIMB_132_FROM_STAND;
Activity ACT_TERROR_CLIMB_144_FROM_STAND;
Activity ACT_TERROR_CLIMB_156_FROM_STAND;
Activity ACT_TERROR_CLIMB_168_FROM_STAND;

Activity ACT_TERROR_CLIMB_38_FROM_STAND;
Activity ACT_TERROR_CLIMB_50_FROM_STAND;
Activity ACT_TERROR_CLIMB_70_FROM_STAND;
Activity ACT_TERROR_CLIMB_115_FROM_STAND;
Activity ACT_TERROR_CLIMB_130_FROM_STAND;
Activity ACT_TERROR_CLIMB_150_FROM_STAND;
Activity ACT_TERROR_CLIMB_166_FROM_STAND;

//====================================================================
// Constructor
//====================================================================
CAI_ClimbBehavior::CAI_ClimbBehavior()
{
	m_flNextClimb = gpGlobals->curtime;
}

//====================================================================
// Devuelve si el NPC choca contra una pared
//====================================================================
bool CAI_ClimbBehavior::HitsWall( float flDistance, int iHeight )
{
	trace_t tr;
	return HitsWall( flDistance, iHeight, &tr );
}

//====================================================================
// Devuelve si el NPC choca contra una pared
//====================================================================
bool CAI_ClimbBehavior::HitsWall( float flDistance, int iHeight, trace_t *trace  )
{
	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward, vecSrc, vecEnd;
	GetOuter()->GetVectors( &vecForward, NULL, NULL );

	vecSrc = GetAbsOrigin() + Vector( 0, 0, iHeight );
	vecEnd = vecSrc + vecForward * flDistance;

	// Trazamos una línea hacia arriba
	/*trace_t trStart;
	AI_TraceHull( GetAbsOrigin(), vecSrc, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, GetOuter(), COLLISION_GROUP_NONE, &trStart );

	// Esta bloqueado, no podemos trepar
	if ( trStart.fraction != 1.0 )
		return false;*/

	// Trazamos una línea hacia nuestro destino
	trace_t	tr;
	AI_TraceHull( vecSrc, vecEnd, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, GetOuter(), COLLISION_GROUP_NONE, &tr );

	trace = &tr;
	
	if ( tr.m_pEnt )
	{
		// No podemos trepar sobre un personaje
		if ( !tr.m_pEnt->IsWorld() )
			return false;
	}

	return ( tr.fraction < 1 || tr.startsolid );
}

//====================================================================
// Devuelve la animación para escalar la altura definida
//====================================================================
Activity CAI_ClimbBehavior::GetClimbActivity( int iHeight )
{
	switch ( iHeight )
	{
		case 24:
			return CLIMB_ACTIVITY( 24 );
		break;

		case 36:
			return CLIMB_ACTIVITY( 36 );
		break;

		case 38:
			return CLIMB_ACTIVITY( 38 );
		break;

		case 48:
			return CLIMB_ACTIVITY( 48 );
		break;

		case 50:
			return CLIMB_ACTIVITY( 50 );
		break;

		case 60:
			return CLIMB_ACTIVITY( 60 );
		break;

		case 70:
			return CLIMB_ACTIVITY( 70 );
		break;

		case 72:
			return CLIMB_ACTIVITY( 72 );
		break;

		case 84:
			return CLIMB_ACTIVITY( 84 );
		break;

		case 96:
			return CLIMB_ACTIVITY( 96 );
		break;

		case 108:
			return CLIMB_ACTIVITY( 108 );
		break;

		case 115:
			return CLIMB_ACTIVITY( 115 );
		break;

		case 120:
			return CLIMB_ACTIVITY( 120 );
		break;

		case 130:
			return CLIMB_ACTIVITY( 130 );
		break;

		case 132:
			return CLIMB_ACTIVITY( 132 );
		break;

		case 144:
			return CLIMB_ACTIVITY( 144 );
		break;

		case 150:
			return CLIMB_ACTIVITY( 150 );
		break;

		case 156:
			return CLIMB_ACTIVITY( 156 );
		break;

		case 166:
			return CLIMB_ACTIVITY( 166 );
		break;

		default:
			return CLIMB_ACTIVITY( 168 );
		break;
	}
}

//====================================================================
// Devuelve si es necesario escalar una pared
//====================================================================
bool CAI_ClimbBehavior::CanClimb()
{
	// No podemos
	if ( !CLIMB_ENABLE )
		return false;

	// Ya estamos escalando
	if ( IsClimbing() || HasCondition(COND_CLIMB_START) )
		return false;

	// Aún no podemos escalar
	if ( gpGlobals->curtime <= m_flNextClimb )
		return false;

	// No hay ningún bloqueo
	if ( !HitsWall(FRONT_DISTANCE, 10) )
		return false;

	bool bCanClimb = false;

	// Verificamos de que altura es la pared
	for ( int i = 0; i < ARRAYSIZE(m_iClimbHeights); ++i )
	{
		// Probamos con esta altura
		int iHeight			= m_iClimbHeights[i];
		int iCheckHeight	= ( iHeight - 12 ) + 3;
		trace_t tr;

		// Esta altura esta bloqueada también, la pared es más grande
		if ( HitsWall(FRONT_DISTANCE, iCheckHeight, &tr) )
			continue;

		Activity iActivity = GetClimbActivity( iHeight );

		// No tenemos esta animación
		if ( GetOuter()->SelectWeightedSequence(iActivity) == ACTIVITY_NOT_AVAILABLE )
			continue;

		// TODO
		//if ( tr.plane.normal == vec3_origin )
			//continue;

		// ¡Aquí termina! Podemos escalar a esta altura
		bCanClimb		= true;
		m_iClimbHeight	= iHeight;
		m_flClimbYaw	= UTIL_VecToYaw( tr.plane.normal * -1 );

		break;
	}

	return bCanClimb;
}

//====================================================================
// Verifica si es necesario escalar un muro
//====================================================================
void CAI_ClimbBehavior::UpdateClimb()
{
	// No podemos escalar ahora
	if ( !CanClimb() )
		return;

	SetCondition( COND_CLIMB_START );
}

//====================================================================
// Prepara todo para empezar a escalar
//====================================================================
void CAI_ClimbBehavior::MoveClimbStart()
{
	// Volteamos hacia la pared
	GetMotor()->SetIdealYawAndUpdate( m_flClimbYaw );
	DevMsg( "m_flClimbYaw: %f \n", m_flClimbYaw );

	// Nos movemos un poco más adelante
	Vector vecForward;
	GetOuter()->GetVectors( &vecForward, NULL, NULL );
	GetOuter()->SetAbsOrigin( GetAbsOrigin() + vecForward * 8 );

	// Establecemos la animación adecuada
	GetOuter()->SetIdealActivity( GetClimbActivity(m_iClimbHeight) );
	m_bIsClimbing = true;

	// No debemos tener gravedad
	GetOuter()->AddFlag( FL_FLY );
	SetSolid( SOLID_BBOX );
	SetGravity( 0.0 );
	SetGroundEntity( NULL );
}

//====================================================================
// Prepara todo para dejar de escalar
//====================================================================
void CAI_ClimbBehavior::MoveClimbStop()
{
	// Restauramos la gravedad
	GetOuter()->RemoveFlag( FL_FLY );
	SetGravity( 1.0 );

	m_flNextClimb	= gpGlobals->curtime + CLIMB_INTERVAL;
	m_bIsClimbing	= false;
	m_flClimbYaw	= -1;

	// Limpiamos la condición
	ClearCondition( COND_CLIMB_START );
}

//====================================================================
// Procesa el movimiento al escalar
//====================================================================
void CAI_ClimbBehavior::MoveClimbExecute()
{
	// Seguimos ejecutando el movimiento de la animación
	GetOuter()->AutoMovement();
}

//====================================================================
//====================================================================
void CAI_ClimbBehavior::GatherConditions()
{
	UpdateClimb();

	BaseClass::GatherConditions();
}

//====================================================================
//====================================================================
void CAI_ClimbBehavior::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		//
		// Empezamos a trepar
		//
		case TASK_CLIMB_START:
		{
			MoveClimbStart();
			break;
		}

		default:
		{
			BaseClass::StartTask( pTask );
			break;
		}
	}
}

//====================================================================
// Ejecuta una tarea y espera a que termine
//====================================================================
void CAI_ClimbBehavior::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_CLIMB_START:
		{
			MoveClimbExecute();

			// La animación ha terminado
			if ( GetOuter()->IsActivityFinished() )
			{
				MoveClimbStop();
				TaskComplete();
			}

			break;
		}

		default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//====================================================================
// Selecciona una tarea
//====================================================================
int CAI_ClimbBehavior::SelectSchedule()
{
	//
	// Debemos trepar
	//
	if ( HasCondition(COND_CLIMB_START) )
	{
		return SCHED_CLIMB_START;
	}

	return BaseClass::SelectSchedule();
}

//====================================================================
//====================================================================

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_ClimbBehavior )

	DECLARE_CONDITION( COND_CLIMB_START )
	DECLARE_TASK( TASK_CLIMB_START )

	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_36_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_48_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_60_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_72_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_84_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_96_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_108_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_120_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_132_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_144_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_156_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_168_FROM_STAND )

	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_38_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_50_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_70_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_115_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_130_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_150_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_166_FROM_STAND )

	DEFINE_SCHEDULE
	(
		SCHED_CLIMB_START,

		"	Tasks"
		"		 TASK_STOP_MOVING	  0"
		"		 TASK_CLIMB_START	  0"
		"		 TASK_STOP_MOVING	  0"
		""
		"	Interrupts"
	)

AI_END_CUSTOM_SCHEDULE_PROVIDER()