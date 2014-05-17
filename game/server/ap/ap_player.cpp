//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ap_player.h"

#include "ap_playeranimstate.h"
#include "ap_gamerules.h"
#include "ap_player_infected.h"

#include "in_buttons.h"

//====================================================================
// Comandos
//====================================================================

ConVar player_blood_level( "in_player_blood_level", "5000", FCVAR_SERVER, "" );

#define BLOOD_LEVEL			player_blood_level.GetFloat()
#define NEXT_LIFE_UPDATE	gpGlobals->curtime + RandomInt(20, 120)

//====================================================================
// Información y Red
//====================================================================

LINK_ENTITY_TO_CLASS( player, CAP_Player );
PRECACHE_REGISTER( player );

IMPLEMENT_SERVERCLASS_ST( CAP_Player, DT_ApPlayer )
END_SEND_TABLE()

BEGIN_DATADESC( CAP_Player )
	DEFINE_FIELD( m_nIncapMusic, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nDeathMusic, FIELD_EHANDLE ),

	DEFINE_FIELD( m_nTired, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nFall, FIELD_EHANDLE ),
END_DATADESC()

//===============================================================================
// Permite la creación del Jugador como Infectado
//===============================================================================
CAP_Player *CAP_Player::CreateSurvivorPlayer( const char *pClassName, edict_t *pEdict, const char *pPlayerName )
{
	CBasePlayer::s_PlayerEdict = pEdict;

	CAP_Player *pPlayer = ( CAP_Player * )CreateEntityByName( pClassName );
	pPlayer->SetPlayerName( pPlayerName );

	return pPlayer;
}

//===============================================================================
// Crea el procesador de animaciones
//===============================================================================
void CAP_Player::CreateAnimationState()
{
	m_nAnimState = CreateApPlayerAnimationState( this );
}

//====================================================================
// Creación en el mapa
//====================================================================
void CAP_Player::Spawn()
{
	BaseClass::Spawn();

	SendLesson( "instructor_start_gameplay_normal" );

	// Niveles de recursos
	m_flBloodLevel		= BLOOD_LEVEL;
	m_iHungryLevel		= RandomInt(70, 90);
	m_iThirstLevel		= RandomInt(70, 90);
	m_bBleed			= false;

	m_iNextLifeUpdate	= NEXT_LIFE_UPDATE;
	m_iNextBleedDamage	= gpGlobals->curtime;
	m_iNextBleedEffect	= gpGlobals->curtime;
}

//====================================================================
// Guarda objetos necesarios en caché.
//====================================================================
void CAP_Player::Precache()
{
	BaseClass::Precache();

	// Música
	for ( int i = 0; i < ARRAYSIZE(m_nPlayerMusic); ++i )
	{
		PrecacheScriptSound( UTIL_VarArgs("Player.Music.%s", m_nPlayerMusic[i]) );
	}

	// Sonidos de los jugadores
	for ( int i = 0; i < ARRAYSIZE(m_nPlayersTypes); ++i )
	{
		for ( int e = 0; e < ARRAYSIZE(m_nPlayersSounds); ++e )
		{
			const char *pSoundName = UTIL_VarArgs("%s.%s", m_nPlayersTypes[i], m_nPlayersSounds[e]);
			PrecacheScriptSound( pSoundName );
		}
	}
}

//====================================================================
//====================================================================
void CAP_Player::PrecacheMultiplayer()
{
	// Modelos para los humanos
	for ( int i = 0; i < ARRAYSIZE(m_nHumansPlayerModels); ++i )
		PrecacheModel( m_nHumansPlayerModels[i] );

	// Modelos para los soldados
	for ( int i = 0; i < ARRAYSIZE(m_nSoldiersPlayerModels); ++i )
		PrecacheModel( m_nSoldiersPlayerModels[i] );

	// Modelos para los infectados
	for ( int i = 0; i < ARRAYSIZE(m_nInfectedPlayerModels); ++i )
		PrecacheModel( m_nInfectedPlayerModels[i] );
}

