//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//

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

//=========================================================
// Comandos
//=========================================================

ConVar infected_health( "sk_infected_health", "10", FCVAR_SERVER, "Salud de los infectados" );
ConVar infected_damage( "sk_infected_damage", "4", FCVAR_SERVER, "Daño causado por el infectado" );
ConVar infected_allow_jump( "sk_infected_allow_jump", "1", FCVAR_SERVER | FCVAR_CHEAT, "Indica si el infectado puede saltar" );
ConVar infected_slash_distance( "sk_infected_slash_distance", "50", FCVAR_SERVER, "" );

//=========================================================
// Configuración
//=========================================================

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

#define INFECTED_SKIN_COUNT				31

#define INFECTED_SKIN_HEAD_COUNT		2
#define INFECTED_SKIN_UPPERBODY_COUNT	3
#define INFECTED_SKIN_LOWERBODY_COUNT	1

#define INFECTED_SKIN_TORSO_COUNT	11
#define INFECTED_SKIN_LEGS_COUNT	7
#define INFECTED_SKIN_HANDS_COUNT	2
#define INFECTED_SKIN_BEANIES_COUNT	2

#define	INFECTED_GIB_MODEL "models/infected/gibs/gibs.mdl"
#define RELAX_TIME_RAND		RandomInt( 10, 130 )

//=========================================================
// Modelos posibles
//=========================================================

/*const char *m_nInfectedModels[] =
{
	"models/bloocobalt/citizens/male_01.mdl",
	"models/bloocobalt/citizens/male_02.mdl",
	"models/bloocobalt/citizens/male_03.mdl",
	"models/bloocobalt/citizens/male_04.mdl",
	"models/bloocobalt/citizens/male_05.mdl",
	"models/bloocobalt/citizens/male_06.mdl",
	"models/bloocobalt/citizens/male_07.mdl",
	"models/bloocobalt/citizens/male_08.mdl",
	"models/bloocobalt/citizens/male_09.mdl",

	"models/bloocobalt/citizens/female_01.mdl",
	"models/bloocobalt/citizens/female_02.mdl",
	"models/bloocobalt/citizens/female_03.mdl",
	"models/bloocobalt/citizens/female_04.mdl",
	"models/bloocobalt/citizens/female_06.mdl",
	"models/bloocobalt/citizens/female_07.mdl",
};*/

const char *m_nInfectedModels[] =
{
	"models/infected/common_male01.mdl",
	"models/infected/common_female01.mdl",
};

//=========================================================
// Eventos de animación
//=========================================================

extern int AE_FOOTSTEP_RIGHT;
extern int AE_FOOTSTEP_LEFT;
extern int AE_ATTACK_HIT;

//=========================================================
// Tareas
//=========================================================
enum
{
	TASK_INFECTED_LOW_ATTACK = LAST_SHARED_TASK,
	TASK_INFECTED_SIT,
	TASK_INFECTED_GO_UP,

	LAST_INFECTED_TASK
};

//=========================================================
// Tareas programadas
//=========================================================
enum
{
	SCHED_INFECTED_MELEE_ATTACK1 = LAST_SHARED_SCHEDULE + 10,
	SCHED_INFECTED_LOW_ATTACK,
	SCHED_INFECTED_WANDER,
	SCHED_INFECTED_ACQUIRE_ANIM,

	SCHED_INFECTED_GO_SIT,
	SCHED_INFECTED_RELAX,
	SCHED_INFECTED_GO_UP,

	LAST_INFECTED_SCHEDULE
};

//=========================================================
// Condiciones
//=========================================================
enum
{
	COND_INFECTED_CAN_LOW_ATTACK = LAST_SHARED_CONDITION + 10,
	COND_INFECTED_CAN_SIT,
	COND_INFECTED_CAN_GO_UP,
	COND_INFECTED_ACQUIRE_ANIM,

	LAST_INFECTED_CONDITION
};

//=========================================================
// Animaciones
//=========================================================

//=========================================================
// Información
//=========================================================

LINK_ENTITY_TO_CLASS( npc_infected, CInfected );

