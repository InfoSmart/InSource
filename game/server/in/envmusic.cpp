//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"
#include "envmusic.h"

#include "directordefs.h"

#include "in_gamerules.h"
#include "in_player.h"
#include "players_manager.h"

#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;

//=========================================================
// Constructor
//=========================================================
EnvMusic::EnvMusic( const char *pName )
{
	// Establecemos el nombre del sonido y reiniciamos las variables
	m_nSoundName		= pName;
	m_flSoundDuration	= 0.0f;
	m_bIsPlaying		= false;

	m_nPatch		= NULL;
	m_nTagMusic		= NULL;

	m_nFromEntity	= NULL;
	m_iTeam			= TEAM_ANY;
	m_bExceptPlayer	= false;
}

//=========================================================
// Destructor
//=========================================================
EnvMusic::~EnvMusic()
{
	if ( m_nPatch )
	{
		ENVELOPE_CONTROLLER.SoundDestroy( m_nPatch );
		m_nPatch = NULL;
	}
}

//=========================================================
// Inicializamos
//=========================================================
bool EnvMusic::Init()
{
	// No se encontro información del sonido
	if ( !soundemitterbase->GetParametersForSound(m_nSoundName, m_nParameters, GENDER_NONE) )
		return false;

	// Necesitamos al primer Jugador
	if ( !PlysManager->GetLocal() )
		return false;

	if ( m_nPatch )
	{
		ENVELOPE_CONTROLLER.SoundDestroy( m_nPatch );
		m_nPatch = NULL;
	}

	// TODO: Reproducir desde un origen/entidad pero solo a un equipo

	// Obtenemos la duración
	m_flSoundDuration = enginesound->GetSoundDuration( m_nParameters.soundname );

	// Debemos reproducir la música desde una entidad
	if ( m_nFromEntity )
	{
		// Es un jugador, en este caso haremos algo especial
		if ( m_nFromEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( m_nFromEntity );

			// Es música que debe ser escuchada por los demás jugadores pero no por el mismo
			if ( m_bExceptPlayer )
			{
				CBroadcastRecipientFilter filter;
				filter.MakeReliable();
				filter.RemoveRecipient( pPlayer );

				m_nPatch = ENVELOPE_CONTROLLER.SoundCreate( filter, pPlayer->entindex(), m_nParameters.channel, m_nParameters.soundname, m_nParameters.soundlevel );
			}

			// Solo el jugador podrá escuchar la música
			else
			{
				CSingleUserRecipientFilter filter( pPlayer );
				filter.MakeReliable();

				m_nPatch = ENVELOPE_CONTROLLER.SoundCreate( filter, pPlayer->entindex(), m_nParameters.channel, m_nParameters.soundname, m_nParameters.soundlevel );
			}
		}

		// Desde un NPC, objeto, entidad
		else
		{
			CPASAttenuationFilter filter( m_nFromEntity, m_nParameters.soundlevel );
			filter.MakeReliable();

			m_nPatch = ENVELOPE_CONTROLLER.SoundCreate( filter, m_nFromEntity->entindex(), m_nParameters.channel, m_nParameters.soundname, m_nParameters.soundlevel );
		}
	}

	// Solo para un equipo
	else if ( m_iTeam != TEAM_ANY )
	{
		CTeamRecipientFilter filter( m_iTeam, true );
		filter.MakeReliable();

		m_nPatch = ENVELOPE_CONTROLLER.SoundCreate( filter, PlysManager->GetLocal()->entindex(), m_nParameters.channel, m_nParameters.soundname, m_nParameters.soundlevel );
	}

	// Para todos
	else
	{
		CReliableBroadcastRecipientFilter filter;
		m_nPatch = ENVELOPE_CONTROLLER.SoundCreate( filter, PlysManager->GetLocal()->entindex(), m_nParameters.channel, m_nParameters.soundname, m_nParameters.soundlevel );
	}

	// Ha ocurrido un problema
	if ( !m_nPatch )
		return false;

	return true;
}

//=========================================================
//=========================================================
void EnvMusic::Update()
{
	// Estamos reproduciendo
	if ( IsPlaying() )
	{
		// El sonido ha acabado de reproducirse
		if ( gpGlobals->curtime > m_flSoundFinish )
			m_bIsPlaying = false;
	}
}

//=========================================================
// Reproduce la música
//=========================================================
void EnvMusic::Play( float flVolume )
{
	// Estamos reproduciendo
	if ( IsPlaying() )
		return;

	// Algo ocurrio mal
	if ( !Init() )
		return;

	// Debemos usar el volumen del sonido
	if ( flVolume <= -1 )
		flVolume = m_nParameters.volume;

	ConColorMsg( DIRECTOR_CON_MUSIC, "[EnvMusic::Play] Reproduciendo: %s <Duración: %f><Volumen: %f> - %s \n", m_nSoundName, m_flSoundDuration, flVolume, m_nParameters.soundname );

	// Reproducimos
	ENVELOPE_CONTROLLER.Play( m_nPatch, flVolume, m_nParameters.pitch );
	m_bIsPlaying = true;

	m_flSoundFinish = gpGlobals->curtime + m_flSoundDuration;

	if ( m_nTagMusic )
		m_nTagMusic->Play();
}