//====================================================================
// Pensamiento
//====================================================================
void CAP_Player::PostThink()
{
	BaseClass::PostThink();

	if ( IsAlive() )
	{
		UpdateLifeLevels();
	}
}

//====================================================================
// Valida y establece el modelo del jugador
//====================================================================
const char* CAP_Player::GetPlayerModelValidated()
{
	const char *pPlayerModel = GetConVar( "cl_playermodel" );
	int iRand = 0;

	// El modelo que desea el Jugador no es válido.
	if ( !IsValidModel(pPlayerModel) )
	{
		switch ( GetTeamNumber() )
		{
			// Humanos
			case TEAM_HUMANS:
			default:
				iRand			= RandomInt( 0, ARRAYSIZE(m_nHumansPlayerModels) - 1 );
				pPlayerModel	= m_nHumansPlayerModels[iRand];
			break;

			// Soldados
			case TEAM_SOLDIERS:
				iRand			= RandomInt( 0, ARRAYSIZE(m_nSoldiersPlayerModels) - 1 );
				pPlayerModel	= m_nSoldiersPlayerModels[iRand];
			break;

			case TEAM_INFECTED:
				iRand			= RandomInt( 0, ARRAYSIZE(m_nInfectedPlayerModels) - 1 );
				pPlayerModel	= m_nInfectedPlayerModels[iRand];
			break;
		}
	}

	//ExecCommand( UTIL_VarArgs("cl_playermodel %s", pPlayerModel) );

	// Devolvemos el Modelo
	return pPlayerModel;
}

//====================================================================
// Devuelve si el modelo es válido
//====================================================================
bool CAP_Player::IsValidModel( const char *pModel )
{
	// Usemos el modelo que ha seleccionado el jugador
	if ( pModel == NULL )
		pModel = GetConVar( "cl_playermodel" );

	switch ( GetTeamNumber() )
	{
		// Humanos
		case TEAM_HUMANS:
			for ( int i = 0; i < ARRAYSIZE(m_nHumansPlayerModels); ++i )
			{
				if ( FStrEq(m_nHumansPlayerModels[i], pModel) )
					return true;
			}
		break;

		// Soldados
		case TEAM_SOLDIERS:
			for ( int i = 0; i < ARRAYSIZE(m_nSoldiersPlayerModels); ++i )
			{
				if ( FStrEq(m_nSoldiersPlayerModels[i], pModel) )
					return true;
			}
		break;

		// Infectados
		case TEAM_INFECTED:
			for ( int i = 0; i < ARRAYSIZE(m_nInfectedPlayerModels); ++i )
			{
				if ( FStrEq(m_nInfectedPlayerModels[i], pModel) )
					return true;
			}
		break;
	}

	// No es válido
	return false;
}

//====================================================================
// Prepara la música del jugador
//====================================================================
void CAP_Player::PrepareMusic()
{
	m_nDeathMusic = CreateLayerSound( "Player.Music.Death", LAYER_VERYHIGH );
	m_nDeathMusic->SetOnlyTeam( GetTeamNumber() );
	m_nDeathMusic->SetListener( this );
	m_nDeathMusic->SetTagSound( "Player.Music.Death.Tag" );

	m_nPreDeathMusic = CreateLayerSound( "Player.Music.PreDeath", LAYER_VERYHIGH );
	m_nPreDeathMusic->SetOnlyTeam( GetTeamNumber() );
	m_nPreDeathMusic->SetListener( this );
	m_nPreDeathMusic->SetTagSound( "Player.Music.PreDeath.Tag" );

	m_nIncapMusic = CreateLayerSound( "Player.Music.Incap", LAYER_NO, true );
	m_nIncapMusic->SetListener( this );

	m_nClimbingIncapMusic = CreateLayerSound( "Player.Music.ClimbIncap.Firm", LAYER_VERYHIGH, true );
	m_nClimbingIncapMusic->SetListener( this );

	// FacePoser!
	m_nTired = CreateLayerSound( "Player.Breath.Tired", LAYER_NO );
	m_nTired->SetListener( this );

	m_nFall = CreateLayerSound( "Abigail.Fall", LAYER_NO );
	m_nFall->SetListener( this );
}