BEGIN_DATADESC( CInfected )
	DEFINE_FIELD( m_flNextPainSound, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextAlertSound, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAttackIn, FIELD_FLOAT ),

	DEFINE_FIELD( m_iFaceGesture, FIELD_INTEGER ),
	DEFINE_FIELD( m_iAttackLayer, FIELD_INTEGER ),
END_DATADESC()

//=========================================================
// Nacimiento
//=========================================================
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
	m_iHealth		= infected_health.GetInt();
	m_flFieldOfView = INFECTED_FOV;
	m_iFaceGesture	= -1;
	m_iAttackLayer	= -1;

	m_bRelaxing = false;

	// ¿Podremos relajarnos?
	if ( RandomInt(0, 2) == 1 )
		m_iNextRelaxing = gpGlobals->curtime + RELAX_TIME_RAND;
	else
		m_iNextRelaxing = -1;

	NPCInit();

	// Distancia de visión
	m_flDistTooFar = 312.0;
	SetDistLook( 824.0 );
}

//=========================================================
// Guardado de objetos necesarios en caché
//=========================================================
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

//=========================================================
// Bucle de ejecución de tareas
//=========================================================
void CInfected::Think()
{
	// Think
	BaseClass::Think();

	// Gestos
	HandleGesture();

	// Al haber varios de nosotros trepando nos podemos atorar entre nosotros mismos
	if ( GetNavType() == NAV_CLIMB )
	{
		SetCollisionGroup( COLLISION_GROUP_NOT_BETWEEN_THEM );
	}

	// ¿Es hora de dormir o despertar?
	UpdateRelaxStatus();

	// ¡No sabemos nadar!
	// TODO: Animación de "ahogo"
	if ( GetWaterLevel() >= WL_Waist )
	{
		SetBloodColor( DONT_BLEED );
		TakeDamage( CTakeDamageInfo(this, this, GetMaxHealth(), DMG_GENERIC) );
	}
}

//=========================================================
// Devuelve la clase de NPC
//=========================================================
Class_T CInfected::Classify()
{
	return CLASS_INFECTED;
}

//=========================================================
// Establece el modelo del infectado
//=========================================================
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

//=========================================================
// Establece el color de la ropa
//=========================================================
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

//=========================================================
// Actualiza los gestos
//=========================================================
void CInfected::HandleGesture()
{
	CAnimationLayer *pFace = GetAnimOverlay( m_iFaceGesture );

	if ( pFace && !pFace->m_bSequenceFinished )
		return;

	if ( !GetEnemy() )
		m_iFaceGesture = AddGesture( ACT_EXP_IDLE );
	else
		m_iFaceGesture = AddGesture( ACT_EXP_ANGRY );
}

//=========================================================
//=========================================================
void CInfected::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->Event() == AE_ATTACK_HIT )
	{
		AttackSound();
		MeleeAttack();

		return;
	}

	if ( pEvent->Event() == AE_FOOTSTEP_LEFT )
		return;

	if ( pEvent->Event() == AE_FOOTSTEP_RIGHT )
		return;

	BaseClass::HandleAnimEvent( pEvent );
}

//=========================================================
//=========================================================
void CInfected::CreateGoreGibs( int iBodyPart, const Vector &vecOrigin, const Vector &vecDir )
{
	CGib *pGib;

	switch ( iBodyPart )
	{
		case HITGROUP_HEAD:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 3 );

			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir, 1 );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 4 );

			break;
		}

		case HITGROUP_LEFTLEG:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 2 );

			break;
		}

		case HITGROUP_RIGHTLEG:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 2 );

			break;
		}
	}

	int iRand = RandomInt(4, 8);

	for ( int i = 2; i < iRand; ++i )
	{
		pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir, i );
		pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), RandomInt(5, 9) );
	}
}