//=========================================================
// Para la música
//=========================================================
void EnvMusic::Stop()
{
	// No estamos reproduciendo
	if ( !IsPlaying() )
		return;

	// Algo ocurrio mal
	if ( !m_nPatch )
		return;

	ConColorMsg( DIRECTOR_CON_MUSIC, "[EnvMusic::Stop] Parando: %s \n", m_nSoundName );

	// Paramos
	SetVolume( 0.0f );
	ENVELOPE_CONTROLLER.Shutdown( m_nPatch );

	m_bIsPlaying = false;

	if ( m_nTagMusic )
		m_nTagMusic->Stop();
}

//=========================================================
// Para la música con un efecto de desvanecimiento
//=========================================================
void EnvMusic::Fadeout( float flRange, bool bDestroy )
{
	// No estamos reproduciendo
	if ( !IsPlaying() )
		return;

	// Algo ocurrio mal
	if ( !m_nPatch )
		return;

	ConColorMsg( DIRECTOR_CON_MUSIC, "[EnvMusic::Fadeout] Desvaneciendo %s en %f \n", m_nSoundName, flRange );

	// Paramos
	ENVELOPE_CONTROLLER.SoundFadeOut( m_nPatch, flRange, bDestroy );
	m_bIsPlaying = false;

	if ( m_nTagMusic )
		m_nTagMusic->Fadeout( flRange, bDestroy );
}

//=========================================================
// Establece el volumen de la música
//=========================================================
void EnvMusic::SetVolume( float flValue )
{
	// Algo ocurrio mal
	if ( !m_nPatch )
		return;

	float flDelta = 0.9f;

	if ( flValue == 0.0f )
		flDelta = 0.0f;

	// Debemos usar el volumen del sonido
	if ( flValue <= -1 )
		flValue = m_nParameters.volume;

	// El volumen no debe superar 1
	if ( flValue > 1.0f )
		flValue = 1.0f;

	ENVELOPE_CONTROLLER.SoundChangeVolume( m_nPatch, flValue, flDelta );
	
	if ( m_nTagMusic )
		m_nTagMusic->SetVolume( flValue );
}

//=========================================================
// Establece el Pitch de la música
//=========================================================
void EnvMusic::SetPitch( float flValue )
{
	// Algo ocurrio mal
	if ( !m_nPatch )
		return;

	float flDelta = 0.9f;

	if ( flValue == 0.0f )
		flDelta = 0.0f;

	ENVELOPE_CONTROLLER.SoundChangePitch( m_nPatch, flValue, flDelta );
	
	if ( m_nTagMusic )
		m_nTagMusic->SetPitch( flValue );
}

//=========================================================
// Establece/Cambia el sonido
//=========================================================
void EnvMusic::SetSoundName( const char *pName )
{
	if ( m_nSoundName == pName )
		return;

	// ¿La música estaba en reproducción?
	bool bIsPlaying = m_bIsPlaying;

	// Paramos de reproducir
	Stop();

	// Establecemos el nuevo sonido
	m_nSoundName = pName;

	// Si se estaba reproduciendo, volver a reproducir
	if ( bIsPlaying )
		Play();
}

//=========================================================
// Establece la música que se reproducira a los demás
// jugadores excepto el actual
//=========================================================
void EnvMusic::SetTagSound( const char *pName )
{
	// No se especifico un jugador para la música actual
	if ( !m_nFromEntity || !m_nFromEntity->IsPlayer() )
		return;

	m_nTagMusic = new EnvMusic( pName );
	m_nTagMusic->SetFrom( m_nFromEntity );
	m_nTagMusic->SetExceptPlayer( true );
}

//=========================================================
// Establece la música que se reproducira a los demás
// jugadores excepto el actual
//=========================================================
void EnvMusic::SetTagSound( EnvMusic *pMusic )
{
	// No se especifico un jugador para la música actual
	if ( !m_nFromEntity || !m_nFromEntity->IsPlayer() )
		return;

	m_nTagMusic = pMusic;
}

//=========================================================
// Establece de donde vendrá la música
//=========================================================
void EnvMusic::SetFrom( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return;

	m_nFromEntity = pEntity;
}

//=========================================================
// Establece si la música solo se reproducirá a un equipo
//=========================================================
void EnvMusic::SetOnlyToTeam( int iTeam )
{
	m_iTeam = iTeam;
}