//====================================================================
// Para la música del jugador
//====================================================================
void CAP_Player::StopMusic()
{
	m_nDeathMusic->Stop();
	m_nPreDeathMusic->Stop();
	m_nIncapMusic->Stop();
}

//====================================================================
// Sonido de dolor
//====================================================================
void CAP_Player::PainSound( const CTakeDamageInfo &info )
{
	// Aún no toca
	if ( gpGlobals->curtime < m_flNextPainSound )
		return;

	// Próximo sonido de dolor
	m_flNextPainSound = gpGlobals->curtime + RandomFloat( 0.5f, 1.5f );

	// Estoy abatido
	if ( IsIncap() )
	{
		EmitPlayerSound("Hurt.Dejected");
		return;
	}

	CBaseEntity *pAttacker = info.GetAttacker();

	// Mi atacante es un jugador
	if ( pAttacker && pAttacker->IsPlayer() )
	{
		// ¡Es del mismo equipo, Fuego amigo!
		if ( pAttacker != this && InRules->PlayerRelationship(this, pAttacker) == GR_TEAMMATE )
		{
			EmitPlayerSound("FriendlyFire");
			return;
		}
	}

	int iHealth = GetHealth();

	if ( iHealth < 20 )
		EmitPlayerSound("Hurt.Critical");
	else if ( iHealth < 50 )
		EmitPlayerSound("Hurt.Major");
	else
		EmitPlayerSound("Hurt.Minor");

	// Retrasamos un poco el sonido de sufrimiento
	m_flNextDyingSound = gpGlobals->curtime + RandomFloat( 5.0f, 10.0f );
}

//====================================================================
// Sonido de poca salud ¡me muero!
//====================================================================
void CAP_Player::DyingSound()
{
	// Aún no toca
	if ( gpGlobals->curtime < m_flNextDyingSound )
		return;

	// Próximo sonido de sufrimiento
	m_flNextDyingSound = gpGlobals->curtime + RandomFloat( 25.0f, 40.0f );

	// Estamos incapacitados, no sería lógico.
	if ( IsIncap() )
		return;

	int iHealth = GetHealth();

	// Aún tienes suficiente salud
	if ( iHealth >= 50 )
		return;

	if ( iHealth < 10 )
		EmitPlayerSound("SP.Dying.Critical2");
	else if ( iHealth < 20 )
		EmitPlayerSound("SP.Dying.Critical");
	else if ( iHealth < 30 )
		EmitPlayerSound("SP.Dying.Major");
	else
		EmitPlayerSound("Dying.Minor");
}

//====================================================================
// Actualiza las voces/sonidos
//====================================================================
void CAP_Player::UpdateSounds()
{
	bool bIsTired = false;

	// Esto... no podemos con ello...
	if ( m_flSanity <= 20.0f )
	{
		m_nTired->SetSoundName("Player.Breath.Panic");
		bIsTired = true;
	}

	// Ya no tenemos energía
	if ( m_flStamina <= 30.0f && !bIsTired )
	{
		m_nTired->SetSoundName("Player.Breath.Tired");
		bIsTired = true;
	}

	// Estamos cansados
	if ( bIsTired )
		m_nTired->Play();

	// Sufro!
	DyingSound();
}

//====================================================================
// [Evento] El jugador ha recibido daño pero sigue vivo
//====================================================================
int CAP_Player::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Nos ha causado una herida
	if ( InRules->IsGameMode(GAME_MODE_SURVIVAL) )
	{
		if ( RandomInt(0, 5) == 5 )
		{
			m_bBleed = true;
		}
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//====================================================================
// [Evento] El jugador ha muerto.
//====================================================================
void CAP_Player::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	if ( GetTeamNumber() == TEAM_HUMANS )
	{
		// Iniciamos la música de Muerte
		m_nDeathMusic->Play();
	}
}

