//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "npc_infected.h"

#include "in_utils.h"
#include "ap_director.h"

#include "in_player.h"

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

//====================================================================
// Comandos
//====================================================================

ConVar infected_health( "sk_infected_health", "3", FCVAR_SERVER, "Salud de los infectados" );
ConVar infected_damage( "sk_infected_damage", "2", FCVAR_SERVER, "Daño causado por el infectado" );
ConVar infected_allow_jump( "sk_infected_allow_jump", "1", FCVAR_SERVER | FCVAR_CHEAT, "Indica si el infectado puede saltar" );
ConVar infected_slash_distance( "sk_infected_slash_distance", "50", FCVAR_SERVER, "" );

//====================================================================
// Configuración
//====================================================================

// Descomente en Release
#define INFECTED_USE_ALL_MODELS
#define INFECTED_USE_NEWMODELS

// Capacidades
// Moverse en el suelo - Ataque cuerpo a cuerpo 1 - Ataque cuerpo a cuerpo 2 - Saltar (No completo) - Girar la cabeza
#define INFECTED_CAPABILITIES	bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | \
								bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB | bits_CAP_ANIMATEDFACE | \
								bits_CAP_TURN_HEAD

// Color de la sangre.
#define INFECTED_BLOOD			BLOOD_COLOR_RED

// Campo de visión
#define INFECTED_FOV					0.75

// Skin
#define INFECTED_SKIN_COUNT				31
#define INFECTED_SKIN_HEAD_COUNT		2
#define INFECTED_SKIN_UPPERBODY_COUNT	3
#define INFECTED_SKIN_LOWERBODY_COUNT	1

// Otros...
#define	INFECTED_GIB_MODEL "models/infected/gibs/gibs.mdl"

#define RELAX_TIME_INTERVAL		gpGlobals->curtime + RandomInt( 10, 130 )
#define SLEEP_TIME_INTERVAL		gpGlobals->curtime + RandomInt( 30, 180 )
#define STUMBLE_INTERVAL		gpGlobals->curtime + RandomInt(10, 20)

//====================================================================
// Modelos posibles
//====================================================================

const char *m_nInfectedModels[] =
{
	"models/infected/common_male01.mdl",
	"models/infected/common_female01.mdl",
};

//====================================================================
// Eventos de animación
//====================================================================

extern int AE_FOOTSTEP_RIGHT;
extern int AE_FOOTSTEP_LEFT;
extern int AE_ATTACK_HIT;

//====================================================================
// Tareas
//====================================================================
enum
{
	TASK_INFECTED_LOW_ATTACK = LAST_SHARED_TASK + 10,

	TASK_INFECTED_TO_RELAX,
	TASK_INFECTED_RELAX,
	TASK_INFECTED_RELAX_TO_STAND,

	TASK_INFECTED_TO_SLEEP,
	TASK_INFECTED_SLEEP,
	TASK_INFECTED_SLEEP_TO_STAND,

	LAST_INFECTED_TASK
};

//====================================================================
// Tareas programadas
//====================================================================
enum
{
	SCHED_INFECTED_MELEE_ATTACK1 = LAST_SHARED_SCHEDULE + 10,
	SCHED_INFECTED_WANDER,
	SCHED_INFECTED_ACQUIRE_ANIM,

	SCHED_INFECTED_TO_RELAX,
	SCHED_INFECTED_RELAX,
	SCHED_INFECTED_RELAX_TO_STAND,

	SCHED_INFECTED_TO_SLEEP,
	SCHED_INFECTED_SLEEP,
	SCHED_INFECTED_SLEEP_TO_STAND,

	SCHED_INFECTED_CHASE_ENEMY_FAILED,
	SCHED_INFECTED_STUMBLE,

	LAST_INFECTED_SCHEDULE
};

//====================================================================
// Condiciones
//====================================================================
enum
{
	COND_INFECTED_CAN_RELAX = LAST_SHARED_CONDITION + 10,
	COND_INFECTED_CAN_RELAX_TO_STAND,

	COND_INFECTED_ACQUIRE_ANIM,

	COND_INFECTED_CAN_SLEEP,
	COND_INFECTED_CAN_SLEEP_TO_STAND,

	COND_INFECTED_CAN_CLIMB,
	COND_INFECTED_CAN_STUMBLE,

	LAST_INFECTED_CONDITION
};

//====================================================================
// Animaciones
//====================================================================

Activity ACT_TERROR_LIE_FROM_STAND;
Activity ACT_TERROR_LIE_IDLE;
Activity ACT_TERROR_LIE_TO_STAND;
Activity ACT_TERROR_LIE_TO_STAND_ALERT;
Activity ACT_TERROR_LIE_TO_SIT;
Activity ACT_TERROR_SIT_TO_LIE;

Activity ACT_TERROR_ATTACK_DOOR_CONTINUOUSLY;
Activity ACT_TERROR_ATTACK_CONTINUOUSLY;

Activity ACT_TERROR_UNABLE_TO_REACH_TARGET;
Activity ACT_TERROR_RUN_STUMBLE;

Activity ACT_TERROR_STADING_ON_FIRE;
Activity ACT_TERROR_RUN_ON_FIRE;

Activity ACT_TERROR_ABOUT_FACE_NEUTRAL;

//====================================================================
// Información
//====================================================================

LINK_ENTITY_TO_CLASS( npc_infected, CInfected );

BEGIN_DATADESC( CInfected )
	DEFINE_FIELD( m_flNextPainSound, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextAlertSound, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAttackIn, FIELD_FLOAT ),

	DEFINE_FIELD( m_iFaceGesture, FIELD_INTEGER ),
END_DATADESC()

