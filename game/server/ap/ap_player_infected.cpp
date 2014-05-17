//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ap_player_infected.h"

#include "ap_director.h"
#include "director_manager.h"
#include "directordefs.h"

#include "in_gamerules.h"
#include "in_buttons.h"
#include "in_utils.h"

extern int AE_ATTACK_HIT;

//====================================================================
// Comandos
//====================================================================

#define IDLE_SOUND_INTERVAL gpGlobals->curtime + RandomInt(10, 20)
#define ATTACK_INTERVAL		gpGlobals->curtime + 1.8f

//====================================================================
// Información y Red
//====================================================================

LINK_ENTITY_TO_CLASS( player_infected, CAP_PlayerInfected );
PRECACHE_REGISTER( player_infected );

// DT_PlayerInfected
IMPLEMENT_SERVERCLASS_ST( CAP_PlayerInfected, DT_PlayerInfected )
END_SEND_TABLE()

BEGIN_DATADESC( CAP_PlayerInfected )
	DEFINE_THINKFUNC( RequestDirectorSpawn )
END_DATADESC()

//====================================================================
// Permite la creación del Jugador como Infectado
//====================================================================
CAP_PlayerInfected *CAP_PlayerInfected::CreateInfectedPlayer( const char *pClassName, edict_t *e )
{
	CAP_PlayerInfected::s_PlayerEdict = e;
	return ( CAP_PlayerInfected *)CreateEntityByName( pClassName );
}

//====================================================================
// Creación en el mapa
//====================================================================
void CAP_PlayerInfected::Spawn()
{
	ConVarRef infected_health("sk_infected_health");
	int iMaxHealth = infected_health.GetInt() + RandomInt( 1, 5 );

	CB_MeleeCharacter::SetOuter( this );
	BaseClass::Spawn();

	// El Director será el responsable de crearnos
	// Mientras tanto seremos invisibles y no nos moveremos
	AddEffects( EF_NODRAW );
	AddFlag( FL_FREEZING );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	m_takedamage = DAMAGE_NO;

	// Esto hará que el Director lo tome como uno de sus hijos
	SetName( AllocPooledString(DIRECTOR_CHILD_NAME) );

	// Seleccionamos un color para nuestra ropa
	SetClothColor();

	// Establecemos la salud
	SetMaxHealth( iMaxHealth );
	SetHealth( iMaxHealth );

	// Más información
	m_iNextIdleSound	= IDLE_SOUND_INTERVAL;
	m_iNextAttack		= ATTACK_INTERVAL;

	// Solicitamos un lugar para el Spawn
	RegisterThinkContext( "RequestDirectorSpawn" );
	SetContextThink( &CAP_PlayerInfected::RequestDirectorSpawn, gpGlobals->curtime, "RequestDirectorSpawn" );
}

//====================================================================
// Solicita al Director un lugar para la creación
//====================================================================
void CAP_PlayerInfected::RequestDirectorSpawn()
{
	Vector vecSpawnPosition;

	// Le preguntamos al Administrador sobre un lugar para aparecer
	DirectorManager->Update();
	bool result = DirectorManager->GetSpawnLocation( &vecSpawnPosition, DIRECTOR_CHILD );

	// El Director te quiere aquí
	if ( result && DirectorManager->PostSpawn(this) )
	{
		// Establecemos la ubicación
		SetLocalOrigin( vecSpawnPosition + Vector(0, 0, 5) );
		SetAbsVelocity( vec3_origin );

		m_Local.m_vecPunchAngle		= vec3_angle;
		m_Local.m_vecPunchAngleVel	= vec3_angle;

		// Ahora somos visibles
		RemoveEffects( EF_NODRAW );
		RemoveFlag( FL_FREEZING );
		SetCollisionGroup( COLLISION_GROUP_PLAYER );
		m_takedamage = DAMAGE_YES;

		// No volvemos a pensar
		SetNextThink( TICK_NEVER_THINK, "RequestDirectorSpawn" );
		return;
	}

	// Volvemos a intentarlo en .3s
	SetNextThink( gpGlobals->curtime + .3, "RequestDirectorSpawn" );
}