//====================================================================
// Pensamiento cuando el Jugador ha muerto y listo para
// respawnear
//====================================================================
void CAP_Player::PlayerDeathPostThink()
{
	// Si no es un Humano o no estamos en una partida Multijugador, usar el código original.
	//if ( GetTeamNumber() != TEAM_HUMANS || !InRules->IsMultiplayer() )
	{
		BaseClass::PlayerDeathPostThink();
		return;
	}

	//
	// CÓDIGO SOLO PARA HUMANOS/SUPERVIVIENTES EN UNA PARTIDA MULTIPLAYER
	//

	// Este Jugador no puede reaparecer
	if ( !InRules->FPlayerCanRespawn(this) )
		return;

	// ¿Hemos presionado algún boton?
	int iAnyButtonDown = ( m_nButtons & ~IN_SCORE );

	// En proceso de reaparición
	m_lifeState = LIFE_RESPAWNABLE;

	// ¿Podemos reaparecer ya?
	bool bCanRespawn = InRules->FPlayerCanRespawnNow( this );

	// Debemos presionar un botón para reaparecer
	if ( !iAnyButtonDown && bCanRespawn )
		bCanRespawn = false;

	// Reaparecemos como Infectado
	if ( bCanRespawn )
	{
		SetNextThink( TICK_NEVER_THINK );
		InRules->ApPointer()->ConvertToInfected( this );

		m_nButtons			= 0;
		m_iRespawnFrames	= 0;
	}
}

//====================================================================
// Actualiza los niveles de Sangre, Comida y Sed
//====================================================================
void CAP_Player::UpdateLifeLevels()
{
	// Solo en Survival
	if ( !InRules->IsGameMode(GAME_MODE_SURVIVAL) )
		return;

	// Hora de disminuir energía
	if ( gpGlobals->curtime >= m_iNextLifeUpdate )
	{
		m_iHungryLevel		-= RandomInt(1, 3);
		m_iThirstLevel		-= RandomInt(1, 3);
		m_iNextLifeUpdate	= NEXT_LIFE_UPDATE;
	}

	// Verificación de datos
	if ( m_iHungryLevel > 100 )
		m_iHungryLevel = 100;
	if ( m_iHungryLevel < 0 )
		m_iHungryLevel = 0;
	if ( m_iThirstLevel > 100 )
		m_iThirstLevel = 100;
	if ( m_iThirstLevel < 0 )
		m_iThirstLevel = 0;

	// TODO
	if ( m_iHungryLevel <= 10 || m_iThirstLevel <= 10 )
	{
		m_bBleed = true;
	}

	// Estamos sangrando
	if ( m_bBleed )
	{
		m_flBloodLevel -= 1 * gpGlobals->frametime;

		// Mientras nuestro nivel de sangre sea mayor a 1
		if ( m_flBloodLevel >= 1 && m_flBloodLevel <= (BLOOD_LEVEL/2) && gpGlobals->curtime >= m_iNextBleedDamage )
		{
			float flDamage = ( 5000 / m_flBloodLevel ) / 100;
			CTakeDamageInfo info( this, this, flDamage, DMG_DIRECT );

			TakeDamage( info );
			m_iNextBleedDamage = gpGlobals->curtime + RandomInt(5, 10);
		}

		// Sin sangre, hemos muerto
		if ( m_flBloodLevel <= 0 )
		{
			m_iHealth = 0;
		}

		// Efectos
		if ( gpGlobals->curtime >= m_iNextBleedEffect )
			BleedEffects();
	}
	else
	{
		m_flBloodLevel += 0.5 * gpGlobals->frametime;
	}

	// Verificación de datos
	if ( m_flBloodLevel > BLOOD_LEVEL )
		m_flBloodLevel = BLOOD_LEVEL;
	if ( m_flBloodLevel < 0 )
		m_flBloodLevel = 0.0f;

	//Msg("[CAP_Player::UpdateLifeLevels] Sangre: %f - Hambre: %i - Sed: %i \n", m_flBloodLevel, m_iHungryLevel, m_iThirstLevel);
}