//====================================================================
// Nacimiento
//====================================================================
void CInfected::Spawn()
{
	// Guardamos en caché...
	Precache();

	// Seleccionamos un modelo
	SetInfectedModel();

	// Spawn
	BaseClass::Spawn();

	// Color de sangre
	SetBloodColor( INFECTED_BLOOD );

	// Tamaño
	SetHullType( HULL_HUMAN );
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
	CapabilitiesAdd( INFECTED_CAPABILITIES );

	// Más información
	SetIdealHealth( infected_health.GetInt(), 3 );
	m_flFieldOfView = INFECTED_FOV;
	m_iFaceGesture	= -1;

	m_bRelaxing		= false;
	m_bSleeping		= false;
	m_bCanSprint	= true;

	m_iNextRelaxing		= -1;
	m_iNextSleep		= SLEEP_TIME_INTERVAL;

	// Estaremos dormidos hasta que algo nos despierte
	if ( Director->IsStatus(ST_NORMAL) && RandomInt(0, 1) == 0 )
	{
		m_bSleeping		= true;
		m_bCanSprint	= false;

		m_iNextSleep	= -1;
	}
	else
	{
		m_iNextRelaxing = RELAX_TIME_INTERVAL;
	}

	NPCInit();

	// Iván: Esto hace que los NPC no recuerden por donde "fallaron" para llegar al enemigo
	// ocacionando que se queden atorados en ciertas ubicaciones
	//GetNavigator()->SetRememberStaleNodes( false );

	// Distancia de visión
	m_flDistTooFar = 812.0;
	SetDistLook( 824.0 );
}

//====================================================================
// Guardado de objetos necesarios en caché
//====================================================================
void CInfected::Precache()
{
	BaseClass::Precache();

	// Modelos
#ifdef INFECTED_USE_ALL_MODELS
	for ( int i = 0; i < ARRAYSIZE(m_nInfectedModels); ++i )
		PrecacheModel( m_nInfectedModels[i] );
#else
	PrecacheModel( m_nInfectedModels[0] );
#endif

	// Gib
	PrecacheModel( INFECTED_GIB_MODEL );

	// Sonidos
	PrecacheScriptSound("Infected.Idle");
	PrecacheScriptSound("Infected.Alert.Male");
	PrecacheScriptSound("Infected.Alert.Female");
	PrecacheScriptSound("Infected.Attack.Male");
	PrecacheScriptSound("Infected.Attack.Female");
	PrecacheScriptSound("Infected.Attack.Miss");
	PrecacheScriptSound("Infected.Death.Male");
	PrecacheScriptSound("Infected.Death.Female");
	PrecacheScriptSound("Infected.Pain.Male");
	PrecacheScriptSound("Infected.Pain.Female");
	PrecacheScriptSound("Infected.Hit");

	PrecacheEffect("bloodspray");
	PrecacheEffect("bloodimpact");
}

//====================================================================
// Bucle de ejecución de tareas
//====================================================================
void CInfected::NPCThink()
{
	// Think
	BaseClass::NPCThink();

	if ( IsAlive() )
	{
		// Gestos
		UpdateGesture();

		// Verificamos nuestra cabeza
		UpdateFallOn();

		// Verificamos si podemos trepar sobre las paredes
		UpdateClimb();
	}
}

//====================================================================
// Devuelve la clase de NPC
//====================================================================
Class_T CInfected::Classify()
{
	return CLASS_INFECTED;
}

//====================================================================
// Actualiza la animación de "caida"
//====================================================================
void CInfected::UpdateFall()
{
	// No si estamos treparando una pared
	if ( m_nClimbBehavior.IsClimbing() )
		return;

	BaseClass::UpdateFall();
}

//====================================================================
// Verifica si alguien le ha caido encima
//====================================================================
void CInfected::UpdateFallOn()
{
	Vector vecUp;
	GetVectors( NULL, NULL, &vecUp );

	// Trazamos una linea hacia arriba
	trace_t tr;
	AI_TraceLine( WorldSpaceCenter(), WorldSpaceCenter() + vecUp * 100, MASK_NPCSOLID, this, COLLISION_GROUP_NOT_BETWEEN_THEM, &tr );

	// Nuestra cabeza esta bien
	if ( tr.fraction == 1.0 || !tr.m_pEnt )
		return;

	// No es un NPC ni un Jugador
	if ( !tr.m_pEnt->IsNPC() && !tr.m_pEnt->IsPlayer() )
		return;

	// ¡Alguien nos ha caido encima!
	GetNavigator()->StopMoving();
	TakeDamage( CTakeDamageInfo(this, this, 300, DMG_GENERIC) );
}

//====================================================================
// Verifica si se puede trepar sobre una pared
//====================================================================
void CInfected::UpdateClimb()
{
	// No podemos trepar paredes mientras estamos descansando
	if ( IsSleeping() || IsRelaxing() )
		return;

	BaseClass::UpdateClimb();
}

//====================================================================
// Establece el modelo del infectado
//====================================================================
void CInfected::SetInfectedModel()
{
#ifdef INFECTED_USE_ALL_MODELS
	// Seleccionamos un modelo al azar.
	const char *pRandomModel = m_nInfectedModels[ RandomInt(0, ARRAYSIZE(m_nInfectedModels)-1) ];
	SetModel( pRandomModel );
#else
	SetModel( m_nInfectedModels[0] );
#endif

	// Skin de la cara
	m_nSkin = RandomInt( 0, INFECTED_SKIN_COUNT );

	// Seleccionamos: Playera, Manos, Pantalones, etc al azar. ¡Diversidad!
#ifdef INFECTED_USE_NEWMODELS
	SetBodygroup( FindBodygroupByName("Head"), RandomInt(0, INFECTED_SKIN_HEAD_COUNT) );
	SetBodygroup( FindBodygroupByName("UpperBody"), RandomInt(0, INFECTED_SKIN_UPPERBODY_COUNT) );
	SetBodygroup( FindBodygroupByName("LowerBody"), 0 );

	SetClothColor();
#else
	SetBodygroup( FindBodygroupByName("torso"), RandomInt(0, INFECTED_SKIN_TORSO_COUNT) );
	SetBodygroup( FindBodygroupByName("legs"), RandomInt(0, INFECTED_SKIN_LEGS_COUNT) );
	SetBodygroup( FindBodygroupByName("hands"), RandomInt(0, INFECTED_SKIN_HANDS_COUNT) );
	SetBodygroup( FindBodygroupByName("beanies"), RandomInt(0, INFECTED_SKIN_BEANIES_COUNT) );
#endif

	// Configuramos los atajos de movimiento
	SetupGlobalModelData();
}