//=========================================================
// Establece el color de la ropa
//=========================================================
void CAP_PlayerInfected::SetClothColor()
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
//=========================================================
void CAP_PlayerInfected::PostThink()
{
	BaseClass::PostThink();

	// Mientras estemos vivos
	if ( IsAlive() )
	{
		// Estamos atacando
		// TODO: Creo que esto no es lo mejor
		if ( IsButtonPressed(IN_ATTACK) && gpGlobals->curtime >= m_iNextAttack )
		{
			DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			m_iNextAttack = ATTACK_INTERVAL;
		}

		// ¡No sabemos nadar!
		// TODO: Animación de "ahogo"
		if ( GetWaterLevel() >= WL_Waist )
		{
			SetBloodColor( DONT_BLEED );
			TakeDamage( CTakeDamageInfo(this, this, GetMaxHealth(), DMG_GENERIC) );
		}
	}
}

//=========================================================
// Prepara la música del jugador
//=========================================================
void CAP_PlayerInfected::PrepareMusic()
{
}

//=========================================================
// Para la música del jugador
//=========================================================
void CAP_PlayerInfected::StopMusic()
{
}

//=========================================================
// Devuelve el tipo de jugador
//=========================================================
const char *CAP_PlayerInfected::GetPlayerType()
{
	return "Infected";
}

//=========================================================
// Sonido de dolor
//=========================================================
void CAP_PlayerInfected::PainSound( const CTakeDamageInfo &info )
{
	// Aún no toca
	if ( gpGlobals->curtime < m_flNextPainSound )
		return;

	// Próximo sonido de dolor
	m_flNextPainSound = gpGlobals->curtime + RandomFloat( 0.5f, 1.5f );

	EmitSound( "Infected.Pain.Male" );
}

//=========================================================
// Sonido de poca salud ¡me muero!
//=========================================================
void CAP_PlayerInfected::DyingSound()
{
	// Nada
}

//=========================================================
// Sonido al morir
//=========================================================
void CAP_PlayerInfected::DieSound()
{
	EmitSound( "Infected.Death.Male" );
}

//=========================================================
//=========================================================
void CAP_PlayerInfected::UpdateSounds()
{
	if ( gpGlobals->curtime >= m_iNextIdleSound )
	{
		EmitSound( "Infected.Idle" );
		m_iNextIdleSound = IDLE_SOUND_INTERVAL;
	}
}

//=========================================================
//=========================================================
void CAP_PlayerInfected::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->Event() == AE_ATTACK_HIT )
	{
		EmitSound( "Infected.Attack.Male" );
		MeleeAttack();

		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//=========================================================
// [Evento] El jugador ha recibido daño pero sigue vivo
//=========================================================
int CAP_PlayerInfected::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Efectos ¡Ouch!
	color32 black = {0, 0, 0, 130}; // Rojo
	UTIL_ScreenFade( this, black, 0.4, 0, FFADE_IN );

	// Ouch!
	PainSound( info );

	return BaseClass::OnTakeDamage_Alive( info );
}

//=========================================================
// Pensamiento cuando el Jugador ha muerto y listo para
// respawnear
//=========================================================
void CAP_PlayerInfected::PlayerDeathPostThink()
{
	// Este Jugador no puede reaparecer
	if ( !InRules->FPlayerCanRespawn(this) )
		return;

	// ¡Podemos reaparecer!
	m_lifeState = LIFE_RESPAWNABLE;

	// Esperamos 3s para que pueda ver su cadaver
	// TODO: ¿Necesario?
	if ( gpGlobals->curtime < GetDeathTime() + 3.0f )
		return;

	// Reaparecemos al instante
	SetNextThink( TICK_NEVER_THINK );
	Spawn();

	m_nButtons			= 0;
	m_iRespawnFrames	= 0;
}

//====================================================================
// Devuelve la cantidad de daño por ataque
//====================================================================
int CAP_PlayerInfected::GetMeleeDamage()
{
	ConVarRef infected_damage( "sk_infected_damage" );
	return infected_damage.GetInt();
}

//====================================================================
// Devuelve la distancia de ataque
//====================================================================
int CAP_PlayerInfected::GetMeleeDistance()
{
	ConVarRef infected_slash_distance( "sk_infected_slash_distance" );
	return infected_slash_distance.GetInt();
}

//=========================================================
// Actualiza la velocidad del jugador
//=========================================================
void CAP_PlayerInfected::UpdateSpeed()
{	
	// Nos ha hecho daño algo que nos hace lentos
	if ( m_nSlowTime.HasStarted() && !m_nSlowTime.IsElapsed() )
	{
		SetMaxSpeed( 100 );
		return;
	}

	// Establecemos la velocidad máxima
	SetMaxSpeed( PLAYER_RUN_SPEED );
}