//=========================================================
// Verifica si el Infectado debe sentarse o levantarse
//=========================================================
void CInfected::UpdateRelaxStatus()
{
	// No somos de los que se duermen
	if ( m_iNextRelaxing <= -1 )
		return;

	if ( !m_bRelaxing )
	{
		// Hora de sentarnos y descansr
		if ( gpGlobals->curtime >= m_iNextRelaxing && m_NPCState == NPC_STATE_IDLE )
		{
			GoToSit();
		}	
	}
	else
	{
		// Hora de levantarnos
		if ( gpGlobals->curtime >= (m_iNextRelaxing+15) )
			GoUp();

		// ¡Tenemos un enemigo!
		if ( GetEnemy() )
		{
			SetState( NPC_STATE_ALERT );
			GoUp();
		}
	}
}

//=========================================================
// Hace que el Infectado se vaya a sentar
//=========================================================
void CInfected::GoToSit()
{
	// Estamos en proceso
	if ( HasCondition(COND_INFECTED_CAN_SIT) )
		return;

	// Ya estamos descansando
	if ( m_bRelaxing )
		return;

	SetCondition( COND_INFECTED_CAN_SIT );
}

//=========================================================
// Hace que el infectado se levante
//=========================================================
void CInfected::GoUp()
{
	// Estamos en proceso
	if ( HasCondition(COND_INFECTED_CAN_GO_UP) )
		return;

	// No estamos descansando
	if ( !m_bRelaxing )
		return;

	SetCondition( COND_INFECTED_CAN_GO_UP );
}

//=========================================================
// ¿Podemos reproducir un sonido de descanso?
// Los infectados reproducen este sonido constantemente.
//=========================================================
bool CInfected::ShouldPlayIdleSound()
{
	if ( (m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT) && RandomInt(0,50) == 0 && !HasSpawnFlags(SF_NPC_GAG) )
		return true;

	return false;
}

//=========================================================
// Reproduce un sonido de "descanso"
//=========================================================
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

//=========================================================
// Reproduce un sonido de dolor
//=========================================================
void CInfected::PainSound( const CTakeDamageInfo &info )
{
	if ( gpGlobals->curtime >= m_flNextPainSound )
	{
		EmitSound( UTIL_VarArgs("Infected.Pain.%s", GetGender()) );

		// Próxima vez...
		m_flNextPainSound = gpGlobals->curtime + RandomFloat(0.1, 0.5);
	}
}

//=========================================================
// Reproduce un sonido de alerta
//=========================================================
void CInfected::AlertSound()
{
	if ( gpGlobals->curtime >= m_flNextAlertSound )
	{
		EmitSound( UTIL_VarArgs("Infected.Alert.%s", GetGender()) );

		// Próxima vez...
		m_flNextAlertSound = gpGlobals->curtime + RandomFloat(.5, 1.0);
	}
}

//=========================================================
// Reproducir sonido de muerte.
//=========================================================
void CInfected::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( UTIL_VarArgs("Infected.Death.%s", GetGender()) );
}

//=========================================================
// Reproducir sonido al azar de ataque alto/fuerte.
//=========================================================
void CInfected::AttackSound()
{
	EmitSound( UTIL_VarArgs("Infected.Attack.%s", GetGender()) );
}

//=========================================================
// Devuelve si el NPC puede atacar
//=========================================================
bool CInfected::CanAttack()
{
	// Estamos sentados, no podemos atacar
	if ( m_bRelaxing )
		return false;

	CAnimationLayer *pLayer = GetAnimOverlay( m_iAttackLayer );

	// No hay ningun gesto activo
	if ( !pLayer )
		return true;

	// El gesto de ataque ha terminado
	if ( pLayer->m_bSequenceFinished )
		return true;

	// Por si acaso...
	if ( gpGlobals->curtime >= GetLastAttackTime()+1.2f )
		return true;

	return false;
}

//=========================================================
// [Evento] Se ha adquirido un nuevo enemigo
//=========================================================
void CInfected::OnEnemyChanged( CBaseEntity *pOldEnemy, CBaseEntity *pNewEnemy )
{
	if ( !pNewEnemy )
		return;

	if ( !Director->Is(CLIMAX) && !Director->Is(PANIC) )
	{
		if ( pNewEnemy->IsPlayer() && pNewEnemy->GetAbsOrigin().DistTo(GetAbsOrigin()) >= 350 && !HasCondition(COND_INFECTED_ACQUIRE_ANIM) )
		{
			SetCondition( COND_INFECTED_ACQUIRE_ANIM );
		}
	}
}