//====================================================================
// Establece el color de la ropa
//====================================================================
void CInfected::SetClothColor()
{
	int iRand = RandomInt(0, 7);

	switch ( iRand )
	{
		case 0:
			SetRenderColor( 97, 11, 11 );
		break;

		case 1:
			SetRenderColor( 11, 56, 97 );
		break;

		case 2:
			SetRenderColor( 11, 97, 11 );
		break;

		case 3:
			SetRenderColor( 110, 110, 110 );
		break;

		case 4:
			SetRenderColor( 42, 18, 10 );
		break;

		case 5:
			SetRenderColor( 180, 95, 4 );
		break;

		case 6:
			SetRenderColor( 0, 0, 0 );
		break;
	}	
}

//====================================================================
// Actualiza los gestos
//====================================================================
void CInfected::UpdateGesture()
{
	CAnimationLayer *pFace = GetAnimOverlay( m_iFaceGesture );

	if ( pFace && !pFace->m_bSequenceFinished )
		return;

	if ( !GetEnemy() )
		m_iFaceGesture = AddGesture( ACT_EXP_IDLE );
	else
		m_iFaceGesture = AddGesture( ACT_EXP_ANGRY );
}

//====================================================================
// Recibe un evento de animación
//====================================================================
void CInfected::HandleAnimEvent( animevent_t *pEvent )
{
	// Ataque
	if ( pEvent->Event() == AE_ATTACK_HIT )
	{
		AttackSound();
		m_nLastEnemy = MeleeAttack();

		return;
	}

	if ( pEvent->Event() == AE_FOOTSTEP_LEFT )
		return;

	if ( pEvent->Event() == AE_FOOTSTEP_RIGHT )
		return;

	BaseClass::HandleAnimEvent( pEvent );
}

//====================================================================
// Crea "restos" de partes del cuerpo del infectado,
// dependiendo de donde fue el último ataque
//====================================================================
void CInfected::CreateGoreGibs( int iBodyPart, const Vector &vecOrigin, const Vector &vecDir )
{
	CGib *pGib;

	// TODO: Manos
	switch ( iBodyPart )
	{
		// Cabeza
		case HITGROUP_HEAD:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 3 );

			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir, 1 );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 4 );

			break;
		}

		// Pie izquierdo
		case HITGROUP_LEFTLEG:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 2 );

			break;
		}

		// Pie derecho
		case HITGROUP_RIGHTLEG:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 2 );

			break;
		}
	}

	// Creamos de 4 a 10 pedacitos de cuerpo
	int iRand = RandomInt(4, 10);

	for ( int i = 2; i < iRand; ++i )
	{
		pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir, i );
		pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), RandomInt(5, 9) );
	}
}

//====================================================================
// Verifica si el Infectado debe sentarse o levantarse
//====================================================================
void CInfected::UpdateRelaxStatus()
{
	//
	// DORMIR
	//
	if ( m_iNextSleep > -1 || Director->IsStatus(ST_FINALE) || Director->IsStatus(ST_COMBAT) )
	{
		if ( !IsSleeping() )
		{
			// Hora de dormir
			if ( gpGlobals->curtime >= m_iNextSleep && m_NPCState == NPC_STATE_IDLE )
				GoToSleep();
		}
		else
		{
			// Hora de levantarnos
			if ( gpGlobals->curtime >= (m_iNextSleep+130) || GetEnemy() || GetNavigator()->IsGoalSet() )
				GoWake();
		}
	}

	//
	// DESCANSAR / SENTARSE
	//
	if ( m_iNextRelaxing > -1 && !IsSleeping() )
	{
		if ( !IsRelaxing() )
		{
			// Hora de sentarnos y descansar
			if ( gpGlobals->curtime >= m_iNextRelaxing && m_NPCState == NPC_STATE_IDLE )
				GoToRelax();
		}
		else
		{
			// Hora de levantarnos
			if ( gpGlobals->curtime >= (m_iNextRelaxing+25) || GetEnemy() || GetNavigator()->IsGoalSet() )
				GoRelaxToStand();
		}
	}
}

//====================================================================
// Hace que el Infectado se vaya a sentar
//====================================================================
void CInfected::GoToRelax()
{
	// Estamos en proceso
	if ( HasCondition(COND_INFECTED_CAN_RELAX) )
		return;

	// Ya estamos descansando o durmiendo
	if ( m_bRelaxing || m_bSleeping )
		return;

	// Estamos tratando de romper una puerta
	if ( m_nBlockingEnt )
		return;

	SetCondition( COND_INFECTED_CAN_RELAX );
}

//====================================================================
// Hace que el infectado se levante
//====================================================================
void CInfected::GoRelaxToStand()
{
	// Estamos en proceso
	if ( HasCondition(COND_INFECTED_CAN_RELAX_TO_STAND) )
		return;

	// No estamos descansando
	if ( !m_bRelaxing )
		return;

	SetCondition( COND_INFECTED_CAN_RELAX_TO_STAND );
}

//====================================================================
// Hace que el Infectado se vaya a dormir
//====================================================================
void CInfected::GoToSleep()
{
	// Estamos en proceso
	if ( HasCondition(COND_INFECTED_CAN_SLEEP) )
		return;

	// Ya estamos durmiendo
	if ( IsSleeping() )
		return;

	// Estamos tratando de romper una puerta
	if ( m_nBlockingEnt )
		return;

	SetCondition( COND_INFECTED_CAN_SLEEP );
}

