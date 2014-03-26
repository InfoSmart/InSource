//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "players_manager.h"
#include "in_utils.h"
#include "in_player.h"
#include "in_gamerules.h"

#include "ai_basenpc.h"
#include "ai_hint.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_default.h"
#include "ai_memory.h"

#include "npcevent.h"
#include "activitylist.h"

#include "soundent.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Comandos
//=========================================================

ConVar chicken_health( "sk_chicken_health", "8", FCVAR_SERVER, "Salud de los pollos" );

//=========================================================
// Configuración
//=========================================================

// Capacidades
#define CHICKEN_CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP

// Color de la sangre.
#define CHICKEN_BLOOD			BLOOD_COLOR_RED

// Campo de visión
#define CHICKEN_FOV				0.95

// Modelo
#define CHICKEN_MODEL			"models/chicken/chicken.mdl"

//=========================================================
// Tareas programadas
//=========================================================
enum
{
	SCHED_CHICKEN_WANDER = LAST_SHARED_SCHEDULE,

	LAST_CHICKEN_SCHEDULE
};

//=========================================================
// >> CNPC_Chicken
//
// Una gallina: sirve como ambientación, comida o distracción
//=========================================================
class CNPC_Chicken : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_Chicken, CAI_BaseNPC );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	// Principales
	virtual void Spawn();
	virtual void Precache();

	// Sonidos
	virtual void IdleSound();
	virtual void PainSound( const CTakeDamageInfo &info );
	virtual void AlertSound();
	virtual void DeathSound( const CTakeDamageInfo &info );

	Class_T Classify();

	// Salud/Daño
	virtual void Event_Killed( const CTakeDamageInfo &info );

	// Animaciones
	virtual bool IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;

	// Tareas
	virtual int SelectSchedule();
};

//=========================================================
// Información
//=========================================================

LINK_ENTITY_TO_CLASS( npc_chicken, CNPC_Chicken );

BEGIN_DATADESC( CNPC_Chicken )
END_DATADESC()

//=========================================================
// Nacimiento
//=========================================================
void CNPC_Chicken::Spawn()
{
	// Guardamos en caché...
	Precache();

	// Modelo
	SetModel( CHICKEN_MODEL );

	// Spawn
	BaseClass::Spawn();

	// Color de sangre
	SetBloodColor( CHICKEN_BLOOD );

	// Tamaño
	SetHullType( HULL_TINY );
	SetHullSizeNormal();
	SetDefaultEyeOffset();

	// Propiedes fisicas
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	// Navegación
	SetNavType( NAV_GROUND );
	SetMoveType( MOVETYPE_STEP );

	// Capacidades
	CapabilitiesClear();
	CapabilitiesAdd( CHICKEN_CAPABILITIES );

	// Más información
	m_iHealth		= chicken_health.GetInt();
	m_flFieldOfView = CHICKEN_FOV;

	NPCInit();
}

//=========================================================
// Guardado de objetos necesarios en caché
//=========================================================
void CNPC_Chicken::Precache()
{
	BaseClass::Precache();

	// Modelo
	PrecacheModel( CHICKEN_MODEL );

	// Sonidos
	PrecacheScriptSound("Chicken.Idle");
	PrecacheScriptSound("Chicken.Panic");
	PrecacheScriptSound("Chicken.Pain");
	PrecacheScriptSound("Chicken.Death");
}

//=========================================================
// Devuelve la clase de NPC
//=========================================================
Class_T CNPC_Chicken::Classify()
{
	return CLASS_EARTH_FAUNA;
}

void CNPC_Chicken::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	// @FIXME
	UTIL_Remove( this );
}

//=========================================================
// Reproduce un sonido de "descanso"
//=========================================================
void CNPC_Chicken::IdleSound()
{
	EmitSound("Chicken.Idle");
}

//=========================================================
// Reproduce un sonido de dolor
//=========================================================
void CNPC_Chicken::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "Chicken.Pain" );
}

//=========================================================
// Reproduce un sonido de alerta
//=========================================================
void CNPC_Chicken::AlertSound()
{
	EmitSound( "Chicken.Panic" );
}

//=========================================================
// Reproducir sonido de muerte.
//=========================================================
void CNPC_Chicken::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "Chicken.Death" );
}

//=========================================================
// Calcula si el salto que realizará es válido.
//=========================================================
bool CNPC_Chicken::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	const float MAX_JUMP_UP			= 50.0f;
	const float MAX_JUMP_DOWN		= 500.0f;
	const float MAX_JUMP_DISTANCE	= 112.0f;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_UP, MAX_JUMP_DOWN, MAX_JUMP_DISTANCE );
}

//=========================================================
// Selecciona una tarea
//=========================================================
int CNPC_Chicken::SelectSchedule()
{
	switch ( m_NPCState )
	{
		case NPC_STATE_IDLE:
			return SCHED_CHICKEN_WANDER;
		break;

		default:
			return BaseClass::SelectSchedule();
	}
	COND_BETTER_WEAPON_AVAILABLE;
}

//=========================================================
// Inteligencia artificial personalizada
//=========================================================
AI_BEGIN_CUSTOM_NPC( npc_chicken, CNPC_Chicken )

	DEFINE_SCHEDULE
	(
		SCHED_CHICKEN_WANDER,

		"	Tasks"
		"		TASK_STOP_MOVING							0"			// Paramos de movernos
		"		TASK_WANDER									320432"		// Buscamos una ubicación al azar
		"		TASK_WALK_PATH								0"			// Caminamos hacia esa ubicación
		"		TASK_WAIT_FOR_MOVEMENT						0"			// Esperamos a que llegue a la ubicación.
		"		TASK_STOP_MOVING							0"			// Paramos de movernos
		"		TASK_WAIT_PVS								0"			//
		"		TASK_WAIT									10"			// Esperamos 10s sin hacer nada
		"		TASK_SET_SCHEDULE							SCHEDULE:SCHED_CHICKEN_WANDER"		// Volvemos a repetir.
		"	"
		"	Interrupts"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_SEE_FEAR"
	)

AI_END_CUSTOM_NPC()