//=========================================================
// [Evento] Se ha adquirido un nuevo enemigo
//=========================================================
int CInfected::OnTakeDamage( const CTakeDamageInfo &info )
{
	int iResult = BaseClass::OnTakeDamage( info );

	// ¡Arg! Dejenme descansar
	if ( m_bRelaxing )
	{
		GoUp();
	}

	return iResult;
}

//=========================================================
// [Evento] He muerto
//=========================================================
void CInfected::Event_Killed( const CTakeDamageInfo &info )
{
	Dismembering( info );

	BaseClass::Event_Killed( info );
}

//=========================================================
// Verifica el último daño causado y desmiembra al infectado
//=========================================================
void CInfected::Dismembering( const CTakeDamageInfo &info )
{
	Vector vecDir = info.GetDamageForce();
	Vector vecOrigin;
	QAngle angDir;

	int iBloodAttach	= -1;
	int iHitBox			= LastHitGroup();

	switch ( iHitBox )
	{
			// 3 - derecha
			// 4 - izquierda
			// 

			// 1 - derecha
			// 2 - izquierda

		// Adios cabeza
		case HITGROUP_HEAD:
			SetBodygroup( FindBodygroupByName("Head"), 4 );
			iBloodAttach = GetAttachment( "Head", vecOrigin, angDir );
		break;

		// Adios brazo izquierdo
		case HITGROUP_LEFTLEG:
			SetBodygroup( FindBodygroupByName("LowerBody"), 2 );
			iBloodAttach = GetAttachment( "severed_LLeg", vecOrigin, angDir );
		break;

		// Adios brazo derecho
		case HITGROUP_RIGHTLEG:
			SetBodygroup( FindBodygroupByName("LowerBody"), 1 );
			iBloodAttach = GetAttachment( "severed_RLeg", vecOrigin, angDir );
		break;
	}

	// Puff, seguimos con nuestro cuerpo
	if ( iBloodAttach <= -1 )
		return;
	
	// ¡GORE!
	//AngleVectors( angDir, &vecDir, NULL, NULL );

	UTIL_BloodSpray( vecOrigin, vecDir, BloodColor(), 18, FX_BLOODSPRAY_GORE );
	UTIL_BloodDrips( vecOrigin, vecDir, BloodColor(), 18 );

	EmitSound("Infected.Hit");

	// Pintamos sangre en las paredes
	for ( int i = 0 ; i < 30; i++ )
	{
		Vector vecTraceDir = vecDir;
		vecTraceDir.x += random->RandomFloat( -0.5f, 0.5f );
		vecTraceDir.y += random->RandomFloat( -0.5f, 0.5f );
		vecTraceDir.z += random->RandomFloat( -0.5f, 0.5f );
 
		trace_t tr;
		AI_TraceLine( vecOrigin, vecOrigin + ( vecTraceDir * 230.0f ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
			UTIL_BloodDecalTrace( &tr, BloodColor() );
	}

	CreateGoreGibs( iHitBox, vecOrigin, vecDir );
}

//=========================================================
// Devuelve el daño de un ataque
//=========================================================
int CInfected::GetMeleeDamage()
{
	return infected_damage.GetInt();
}

//=========================================================
// Devuelve la distancia de un ataque
//=========================================================
int CInfected::GetMeleeDistance()
{
	return infected_slash_distance.GetInt();
}

//=========================================================
//=========================================================
bool CInfected::CanPlayDeathAnim( const CTakeDamageInfo &info )
{
	// Estabamos relajandonos
	if ( m_bRelaxing )
		return false;

	return BaseClass::CanPlayDeathAnim( info );
}

//=========================================================
// Calcula si el salto que realizará es válido.
//=========================================================
bool CInfected::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	// No podemos saltar
	if ( !infected_allow_jump.GetBool() )
		return false;

	const float MAX_JUMP_UP			= 60.0f;
	const float MAX_JUMP_DOWN		= 11084.0f;
	const float MAX_JUMP_DISTANCE	= 312.0f;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_UP, MAX_JUMP_DOWN, MAX_JUMP_DISTANCE );
}

