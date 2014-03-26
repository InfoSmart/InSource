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

//=========================================================
// Comandos
//=========================================================

#define IDLE_SOUND_INTERVAL gpGlobals->curtime + RandomInt(3, 10)
#define ATTACK_INTERVAL		gpGlobals->curtime + 1.8f

//=========================================================
// Información y Red
//=========================================================

LINK_ENTITY_TO_CLASS( player_infected, CAP_PlayerInfected );
PRECACHE_REGISTER( player_infected );

// DT_PlayerInfected
IMPLEMENT_SERVERCLASS_ST( CAP_PlayerInfected, DT_PlayerInfected )
END_SEND_TABLE()

BEGIN_DATADESC( CAP_PlayerInfected )
	DEFINE_THINKFUNC( RequestDirectorSpawn )
END_DATADESC()

//=========================================================
// Permite la creación del Jugador 
// desde este tipo de clase.
//=========================================================
CAP_PlayerInfected *CAP_PlayerInfected::CreateInfectedPlayer( const char *pClassName, edict_t *e )
{
	CAP_PlayerInfected::s_PlayerEdict = e;
	return ( CAP_PlayerInfected *)CreateEntityByName( pClassName );
}

//=========================================================
// Creación en el mapa
//=========================================================
void CAP_PlayerInfected::Spawn()
{
	ConVarRef infected_health("sk_infected_health");
	BaseClass::Spawn();

	// El Director será el responsable de crearnos
	// Mientras tanto seremos invisibles y no nos moveremos
	AddEffects( EF_NODRAW );
	AddFlag( FL_FREEZING );

	// Esto hará que el Director lo tome como uno de sus hijos
	SetName( AllocPooledString(DIRECTOR_CHILD_NAME) );

	// Seleccionamos un color para nuestra ropa
	SetClothColor();

	int iMaxHealth = infected_health.GetInt() + RandomInt( 1, 5 );

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

//=========================================================
// Solicita al Director un lugar para la creación
//=========================================================
void CAP_PlayerInfected::RequestDirectorSpawn()
{
	Vector vecPosition;

	// Obtenemos una ubicación recomendada según el Director
	DirectorManager->Update();
	bool bResult = DirectorManager->GetSpawnLocation( &vecPosition, DIRECTOR_SPECIAL_CHILD );

	// El Director te quiere aquí
	if ( bResult )
	{
		// Establecemos la ubicación
		SetLocalOrigin( vecPosition + Vector(0, 0, 1) );
		SetAbsVelocity( vec3_origin );
		m_Local.m_vecPunchAngle		= vec3_angle;
		m_Local.m_vecPunchAngleVel	= vec3_angle;

		// Ahora somos visibles
		RemoveEffects( EF_NODRAW );

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

	// Estamos atacando
	// TODO: ¿Esto se hace así?
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

//=========================================================
// Ataque cuerpo a cuerpo
//=========================================================
CBaseEntity *CAP_PlayerInfected::MeleeAttack()
{
	ConVarRef infected_damage( "sk_infected_damage" );
	ConVarRef infected_slash_distance( "sk_infected_slash_distance" );

	// Información
	CBaseEntity *pVictim	= NULL;
	int iDamage				= infected_damage.GetInt();
	int iDamageBits			= DMG_SLASH;

	Vector vecMins	= InRules->GetInViewVectors()->m_vHullMin;
	Vector vecMaxs	= InRules->GetInViewVectors()->m_vHullMax;
	vecMins.z		= vecMins.x;
	vecMaxs.z		= vecMaxs.x;

	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );

	// Veces que puedo golpear a un objeto y el numero de objetos que he golpeado.
	int iMax	= 2;
	int iStrike = 0;

	ConVarRef ai_show_hull_attacks("ai_show_hull_attacks");
	trace_t	tr;

	// TODO: Para hacer que esto funcione es necesario que el NPC tenga un impulso parecido a npc_boss
	// Es decir, golpear a algo muy lejos para que los demás puedan ser golpeados también.
	for ( int i = 0; i < iMax; ++i )
	{
		// Trazamos una línea enfrente de mi y verificamos si hemos dado con algo
		UTIL_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + vecForward * infected_slash_distance.GetFloat(), vecMins, vecMaxs, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

		// Handy debuging tool to visualize HullAttack trace
		if ( ai_show_hull_attacks.GetBool() )
		{
			float length	 = ( tr.endpos - tr.startpos ).Length();
			Vector direction = ( tr.endpos - tr.startpos );
			VectorNormalize( direction );

			Vector hullMaxs = vecMaxs;
			hullMaxs.x		= length + hullMaxs.x;

			NDebugOverlay::BoxDirection( tr.startpos, vecMins, hullMaxs, direction, 100,255,255,20,1.0 );
			NDebugOverlay::BoxDirection( tr.startpos, vecMins, vecMaxs, direction, 255,0,0,20,1.0 );
		}

		// No hemos dado con algo...
		if ( tr.fraction == 1.0 || !tr.m_pEnt )
			continue;
		
		// Es el mismo que he golpeado hace poco
		if ( tr.m_pEnt == pVictim )
			continue;

		CBaseCombatCharacter *pCharacter	= dynamic_cast<CBaseCombatCharacter *>( tr.m_pEnt );
		CAI_BaseNPC *pNPC					= dynamic_cast<CAI_BaseNPC *>( tr.m_pEnt );

		//
		// Es un Jugador o un NPC
		//
		if ( pCharacter )
		{
			// Le di a un amigo mio
			if ( IRelationType(pCharacter) == D_LIKE )
			{
				// No debo atacar a mi amigo
				if ( pNPC && pNPC->CapabilitiesGet() & bits_CAP_FRIENDLY_DMG_IMMUNE )
					continue;

				// Hacemos menos daño
				iDamage = RandomInt( 1, 3 );
			}

			pVictim = AttackCharacter( pCharacter );
		}

		//
		// Es un objeto
		//
		else
		{
			pVictim = AttackEntity( tr.m_pEnt );
		}

		// Hemos dado con algo/alguien
		if ( pVictim )
		{
			++iStrike;

			// Ubicación de donde proviene el impulso.
			Vector vecForce = pVictim->WorldSpaceCenter() - WorldSpaceCenter();
			VectorNormalize( vecForce );
			vecForce *= 5 * 24;

			// Realizamos el daño
			CTakeDamageInfo info( this, this, vecForce, GetAbsOrigin(), iDamage, iDamageBits );
			pVictim->TakeDamage( info );
		}
	}

	// FIXME: Por ahora solo devolvemos la última victima que hemos golpeado.
	return pVictim;
}

//=========================================================
// Realiza el ataque a una entidad
//=========================================================
CBaseEntity *CAP_PlayerInfected::AttackEntity( CBaseEntity *pEntity )
{
	IPhysicsObject *pPhysObj = pEntity->VPhysicsGetObject();

	// No es algo que pueda romper o mover
	if ( !Utils::IsBreakable(pEntity) && !pPhysObj || pEntity->IsWorld() )
		return NULL;

	Vector vecForward;

	QAngle eyeAngles = EyeAngles();
	AngleVectors( eyeAngles, &vecForward, NULL, NULL );

	// Tiene físicas, darle un empujon
	if ( pPhysObj )
	{
		float flEntityMass	= pPhysObj->GetMass();
		float flMaxMass		= 100;

		// No es pesado para mi
		if ( flEntityMass <= flMaxMass )
		{
			PhysicsImpactSound( this, pPhysObj, CHAN_BODY, pPhysObj->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 1800 );

			// Impulso hacia adelante y hacia arriba
			vecForward	= vecForward * (flMaxMass*8);
			vecForward.z	+= (flMaxMass*2);

			// Ahora aplicamos el impulso de verdad
			AngularImpulse angVelocity( RandomFloat(-80, 80), 20, RandomFloat(-160, 160) );
			pPhysObj->AddVelocity( &vecForward, &angVelocity );
		}
	}

	return pEntity;
}

//=========================================================
// Realiza el ataque a un personaje
//=========================================================
CBaseEntity *CAP_PlayerInfected::AttackCharacter( CBaseCombatCharacter *pCharacter )
{
	// Punch
	EmitSound( "Infected.Hit" );

	// Nuestra victima es un jugador, le distorcionamos la vista
	if ( pCharacter->IsPlayer() )
		pCharacter->ViewPunch( QAngle(5, 0, -5) );

	return pCharacter;
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