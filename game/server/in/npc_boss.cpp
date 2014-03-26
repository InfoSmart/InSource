//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "npc_boss.h"

#include "players_manager.h"
#include "in_utils.h"
#include "in_player.h"

#include "director.h"

#include "in_gamerules.h"

#ifdef APOCALYPSE
	#include "ap_director.h"
#endif

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

ConVar boss_health( "sk_boss_health", "300", FCVAR_SERVER, "Salud de los jefes" );
ConVar boss_damage( "sk_boss_damage", "15", FCVAR_SERVER, "Daño causado por el jefe" );
ConVar boss_allow_jump( "sk_boss_allow_jump", "1", FCVAR_SERVER | FCVAR_CHEAT, "Indica si el infectado puede saltar" );
ConVar boss_slash_distance( "sk_boss_slash_distance", "160", FCVAR_SERVER, "" );

//=========================================================
// Configuración
//=========================================================

// Capacidades
// Moverse en el suelo - Ataque cuerpo a cuerpo 1 - Ataque cuerpo a cuerpo 2 - Saltar (No completo) - Girar la cabeza
#define BOSS_CAPABILITIES		bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | \
								bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB | bits_CAP_ANIMATEDFACE | \
								bits_CAP_TURN_HEAD | bits_CAP_FRIENDLY_DMG_IMMUNE

// Color de la sangre.
#define BOSS_BLOOD				BLOOD_COLOR_RED

// Campo de visión
#define BOSS_FOV				0.55

// Modelo
#define BOSS_MODEL				"models/infected/hulk.mdl"

//=========================================================
// Eventos de animación
//=========================================================

int AE_FOOTSTEP_RIGHT;
int AE_FOOTSTEP_LEFT;
int AE_ATTACK_HIT;

//=========================================================
// Tareas programadas
//=========================================================
enum
{
	SCHED_BOSS_ATTACK1 = LAST_SHARED_SCHEDULE,
	LAST_BOSS_SCHEDULE
};

//=========================================================
// Condiciones
//=========================================================

//=========================================================
// Animaciones
//=========================================================

Activity ACT_HULK_ATTACK_LOW;
Activity ACT_HULK_THROW;
Activity ACT_TERROR_HULK_VICTORY;

//=========================================================
// Información
//=========================================================

LINK_ENTITY_TO_CLASS( npc_boss, CBoss );

BEGIN_DATADESC( CBoss )
	DEFINE_FIELD( m_bCanAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bISeeAEnemy, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flNextPainSound, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextAlertSound, FIELD_FLOAT ),

	DEFINE_FIELD( m_nAttackStress, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iAttackLayer, FIELD_INTEGER ),
END_DATADESC()

//=========================================================
// Nacimiento
//=========================================================
void CBoss::Spawn()
{
	// Guardamos en caché...
	Precache();

	// Modelo
	SetModel( BOSS_MODEL );
	SetupGlobalModelData();

	// Spawn
	BaseClass::Spawn();

	// Color de sangre
	SetBloodColor( BOSS_BLOOD );

	// Tamaño
	SetHullType( HULL_MEDIUM );
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
	CapabilitiesAdd( BOSS_CAPABILITIES );

	// Más información
	m_iHealth		= boss_health.GetInt();
	m_flFieldOfView = BOSS_FOV;

	m_bCanAttack	= true;
	m_bISeeAEnemy	= false;
	m_iAttackLayer	= -1;
	m_nAttackStress.Invalidate();

	NPCInit();

	// Distancia de visión
	m_flDistTooFar = 3512.0;
	SetDistLook( 3024.0 );
}