//====================================================================
// Hace que el infectado se levante de su sueño
//====================================================================
void CInfected::GoWake()
{
	// Estamos en proceso
	if ( HasCondition(COND_INFECTED_CAN_SLEEP_TO_STAND) )
		return;

	// No estamos durmiendo
	if ( !IsSleeping() )
		return;

	SetCondition( COND_INFECTED_CAN_SLEEP_TO_STAND );
}

//====================================================================
// ¿Podemos reproducir un sonido de descanso?
// Los infectados reproducen este sonido constantemente.
//====================================================================
bool CInfected::ShouldPlayIdleSound()
{
	// Debemos estar en Idle o Alert
	if ( m_NPCState != NPC_STATE_IDLE && m_NPCState != NPC_STATE_ALERT )
		return false;

	// Muy poca probabilidad
	if ( RandomInt(0, 150) != 0 )
		return false;

	// Estamos durmiendo
	if ( IsSleeping() )
		return false;

	return true;
}

//====================================================================
// Reproduce un sonido de "descanso"
//====================================================================
void CInfected::IdleSound()
{
	// Why?
	if ( m_NPCState == NPC_STATE_ALERT )
	{
		AlertSound();
		return;
	}

	EmitSound("Infected.Idle");
}

//====================================================================
// Reproduce un sonido de dolor
//====================================================================
void CInfected::PainSound( const CTakeDamageInfo &info )
{
#ifdef APOCALYPSE
	if ( info.GetAttacker() == this || info.GetInflictor() == this )
		return;
#endif

	if ( gpGlobals->curtime >= m_flNextPainSound )
	{
		EmitSound( UTIL_VarArgs("Infected.Pain.%s", GetGender()) );

		// Próxima vez...
		m_flNextPainSound = gpGlobals->curtime + RandomFloat(0.1, 0.5);
	}
}

//====================================================================
// Reproduce un sonido de alerta
//====================================================================
void CInfected::AlertSound()
{
	if ( gpGlobals->curtime >= m_flNextAlertSound )
	{
		EmitSound( UTIL_VarArgs("Infected.Alert.%s", GetGender()) );

		// Próxima vez...
		m_flNextAlertSound = gpGlobals->curtime + RandomFloat(.5, 1.0);
	}
}

//====================================================================
// Reproducir sonido de muerte.
//====================================================================
void CInfected::DeathSound( const CTakeDamageInfo &info )
{
#ifdef APOCALYPSE
	if ( info.GetAttacker() == this || info.GetInflictor() == this )
		return;
#endif

	EmitSound( UTIL_VarArgs("Infected.Death.%s", GetGender()) );
}

//====================================================================
// Reproducir sonido al azar de ataque alto/fuerte.
//====================================================================
void CInfected::AttackSound()
{
	EmitSound( UTIL_VarArgs("Infected.Attack.%s", GetGender()) );
}

//====================================================================
// Devuelve si el NPC puede atacar
//====================================================================
bool CInfected::CanAttack()
{
	// Estamos relajados, no podemos atacar
	if ( IsRelaxing() || IsSleeping() )
		return false;

	// Estamos en llamas
	if ( IsOnFire() )
		return false;

	// El gesto de ataque no ha acabado
	if ( IsPlayingGesture(ACT_TERROR_ATTACK) )
		return false;

	return true;
}

//====================================================================
// [Evento] Se ha adquirido un nuevo enemigo
//====================================================================
void CInfected::OnEnemyChanged( CBaseEntity *pOldEnemy, CBaseEntity *pNewEnemy )
{
	if ( !pNewEnemy )
		return;

	// Si estabamos dormidos o sentados, hay que pararnos
	if ( IsRelaxing() || IsSleeping() )
	{
		SetState( NPC_STATE_ALERT );

		GoRelaxToStand();
		GoWake();

		return;
	}

#ifdef APOCALYPSE
	// No estamos en Final o Combate
	if ( !Director->IsStatus(ST_FINALE) && !Director->IsStatus(ST_COMBAT) )
	{
		// Nuestro nuevo enemigo es un Jugador
		if ( pNewEnemy->IsPlayer() && RandomInt(0, 1) == 1 )
		{
			// Esta lejos, usamos una animación
			if ( pNewEnemy->GetAbsOrigin().DistTo(GetAbsOrigin()) >= 350 && !HasCondition(COND_INFECTED_ACQUIRE_ANIM) )
			{
				SetCondition(COND_INFECTED_ACQUIRE_ANIM);
			}
		}
	}
#endif
}

//====================================================================
// [Evento] Alguien me ha tocado mientras estaba descansando
//====================================================================
void CInfected::OnTouch( CBaseEntity *pActivator )
{
	if ( !pActivator->IsPlayer() )
		return;

	SetState( NPC_STATE_ALERT );

	GoRelaxToStand();
	GoWake();
}

//====================================================================
// [Evento] Se ha adquirido un nuevo enemigo
//====================================================================
int CInfected::OnTakeDamage( const CTakeDamageInfo &info )
{
	int iResult = BaseClass::OnTakeDamage( info );

	// ¡Arg! Dejenme descansar
	if ( m_bRelaxing )
	{
		GoRelaxToStand();
	}
	if ( m_bSleeping )
	{
		GoWake();
	}

	return iResult;
}

//====================================================================
// [Evento] He muerto
//====================================================================
void CInfected::Event_Killed( const CTakeDamageInfo &info )
{
	Dismembering( info );
	
	// Podemos reproducir la animación de muerte
	if ( CanPlayDeathAnim(info) )
	{
		BaseClass::Event_Killed( info );
		return;
	}

	PreDeath( info );
	StopAnimation();

	if ( CanBecomeRagdoll() )
	{
		// Nos convertimos en un cadaver
		BecomeRagdollOnClient( vec3_origin );
	}

	// Hemos muerto
	m_lifeState	= LIFE_DEAD;

	// Nos eliminamos en 1s
	SetThink( &CAI_BaseNPC::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1.0f );
}