//====================================================================
// Efectos al sangrar
//====================================================================
void CAP_Player::BleedEffects()
{
	// Solo en Survival
	if ( !InRules->IsGameMode(GAME_MODE_SURVIVAL) )
		return;

	UTIL_BloodSpray( GetAbsOrigin(), vec3_origin, BloodColor(), 5, FX_BLOODSPRAY_GORE );
	UTIL_BloodDrips( GetAbsOrigin(), vec3_origin, BloodColor(), 5 );

	// Put blood on the ground if near enough
	trace_t bloodTrace;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 256 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &bloodTrace);
	
	if ( bloodTrace.fraction < 1.0f )
	{
		UTIL_BloodDecalTrace( &bloodTrace, BloodColor() );
	}

	m_iNextBleedEffect = gpGlobals->curtime + RandomInt(1, 3);
}

//====================================================================
// Actualiza los efectos de una GRAN caida
//====================================================================
void CAP_Player::UpdateFallEffects()
{
	BaseClass::UpdateFallEffects();

	// No podemos tener daño por caida
	if ( IsGod() || GetMoveType() == MOVETYPE_NOCLIP )
		return;

	// No es una velocidad de caida peligrosa
	if ( m_Local.m_flFallVelocity < 600 || IsInGround() )
	{
		m_nFall->Stop();
		return;
	}

	m_nFall->Play();
}

//====================================================================
// Pensamiento: Al estar incapacitado
//====================================================================
void CAP_Player::UpdateIncap()
{
	BaseClass::UpdateIncap();

	// No estamos incapacitados
	if ( !IsIncap() )
	{
		m_nIncapMusic->Fadeout();
		m_nClimbingIncapMusic->Fadeout();
		m_nPreDeathMusic->Fadeout();

		return;
	}

	//
	// Incapacitación en el suelo
	//
	if ( !IsClimbingIncap() )
	{

		// ¡Estamos muriendo!
		if ( GetHealth() <= 15 )
		{
			if ( !m_nPreDeathMusic->IsPlaying() )
			{
				color32 black = { 0, 0, 0, 255 };
				UTIL_ScreenFade( this, black, 30.0f, 0.5f, FFADE_OUT | FFADE_PURGE );
			}

			m_nIncapMusic->Fadeout();
			m_nPreDeathMusic->Play();
		}
		else
		{
			m_nIncapMusic->Play();
		}

		// ¡Nos quedamos sordos! 
		if ( GetHealth() < 30 )
		{
			int effect = 14;

			if ( GetHealth() < 15 )
				effect = 15;

			CSingleUserRecipientFilter user( this );
			enginesound->SetPlayerDSP( user, effect, false );
		}

		return;
	}

	//
	// Incapacitación por quedar colgando
	//

	float climbingHold	= GetClimbingHold();

	if ( IsWaitingGroundDeath() )
	{
		m_nClimbingIncapMusic->SetSoundName( "Player.Music.ClimbIncap.Fall" );
	}
	else
	{
		if ( climbingHold < 100 )
		{
			m_nClimbingIncapMusic->SetSoundName( "Player.Music.ClimbIncap.Dangle" );
		}
		else if ( climbingHold < 250 )
		{
			m_nClimbingIncapMusic->SetSoundName("Player.Music.ClimbIncap.Weak");
		}
		else
		{
			m_nClimbingIncapMusic->SetSoundName( "Player.Music.ClimbIncap.Firm" );
		}
	}

	m_nClimbingIncapMusic->Play();
}

//====================================================================
//====================================================================
void CAP_Player::OnStartIncap()
{
	SendLesson( "instructor_incap" );
}

//====================================================================
//====================================================================
void CAP_Player::OnStopIncap()
{
	CSingleUserRecipientFilter user( this );
	enginesound->SetPlayerDSP( user, 0, false );
}

//====================================================================
// Permite ejecutar un comando no registrado
//====================================================================
bool CAP_Player::ClientCommand( const CCommand &args )
{
	if ( FStrEq(args[0], "join_infected") )
	{
		InRules->ApPointer()->ConvertToInfected( this );
		return true;
	}

	if ( FStrEq(args[0], "join_survivor") )
	{
		InRules->ApPointer()->ConvertToHuman( this );
		return true;
	}

	return BaseClass::ClientCommand( args );
}