//=========================================================
// Guardado de objetos necesarios en caché
//=========================================================
void CBoss::Precache()
{
	BaseClass::Precache();

	// Modelo
	PrecacheModel( BOSS_MODEL );

	// Sonidos
	PrecacheScriptSound("Boss.Idle");
	PrecacheScriptSound("Boss.Pain");
	PrecacheScriptSound("Boss.Alert");
	PrecacheScriptSound("Boss.Death");
	PrecacheScriptSound("Boss.HighAttack");
	PrecacheScriptSound("Boss.LowAttack");
	PrecacheScriptSound("Boss.Step");
	PrecacheScriptSound("Boss.Fail");
	PrecacheScriptSound("Boss.Yell");
}

//=========================================================
// Bucle de ejecución de tareas
//=========================================================
void CBoss::Think()
{
	// Think
	BaseClass::Think();

	// Hay un Director
	if ( Director )
	{
		UpdateStress();
		UpdateEnemyChase();
	}

	CAnimationLayer *pLayer = GetAnimOverlay( m_iAttackLayer );

	if ( pLayer && pLayer->m_bSequenceFinished )
	{
		m_iAttackLayer = -1;
		MeleeAttack();
	}

	// ¡No sabemos nadar!
	// TODO: Animación de "ahogo"
	if ( GetWaterLevel() >= WL_Waist )
	{
		SetBloodColor( DONT_BLEED );
		TakeDamage( CTakeDamageInfo(this, this, GetMaxHealth() + 100, DMG_GENERIC) );
	}
}

//=========================================================
// Actualiza el estres por no atacar a un jugador
//=========================================================
void CBoss::UpdateStress()
{
	// No hemos visto a nadie
	if ( !m_bISeeAEnemy )
		return;

	// Aún no nos hemos estresado por no atacar a nadie
	if ( !m_nAttackStress.IsElapsed() || !m_nAttackStress.HasStarted() )
		return;

	// Estamos en la vista de un Jugador
	if ( PlysManager->InVisibleCone(this) )
		return;

	// Obtenemos la distancia al Jugador más cercano
	float flDistance = 0.0f;
	PlysManager->GetNear( GetAbsOrigin(), flDistance );

	// No estamos lo suficientemente lejos
	if ( flDistance <= 1000 )
		return;

	// Has muerto/escapado por estres...
	UTIL_Remove( this );

	// Necesitamos otro
	Director->TryBoss();
}

//=========================================================
// Actualiza el conocimiento de los enemigos
//=========================================================
void CBoss::UpdateEnemyChase()
{
	// No hemos visto a nadie
	if ( !m_bISeeAEnemy && !InRules->IsGameMode(GAME_MODE_SURVIVAL_TIME) )
		return;

	// Ya tenemos a un enemigo
	if ( GetEnemy() && GetEnemy()->IsPlayer() )
	{
		UpdateEnemyMemory( GetEnemy(), GetEnemy()->GetAbsOrigin() );
	}

	CIN_Player *pPlayer = PlysManager->GetRandom();

	// Este es tu nuevo enemigo.
	if ( pPlayer && pPlayer->IsAlive() )
	{
		UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin() );
	}
}

//=========================================================
// Devuelve la clase de NPC
//=========================================================
Class_T CBoss::Classify()
{
	return CLASS_BOSS;
}