//=========================================================
// Traduce una animación a otra
//=========================================================
Activity CInfected::NPC_TranslateActivity( Activity pActivity )
{
	switch ( pActivity )
	{
		// Sin hacer nada
		case ACT_IDLE:
		{
			// Estamos en alerta o combate
			if ( m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT )
				return ACT_TERROR_IDLE_ALERT;

			return ACT_TERROR_IDLE_NEUTRAL;
			break;
		}
		
		// Corriendo
		case ACT_RUN:
		{
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
			return ACT_TERROR_CRAWL_RUN;
			break;
		}

		// Hemos muerto
		case ACT_DIESIMPLE:
		{
			// TODO
			//if ( m_flGroundSpeed >= 200 )
				//return ACT_TERROR_DIE_WHILE_RUNNING;

			return ACT_TERROR_DIE_FROM_STAND;
			break;
		}
	}

	return pActivity;
}

//=========================================================
// Comienza la ejecución de una tarea
//=========================================================
void CInfected::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		//
		// Ataque primario
		//
		case TASK_MELEE_ATTACK1:
		{
		
			if ( !GetEnemy() )
				return;

			CAI_BaseNPC *pEnemy			= GetEnemy()->MyNPCPointer();
			CIN_Player *pEnemyPlayer	= ToInPlayer( GetEnemy() );

			bool bLowAttack = false;

			// Es un Jugador y esta incapacitado
			if ( pEnemyPlayer && pEnemyPlayer->IsDejected() )
			{
				bLowAttack = true;
			}

			// Es un NPC
			else if ( pEnemy )
			{
				// Obtenemos su tamaño
				Hull_t pHull = pEnemy->GetHullType();

				// Es pequeño
				if ( pHull == HULL_TINY || pHull == HULL_WIDE_SHORT )
				{
					bLowAttack = true;
				}
			}

			// Deben estar muy juntos para hacer esta animación
			if ( m_nPotentialEnemy != GetEnemy() )
				bLowAttack = false;

			// Debemos realizar un ataque hacia abajo
			if ( bLowAttack )
			{
				SetCondition( COND_INFECTED_CAN_LOW_ATTACK );
				return;
			}
			else
			{
				m_iAttackLayer = AddGesture( ACT_TERROR_ATTACK );
			}

			// Establecemos la última vez que hemos atacado
			SetLastAttackTime( gpGlobals->curtime );

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

			// 
			ResetIdealActivity( ACT_TERROR_ATTACK_LOW_CONTINUOUSLY );
			SetLastAttackTime( gpGlobals->curtime );

			break;
		}

		//
		// Ir a descansar un poco
		//
		case TASK_INFECTED_SIT:
		{

			m_bRelaxing = true;
			ResetIdealActivity( ACT_TERROR_SIT_FROM_STAND );

			break;
		}

		//
		// Hora de levantarse
		//
		case TASK_INFECTED_GO_UP:
		{

			if ( m_NPCState == NPC_STATE_IDLE )
			{
				ResetIdealActivity( ACT_TERROR_SIT_TO_STAND );
			}
			else
			{
				ResetIdealActivity( ACT_TERROR_SIT_TO_STAND_ALERT );
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

		default:
			BaseClass::StartTask( pTask );
		break;
	}
}

//=========================================================
// Ejecuta una tarea y espera a que termine
//=========================================================
void CInfected::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		//
		// Ataque primario
		//
		case TASK_MELEE_ATTACK1:
		{

			TaskComplete();
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
		// Ir a descansar un poco
		//
		case TASK_INFECTED_SIT:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}

			break;
		}

		//
		// Hora de levantarse
		//
		case TASK_INFECTED_GO_UP:
		{

			if ( IsActivityFinished() )
			{
				m_iNextRelaxing = gpGlobals->curtime + RELAX_TIME_RAND;
				m_bRelaxing		= false;

				ClearCondition( COND_INFECTED_CAN_GO_UP ); // Evitamos repetir
				TaskComplete();
			}

			break;
		}

		default:
			BaseClass::RunTask( pTask );
		break;
	}
}