//====================================================================
// Verifica el último daño causado y desmiembra al infectado
//====================================================================
void CInfected::Dismembering( const CTakeDamageInfo &info )
{
	// Solo por daño de bala
	if ( !(info.GetDamageType() & DMG_BULLET) )
		return;

	// Esta vez no...
	if ( RandomInt(0, 2) == 0 )
		return;

	Vector vecDir = info.GetDamageForce();
	Vector vecOrigin;
	QAngle angDir;

	int bloodAttach	= -1;
	int hitBox		= LastHitGroup();

	switch ( hitBox )
	{
			// 3 - derecha
			// 4 - izquierda
			// 

			// 1 - derecha
			// 2 - izquierda

		// Adios cabeza
		case HITGROUP_HEAD:
			SetBodygroup( FindBodygroupByName("Head"), 4 );
			bloodAttach = GetAttachment( "Head", vecOrigin, angDir );
		break;

		// Adios brazo izquierdo
		case HITGROUP_LEFTLEG:
			SetBodygroup( FindBodygroupByName("LowerBody"), 2 );
			bloodAttach = GetAttachment( "severed_LLeg", vecOrigin, angDir );
		break;

		// Adios brazo derecho
		case HITGROUP_RIGHTLEG:
			SetBodygroup( FindBodygroupByName("LowerBody"), 1 );
			bloodAttach = GetAttachment( "severed_RLeg", vecOrigin, angDir );
		break;
	}

	// Puff, seguimos con nuestro cuerpo
	if ( bloodAttach <= -1 )
		return;

	// Soltamos más sangre
	UTIL_BloodSpray( vecOrigin, vecDir, BloodColor(), 18, FX_BLOODSPRAY_GORE );
	UTIL_BloodDrips( vecOrigin, vecDir, BloodColor(), 18 );

	// Sonido Gore
	EmitSound("Infected.Hit");

	// Pintamos sangre en las paredes
	for ( int i = 0 ; i < 30; i++ )
	{
		Vector vecTraceDir = vecDir;
		vecTraceDir.x += random->RandomFloat( -0.5f, 0.5f );
		vecTraceDir.y += random->RandomFloat( -0.5f, 0.5f );
		vecTraceDir.z += random->RandomFloat( -0.5f, 0.5f );
 
		trace_t tr;
		UTIL_TraceLine( vecOrigin, vecOrigin + ( vecTraceDir * 230.0f ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
			UTIL_BloodDecalTrace( &tr, BloodColor() );
	}

	CreateGoreGibs( hitBox, vecOrigin, vecDir );
}

//====================================================================
// Devuelve el daño de un ataque
//====================================================================
int CInfected::GetMeleeDamage()
{
	return infected_damage.GetInt();
}

//====================================================================
// Devuelve la distancia de un ataque
//====================================================================
int CInfected::GetMeleeDistance()
{
	return infected_slash_distance.GetInt();
}

//====================================================================
// Devuelve si puede reproducir la animación de muerte
//====================================================================
bool CInfected::CanPlayDeathAnim( const CTakeDamageInfo &info )
{
	// Estabamos relajandonos
	if ( m_bRelaxing || m_bSleeping )
		return false;

	return BaseClass::CanPlayDeathAnim( info );
}

//====================================================================
// Calcula si el salto que realizará es válido.
//====================================================================
bool CInfected::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	// No podemos saltar
	if ( !infected_allow_jump.GetBool() )
		return false;

	const float MAX_JUMP_UP			= 10.0f;
	const float MAX_JUMP_DOWN		= 11084.0f;
	const float MAX_JUMP_DISTANCE	= 212.0f;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_UP, MAX_JUMP_DOWN, MAX_JUMP_DISTANCE );
}

//====================================================================
// Traduce una animación a otra
//====================================================================
Activity CInfected::NPC_TranslateActivity( Activity pActivity )
{
	switch ( pActivity )
	{
		// Sin hacer nada
		case ACT_IDLE:
		{
			// Estamos en llamas
			if ( IsOnFire() )
				return ACT_TERROR_STADING_ON_FIRE;

			// Estamos en alerta o combate
			if ( m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT )
				return ACT_TERROR_IDLE_ALERT;

			return ACT_TERROR_IDLE_NEUTRAL;
			break;
		}
		
		// Corriendo
		case ACT_RUN:
		{
			if ( !m_bCanSprint )
				return ACT_TERROR_WALK_INTENSE;

			return ACT_TERROR_RUN_INTENSE;
			break;
		}

		// Caminando
		case ACT_WALK:
		{
			return ACT_TERROR_WALK_NEUTRAL;
			break;
		}

		// Saltando
		case ACT_JUMP:
		{
			return ACT_TERROR_JUMP;
			break;
		}

		// Aterrizando
		case ACT_LAND:
		{
			if ( m_NPCState == NPC_STATE_IDLE )
				return ACT_TERROR_JUMP_LANDING_NEUTRAL;
			else
				return ACT_TERROR_JUMP_LANDING;

			break;
		}

		// Cayendo
		case ACT_GLIDE:
		{
			return ACT_TERROR_FALL;
			break;
		}

		// Desmontandose de algo donde trepar
		case ACT_CLIMB_DISMOUNT:
		{
			return ACT_TERROR_LADDER_DISMOUNT;
			break;
		}

		// Agacharse
		case ACT_CROUCH:
		{
			return ACT_TERROR_CROUCH_RUN_INTENSE;
			break;
		}

		// Hemos muerto
		case ACT_DIESIMPLE:
		{
			// TODO
			if ( m_flGroundSpeed >= 200 )
				return ACT_TERROR_DIE_WHILE_RUNNING;

			return ACT_TERROR_DIE_FROM_STAND;
			break;
		}
	}

	return pActivity;
}