//=========================================================
//=========================================================
void CBoss::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->Event() == AE_ATTACK_HIT )
	{
		MeleeAttack();
		m_bCanAttack = true;

		return;
	}

	if ( pEvent->Event() == AE_FOOTSTEP_LEFT )
	{
		UTIL_ScreenShake( WorldSpaceCenter(), 5.0f, 2.5f, 1.0f, 800.0f, SHAKE_START );
		//EmitSound( "Boss.Step" );

		return;
	}

	if ( pEvent->Event() == AE_FOOTSTEP_RIGHT )
	{
		UTIL_ScreenShake( WorldSpaceCenter(), 5.0f, 2.5f, 1.0f, 800.0f, SHAKE_START );
		EmitSound( "Boss.Step" );

		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//=========================================================
// Reproduce un sonido de "descanso"
//=========================================================
void CBoss::IdleSound()
{
	EmitSound("Boss.Idle");
}

//=========================================================
// Reproduce un sonido de dolor
//=========================================================
void CBoss::PainSound( const CTakeDamageInfo &info )
{
	if ( gpGlobals->curtime >= m_flNextPainSound )
	{
		EmitSound( "Boss.Pain" );

		// Próxima vez...
		m_flNextPainSound = gpGlobals->curtime + RandomFloat(0.1, 0.5);
	}
}

//=========================================================
// Reproduce un sonido de alerta
//=========================================================
void CBoss::AlertSound()
{
	if ( gpGlobals->curtime >= m_flNextAlertSound )
	{
		EmitSound( "Boss.Alert" );

		// Próxima vez...
		m_flNextAlertSound = gpGlobals->curtime + RandomFloat(.5, 1.0);
	}
}

//=========================================================
// Reproducir sonido de muerte.
//=========================================================
void CBoss::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "Boss.Death" );
}

//=========================================================
// Reproducir sonido al azar de ataque alto/fuerte.
//=========================================================
void CBoss::AttackSound()
{
	EmitSound( "Boss.LowAttack" );
}

//=========================================================
// Devuelve si el NPC puede atacar
//=========================================================
bool CBoss::CanAttack()
{
	// Los Jefes deben esperar 3s
	if ( gpGlobals->curtime <= (m_flLastAttackTime+3.0f) )
		return false;

	CAnimationLayer *pLayer = GetAnimOverlay( m_iAttackLayer );

	if ( !pLayer || m_iAttackLayer <= -1 )
		return true;

	return pLayer->m_bSequenceFinished;
}

//=========================================================
// [Evento] Se ha adquirido un nuevo enemigo
//=========================================================
void CBoss::OnEnemyChanged( CBaseEntity *pOldEnemy, CBaseEntity *pNewEnemy )
{
	if ( !Director )
		return;

	// He encontrado a un Jugador
	if ( !m_bISeeAEnemy && pNewEnemy->IsPlayer() )
	{
		m_bISeeAEnemy = true;

		// No estamos en el Climax
		if ( !Director->Is(CLIMAX) )
		{
			// Pasamos a Jefe
			Director->Boss();

			// Tienes 30s para atacar a un jugador
			m_nAttackStress.Start( 30 );
		}
	}
}

//=========================================================
// Realiza el ataque a una entidad
//=========================================================
CBaseEntity *CBoss::AttackEntity( CBaseEntity *pEntity )
{
	CBaseEntity *pEntityPush = BaseClass::AttackEntity( pEntity );
	return pEntityPush;
}

//=========================================================
// Realiza el ataque a un personaje
//=========================================================
CBaseEntity *CBoss::AttackCharacter( CBaseCombatCharacter *pCharacter )
{
	CBaseEntity *pEntityPush = BaseClass::AttackCharacter( pCharacter );
	PushEntity( pEntityPush );

	return pEntityPush;
}

//=========================================================
//=========================================================
void CBoss::PushEntity( CBaseEntity *pEntity )
{
	if ( !pEntity )
	{
		// Arg...
		EmitSound( "Boss.Fail" );
		return;
	}

	// ¡Yey!
	EmitSound("Boss.Yell");

	// Obtenemos el vector que define "enfrente nuestro".
	Vector forward, up;
	AngleVectors( GetAbsAngles(), &forward, NULL, &up );


	//
	// Es un Jugador
	//
	if ( pEntity->IsPlayer() )
	{
		CIN_Player *pPlayer = ToInPlayer( pEntity );

		if ( !pPlayer )
			return;

		// Tienes 30s para volver a atacar
		m_nAttackStress.Start( 30 );

		
		if ( !pPlayer->IsDejected() )
		{
			// Establecemos el impulso del golpe.
			Vector vecImpulse = 300 * (up + 2 * forward);

			// Lanzarlo por los aires
			pEntity->ApplyAbsVelocityImpulse( vecImpulse );
		}
	}

	//
	// Un NPC (Matarlo)
	//
	else
	{
		CTakeDamageInfo info( this, this, pEntity->GetMaxHealth(), DMG_CRUSH );
		pEntity->TakeDamage( info );
	}
}