//=========================================================
// Selecciona una tarea
//=========================================================
int CInfected::SelectSchedule()
{
	//
	// Hora de levantarse
	//
	if ( HasCondition(COND_INFECTED_CAN_GO_UP) )
	{
		ClearCondition( COND_INFECTED_CAN_GO_UP );
		return SCHED_INFECTED_GO_UP;
	}

	//
	// Estamos intentando descansar
	//
	if ( m_bRelaxing )
		return SCHED_INFECTED_RELAX;

	//
	// Hora de descansar
	//
	if ( HasCondition(COND_INFECTED_CAN_SIT) )
	{
		ClearCondition( COND_INFECTED_CAN_SIT );
		return SCHED_INFECTED_GO_SIT;
	}

	//
	// ¡Un nuevo enemigo!
	//
	if ( HasCondition(COND_INFECTED_ACQUIRE_ANIM) )
	{
		ClearCondition( COND_INFECTED_ACQUIRE_ANIM );
		return SCHED_INFECTED_ACQUIRE_ANIM;
	}

	if ( m_NPCState == NPC_STATE_IDLE )
	{
		if ( !m_bRelaxing )
			return SCHED_INFECTED_WANDER;
	}

	//
	// Ataque hacia abajo
	//
	if ( HasCondition(COND_INFECTED_CAN_LOW_ATTACK) )
	{
		return SCHED_INFECTED_LOW_ATTACK;
	}

	return BaseClass::SelectSchedule();
}

//=========================================================
// Traduce una tarea registrada a otra
//=========================================================
int CInfected::TranslateSchedule( int scheduleType )
{
	switch ( scheduleType )
	{
		// Ataque
		case SCHED_MELEE_ATTACK1:
			return SCHED_INFECTED_MELEE_ATTACK1;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
// Inteligencia artificial personalizada
//=========================================================
AI_BEGIN_CUSTOM_NPC( npc_infected, CInfected )

	DECLARE_CONDITION( COND_INFECTED_CAN_LOW_ATTACK )
	DECLARE_CONDITION( COND_INFECTED_CAN_SIT )
	DECLARE_CONDITION( COND_INFECTED_CAN_GO_UP )
	DECLARE_CONDITION( COND_INFECTED_ACQUIRE_ANIM )

	DECLARE_ANIMEVENT( AE_FOOTSTEP_RIGHT )
	DECLARE_ANIMEVENT( AE_FOOTSTEP_LEFT )
	DECLARE_ANIMEVENT( AE_ATTACK_HIT )

	DECLARE_TASK( TASK_INFECTED_LOW_ATTACK )
	DECLARE_TASK( TASK_INFECTED_SIT )
	DECLARE_TASK( TASK_INFECTED_GO_UP )

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
		SCHED_INFECTED_LOW_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_INFECTED_LOW_ATTACK	0"
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
		"		TASK_STOP_MOVING							0"			// Paramos de movernos
		"		TASK_WANDER									100"		// Buscamos una ubicación al azar
		"		TASK_WALK_PATH								0"			// Caminamos hacia esa ubicación
		"		TASK_WAIT_FOR_MOVEMENT						0"			// Esperamos a que llegue a la ubicación.
		"		TASK_STOP_MOVING							0"			// Paramos de movernos
		"		TASK_WAIT_PVS								0"			//
		"		TASK_WAIT									15"			// Esperamos 15s sin hacer nada
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
		SCHED_INFECTED_GO_SIT,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_INFECTED_SIT				0"
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
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_TERROR_SIT_IDLE"
		"	"
		"	Interrupts"
		"		COND_INFECTED_CAN_GO_UP"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INFECTED_GO_UP,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_INFECTED_GO_UP			0"
		"		TASK_WAIT					1"
		"	"
		"	Interrupts"
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

AI_END_CUSTOM_NPC()