//====================================================================
// Aplica las condiciones dependiendo del entorno
//====================================================================
void CInfected::GatherConditions()
{
	BaseClass::GatherConditions();

	// Estamos en llamas
	if ( IsOnFire() )
		return;

	// ¿Es hora de dormir o despertar?
	UpdateRelaxStatus();
}

//====================================================================
// Comienza la ejecución de una tarea
//====================================================================
void CInfected::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		//
		// Ataque primario
		//
		case TASK_MELEE_ATTACK1:
		{
			// No tenemos un enemigo
			if ( !GetEnemy() )
				return;

			float flDistance	= GetEnemy()->GetAbsOrigin().DistTo( GetAbsOrigin() );
			bool bLowAttack		= false;

			// 33 - Junto
			//DevMsg( "[CInfected::StartTask] %f \n", distance );

			// Para saber rapidamente si es un NPC o un Jugador
			CBaseCombatCharacter *pEnemy = GetEnemy()->MyCombatCharacterPointer();

			// Obtenemos su tamaño
			Hull_t pHull = pEnemy->GetHullType();

			// Es pequeño
			if ( pHull == HULL_TINY || pHull == HULL_WIDE_SHORT )
			{
				bLowAttack = true;
			}

			// Deben estar muy juntos para hacer esta animación
			if ( flDistance > 35 )
				bLowAttack = false;

			// Establecemos la última vez que he atacado
			SetLastAttackTime( gpGlobals->curtime );

			// Esta muy cerca
			if ( flDistance <= 40 )
			{
				GetNavigator()->StopMoving();
			}

			// Debemos realizar un ataque hacia abajo
			if ( bLowAttack )
			{
				ChainStartTask( TASK_INFECTED_LOW_ATTACK );
			}
			else
			{
				if ( flDistance <= 40 )
				{
					SetActivity( ACT_TERROR_ATTACK_CONTINUOUSLY );
				}
				else
				{
					AddGesture( ACT_TERROR_ATTACK );
				}
			}

			break;
		}

		//
		// Ataque hacia abajo
		//
		case TASK_INFECTED_LOW_ATTACK:
		{
			// NOTE: Con esto solucionamos el problema de matar a otros infectados cerca, no tiene
			// lógica pues nuestro ataque va hacia abajo
			CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );

			// Establecemos la última vez que he atacado
			SetIdealActivity( ACT_TERROR_ATTACK_LOW_CONTINUOUSLY );
			break;
		}

		//
		// Hora de sentarse un poco
		//
		case TASK_INFECTED_TO_RELAX:
		{
			SetIdealActivity( ACT_TERROR_SIT_FROM_STAND );
			break;
		}

		//
		// Estamos sentados
		//
		case TASK_INFECTED_RELAX:
		{
			SetActivity( ACT_TERROR_SIT_IDLE );
			break;
		}

		//
		// Hora de levantarse
		//
		case TASK_INFECTED_RELAX_TO_STAND:
		{
			if ( m_NPCState == NPC_STATE_IDLE )
			{
				SetIdealActivity( ACT_TERROR_SIT_TO_STAND );
			}
			else
			{
				SetIdealActivity( ACT_TERROR_SIT_TO_STAND_ALERT );
			}
			
			break;
		}

		//
		// Hora de dormir
		//
		case TASK_INFECTED_TO_SLEEP:
		{
			if ( !IsRelaxing() )
			{
				SetIdealActivity( ACT_TERROR_LIE_FROM_STAND );
			}
			else
			{
				SetIdealActivity( ACT_TERROR_SIT_TO_LIE );
			}

			break;
		}

		//
		// Estamos durmiendo
		//
		case TASK_INFECTED_SLEEP:
		{
			SetActivity( ACT_TERROR_LIE_IDLE );
			break;
		}

		//
		// Hora de despertar
		//
		case TASK_INFECTED_SLEEP_TO_STAND:
		{
			if ( m_NPCState == NPC_STATE_IDLE )
			{
				SetIdealActivity( ACT_TERROR_LIE_TO_STAND );
			}
			else
			{
				SetIdealActivity( ACT_TERROR_LIE_TO_STAND_ALERT );
			}

			break;
		}

		//
		// ¡Atacad!
		//
		case TASK_ANNOUNCE_ATTACK:
		{
			AttackSound();
			break;
		}

		//
		// Tratamos de romper nuestro bloqueo
		//
		case 152: // TASK_MELEE_ATTACK_OBJ
		{
			if ( Utils::IsDoor(GetBlockingEnt()) )
			{
				SetIdealActivity( ACT_TERROR_ATTACK_DOOR_CONTINUOUSLY );
			}
			else
			{
				SetIdealActivity( ACT_TERROR_ATTACK_CONTINUOUSLY );
			}
			
			break;
		}

		//
		// Vagar por el mundo
		//
		case TASK_WANDER:
		{
			SetIdealActivity( ACT_TERROR_WALK_NEUTRAL );
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
void CInfected::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		//
		// Ataque frontal
		//
		case TASK_MELEE_ATTACK1:
		{
			if ( !GetEnemy() )
			{
				TaskComplete(); return;
			}

			vec_t flDistance	= GetEnemy()->GetAbsOrigin().DistTo( GetAbsOrigin() );
			bool bIsGesture		= IsPlayingGesture( ACT_TERROR_ATTACK );

			// Esta muy lejos
			if ( flDistance >= 45 )
			{
				TaskComplete(); return;
			}

			if ( flDistance <= 40 && !IsActivityFinished() )
				bIsGesture = true;

			if ( !bIsGesture )
			{
				TaskComplete();
				CapabilitiesRemove( bits_CAP_FRIENDLY_DMG_IMMUNE );
			}

			break;
		}

		//
		// Ataque hacia abajo
		//
		case TASK_INFECTED_LOW_ATTACK:
		{
			if ( !GetEnemy() || GetEnemy()->GetAbsOrigin().DistTo( GetAbsOrigin() ) >= 50 )
			{
				TaskFail( "Enemy Fail" );
				return;
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
				CapabilitiesRemove( bits_CAP_FRIENDLY_DMG_IMMUNE );
			}

			break;
		}

		//
		// Hora de sentarse
		//
		case TASK_INFECTED_TO_RELAX:
		{
			AutoMovement();

			if ( IsActivityFinished() )
			{
				m_bRelaxing = true;
				m_bSleeping = false;

				SetTouch( &CInfected::OnTouch );
				SetHullType( HULL_MEDIUM );

				TaskComplete();
			}

			break;
		}
	
		//
		//
		//
		case TASK_INFECTED_RELAX:
		{
			AutoMovement();

			if ( IsActivityFinished() )
			{
				MaintainActivity();
				//TaskComplete();
			}

			break;
		}

		//
		// Hora de levantarse
		//
		case TASK_INFECTED_RELAX_TO_STAND:
		{
			AutoMovement();

			if ( IsActivityFinished() )
			{
				m_iNextRelaxing		= RELAX_TIME_INTERVAL;
				m_iNextSleep		= SLEEP_TIME_INTERVAL;
				m_iRelaxingSequence = ACTIVITY_NOT_AVAILABLE;

				m_bRelaxing		= false;
				m_bSleeping		= false;

				SetTouch( NULL );
				SetHullType( HULL_HUMAN );

				ClearCondition( COND_INFECTED_CAN_RELAX_TO_STAND ); // Evitamos repetir
				TaskComplete();
			}

			break;
		}

		//
		// Hora de dormir
		//
		case TASK_INFECTED_TO_SLEEP:
		{
			AutoMovement();

			if ( IsActivityFinished() )
			{
				m_bRelaxing		= false;
				m_bSleeping		= true;

				SetTouch( &CInfected::OnTouch );
				SetHullType( HULL_WIDE_SHORT );

				TaskComplete();
			}

			break;
		}

		//
		//
		//
		case TASK_INFECTED_SLEEP:
		{
			AutoMovement();

			if ( IsActivityFinished() )
			{
				MaintainActivity();
				//TaskComplete();
			}

			break;
		}

		//
		// Hora de despertar
		//
		case TASK_INFECTED_SLEEP_TO_STAND:
		{
			AutoMovement();

			if ( IsActivityFinished() )
			{
				m_iNextSleep		= SLEEP_TIME_INTERVAL;
				m_iNextRelaxing		= RELAX_TIME_INTERVAL;
				m_iSleepSequence	= ACTIVITY_NOT_AVAILABLE;

				m_bSleeping		= false;
				m_bRelaxing		= false;

				SetTouch( NULL );
				SetHullType( HULL_HUMAN );

				ClearCondition( COND_INFECTED_CAN_SLEEP_TO_STAND ); // Evitamos repetir
				TaskComplete();
			}

			break;
		}

		//
		// Tratamos de abrir la puerta
		//
		case 152: // TASK_MELEE_ATTACK_OBJ
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}

			break;
		}

		//
		// Vagar por el mundo
		//
		case TASK_WANDER:
		{
			AutoMovement();

			// La animación ha finalizado
			if ( IsActivityFinished() )
			{
				TaskComplete();
				return;
			}

			// Nos hemos topado con una pared
			if ( HitsWall() )
				TaskComplete();

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
int CInfected::SelectSchedule()
{
	//
	// ¡Un nuevo enemigo!
	//
	if ( HasCondition(COND_INFECTED_ACQUIRE_ANIM) )
	{
		ClearCondition( COND_INFECTED_ACQUIRE_ANIM );
		return SCHED_INFECTED_ACQUIRE_ANIM;
	}

	//
	// Hora de levantarse
	//
	if ( HasCondition(COND_INFECTED_CAN_RELAX_TO_STAND) )
	{
		ClearCondition( COND_INFECTED_CAN_RELAX_TO_STAND );
		return SCHED_INFECTED_RELAX_TO_STAND;
	}

	//
	// Hora de despertar
	//
	if ( HasCondition(COND_INFECTED_CAN_SLEEP_TO_STAND) )
	{
		ClearCondition( COND_INFECTED_CAN_SLEEP_TO_STAND );
		return SCHED_INFECTED_SLEEP_TO_STAND;
	}

	//
	// Debemos tropezar
	//
	if ( HasCondition(COND_INFECTED_CAN_STUMBLE) )
	{
		ClearCondition( COND_INFECTED_CAN_STUMBLE );
		return SCHED_INFECTED_STUMBLE;
	}

	if ( m_pPrimaryBehavior == NULL )
	{
		if ( m_NPCState == NPC_STATE_IDLE )
			return SelectIdleSchedule();

		if ( m_NPCState == NPC_STATE_COMBAT )
			return SelectCombatSchedule();
	}

	return BaseClass::SelectSchedule();
}

//====================================================================
// Selecciona una tarea
//====================================================================
int CInfected::SelectIdleSchedule()
{
	//
	// Estamos intentando descansar
	//
	if ( m_bRelaxing )
		return SCHED_INFECTED_RELAX;

	//
	// Estamos intentando dormir
	//
	if ( m_bSleeping )
		return SCHED_INFECTED_SLEEP;

	//
	// Hora de dormir
	//
	if ( HasCondition(COND_INFECTED_CAN_SLEEP) )
	{
		ClearCondition( COND_INFECTED_CAN_SLEEP );
		return SCHED_INFECTED_TO_SLEEP;
	}

	//
	// Hora de descansar
	//
	if ( HasCondition(COND_INFECTED_CAN_RELAX) )
	{
		ClearCondition( COND_INFECTED_CAN_RELAX );
		return SCHED_INFECTED_TO_RELAX;
	}

	return SCHED_INFECTED_WANDER;
}

//====================================================================
//====================================================================
int CInfected::SelectCombatSchedule()
{
	return BaseClass::SelectCombatSchedule();
}

//====================================================================
// Traduce una tarea registrada a otra
//====================================================================
int CInfected::TranslateSchedule( int scheduleType )
{
	switch ( scheduleType )
	{
		// Ataque
		case SCHED_MELEE_ATTACK1:
		{
			return SCHED_INFECTED_MELEE_ATTACK1;
			break;
		}
	
		// Fallo al perseguir al enemigo
		case SCHED_CHASE_ENEMY_FAILED:
		{
			// Si el enemigo esta muy cerca solo nos quedamos esperando ¡¿que hacemos?!
			if ( GetEnemy() )
			{
				if ( GetEnemy()->GetAbsOrigin().DistTo(GetAbsOrigin()) < 400 )
					return SCHED_STANDOFF;
			}

			// Animación de "no puedo alcanzarte, grr!"
			return SCHED_INFECTED_CHASE_ENEMY_FAILED;
			break;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//====================================================================
// Inteligencia artificial personalizada
//====================================================================
AI_BEGIN_CUSTOM_NPC( npc_infected, CInfected )

	DECLARE_CONDITION( COND_INFECTED_CAN_RELAX )
	DECLARE_CONDITION( COND_INFECTED_CAN_RELAX_TO_STAND )
	DECLARE_CONDITION( COND_INFECTED_ACQUIRE_ANIM )

	DECLARE_CONDITION( COND_INFECTED_CAN_SLEEP )
	DECLARE_CONDITION( COND_INFECTED_CAN_SLEEP_TO_STAND )
	DECLARE_CONDITION( COND_INFECTED_CAN_CLIMB )
	DECLARE_CONDITION( COND_INFECTED_CAN_STUMBLE )

	DECLARE_ANIMEVENT( AE_FOOTSTEP_RIGHT )
	DECLARE_ANIMEVENT( AE_FOOTSTEP_LEFT )
	DECLARE_ANIMEVENT( AE_ATTACK_HIT )

	DECLARE_TASK( TASK_INFECTED_LOW_ATTACK )
	DECLARE_TASK( TASK_INFECTED_TO_RELAX )
	DECLARE_TASK( TASK_INFECTED_RELAX_TO_STAND )

	DECLARE_TASK( TASK_INFECTED_TO_SLEEP )
	DECLARE_TASK( TASK_INFECTED_SLEEP_TO_STAND )

	DECLARE_TASK( TASK_INFECTED_RELAX )
	DECLARE_TASK( TASK_INFECTED_SLEEP )

	DECLARE_ACTIVITY( ACT_TERROR_LIE_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_LIE_IDLE )
	DECLARE_ACTIVITY( ACT_TERROR_LIE_TO_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_LIE_TO_STAND_ALERT )
	DECLARE_ACTIVITY( ACT_TERROR_LIE_TO_SIT )
	DECLARE_ACTIVITY( ACT_TERROR_SIT_TO_LIE )
	DECLARE_ACTIVITY( ACT_TERROR_ATTACK_DOOR_CONTINUOUSLY )
	DECLARE_ACTIVITY( ACT_TERROR_ATTACK_CONTINUOUSLY )
	DECLARE_ACTIVITY( ACT_TERROR_UNABLE_TO_REACH_TARGET )
	DECLARE_ACTIVITY( ACT_TERROR_RUN_STUMBLE )
	DECLARE_ACTIVITY( ACT_TERROR_STADING_ON_FIRE )
	DECLARE_ACTIVITY( ACT_TERROR_RUN_ON_FIRE )
	DECLARE_ACTIVITY( ACT_TERROR_ABOUT_FACE_NEUTRAL )

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_MELEE_ATTACK1,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY				0"
		"		TASK_MELEE_ATTACK1			0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_OCCLUDED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_WANDER,

		"	Tasks"
		"		TASK_STOP_MOVING				0"		// Paramos de movernos
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_TERROR_ABOUT_FACE_NEUTRAL"
		"		TASK_WANDER						0"		// Buscamos una ubicación al azar
		"		TASK_STOP_MOVING				0"		// Paramos de movernos
		"		TASK_WAIT						10"		// Esperamos 15s sin hacer nada
		"	"
		"	Interrupts"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_PLAYER"
		"		COND_INFECTED_CAN_SIT"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_ACQUIRE_ANIM,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_TERROR_IDLE_ACQUIRE"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_OCCLUDED"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_LOST_PLAYER"
	)

	//
	// DESCANSAR
	//

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_TO_RELAX,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_INFECTED_TO_RELAX			0"
		"	"
		"	Interrupts"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_PLAYER"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_RELAX,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_INFECTED_RELAX				0"
		"	"
		"	Interrupts"
		"		COND_INFECTED_CAN_RELAX_TO_STAND"
		"		COND_INFECTED_CAN_SLEEP"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_RELAX_TO_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_INFECTED_RELAX_TO_STAND	0"
		"		TASK_WAIT						1"
		"	"
		"	Interrupts"
	)

	//
	// DORMIR
	//

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_TO_SLEEP,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_INFECTED_TO_SLEEP			0"
		"	"
		"	Interrupts"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_PLAYER"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_SLEEP,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_INFECTED_SLEEP				0"
		"	"
		"	Interrupts"
		"		COND_INFECTED_CAN_SLEEP_TO_STAND"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_SLEEP_TO_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_INFECTED_SLEEP_TO_STAND	0"
		"		TASK_WAIT						1"
		"	"
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_CHASE_ENEMY_FAILED,

		"	Tasks"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_STANDOFF"
		"		 TASK_PLAY_SEQUENCE					ACTIVITY:ACT_TERROR_UNABLE_TO_REACH_TARGET"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_HEAR_DANGER"
		"		COND_BETTER_WEAPON_AVAILABLE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_STUMBLE,

		"	Tasks"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_PLAY_SEQUENCE					ACTIVITY:ACT_TERROR_RUN_STUMBLE"
		"		 TASK_STOP_MOVING					0"
		""
		"	Interrupts"
	)

AI_END_CUSTOM_NPC()