//=========================================================
// Devuelve el daño de un ataque
//=========================================================
int CBoss::GetMeleeDamage()
{
	return boss_damage.GetInt();
}

//=========================================================
// Devuelve la distancia de un ataque
//=========================================================
int CBoss::GetMeleeDistance()
{
	return boss_slash_distance.GetInt();
}

//=========================================================
// Calcula si el salto que realizará es válido.
//=========================================================
bool CBoss::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	// No podemos saltar
	if ( !boss_allow_jump.GetBool() )
		return false;

	const float MAX_JUMP_UP			= 100.0f;
	const float MAX_JUMP_DOWN		= 13084.0f;
	const float MAX_JUMP_DISTANCE	= 312.0f;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_UP, MAX_JUMP_DOWN, MAX_JUMP_DISTANCE );
}

//=========================================================
// Traduce una animación a otra
//=========================================================
Activity CBoss::NPC_TranslateActivity( Activity pActivity )
{
	switch ( pActivity )
	{
		// Ataque
		case ACT_MELEE_ATTACK1:
			return ACT_TERROR_ATTACK;
		break;

		// Hemos muerto
		case ACT_DIESIMPLE:

			if ( m_flSpeed >= 200 )
				return ACT_TERROR_DIE_WHILE_RUNNING;

			return ACT_TERROR_DIE_FROM_STAND;

		break;
	}

	return pActivity;
}

//=========================================================
// Comienza la ejecución de una tarea
//=========================================================
void CBoss::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		// Ataque primario
		case TASK_MELEE_ATTACK1:

			// Establecemos la última vez que hemos atacado y
			// reproducimos el gesto de ataque
			SetLastAttackTime( gpGlobals->curtime );
			m_iAttackLayer = AddGesture( ACT_TERROR_ATTACK_MOVING );

		break;

		// ¡Atacad!
		case TASK_ANNOUNCE_ATTACK:
			AttackSound();
		break;

		default:
			BaseClass::StartTask( pTask );
	}
}

//=========================================================
// Ejecuta una tarea y espera a que termine
//=========================================================
void CBoss::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		// Ataque primario
		case TASK_MELEE_ATTACK1:
			TaskComplete();
		break;

		case TASK_ANNOUNCE_ATTACK:
			TaskComplete();
		break;

		default:
			BaseClass::RunTask( pTask );
	}
}

//=========================================================
// Traduce una tarea registrada a otra
//=========================================================
int CBoss::TranslateSchedule( int scheduleType )
{
	switch ( scheduleType )
	{
		case SCHED_MELEE_ATTACK1:
			return SCHED_BOSS_ATTACK1;
		break;

		default:
			return BaseClass::TranslateSchedule( scheduleType );
	}
}

//=========================================================
// Inteligencia artificial personalizada
//=========================================================
AI_BEGIN_CUSTOM_NPC( npc_boss, CBoss )

	DECLARE_ACTIVITY( ACT_HULK_ATTACK_LOW )
	DECLARE_ACTIVITY( ACT_HULK_THROW )
	DECLARE_ACTIVITY( ACT_TERROR_HULK_VICTORY )

	DECLARE_ANIMEVENT( AE_FOOTSTEP_RIGHT )
	DECLARE_ANIMEVENT( AE_FOOTSTEP_LEFT )
	DECLARE_ANIMEVENT( AE_ATTACK_HIT )

	DEFINE_SCHEDULE
	(
		SCHED_BOSS_ATTACK1,

		"	Tasks"
		"		TASK_MELEE_ATTACK1				0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_OCCLUDED"
	)

AI_END_CUSTOM_NPC()