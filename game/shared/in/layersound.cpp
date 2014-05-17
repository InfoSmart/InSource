//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "layersound.h"

#ifdef CLIENT_DLL
	#include "layersound_manager.h"
#endif

#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;

//====================================================================
// Comandos
//====================================================================

//====================================================================
// Información y Red
//====================================================================

LINK_ENTITY_TO_CLASS( layer_sound, CLayerSound );

IMPLEMENT_NETWORKCLASS_ALIASED( LayerSound, DT_LayerSound );

BEGIN_NETWORK_TABLE( CLayerSound, DT_LayerSound )
	
#ifndef CLIENT_DLL
	SendPropString( SENDINFO(m_nSoundName) ),
	SendPropBool( SENDINFO(m_bIsLoop) ),

	SendPropEHandle( SENDINFO(m_nSource) ),
	SendPropEHandle( SENDINFO(m_nPlayerListener) ),
	SendPropBool( SENDINFO(m_nPlayerExcept) ),

	SendPropInt( SENDINFO(m_iTeam) ),
	SendPropInt( SENDINFO(m_iLayer) ),
#else
	RecvPropString( RECVINFO(m_nSoundName) ),
	RecvPropBool( RECVINFO(m_bIsLoop) ),

	RecvPropEHandle( RECVINFO(m_nSource) ),
	RecvPropEHandle( RECVINFO(m_nPlayerListener) ),
	RecvPropBool( RECVINFO(m_nPlayerExcept) ),

	RecvPropInt( RECVINFO(m_iTeam) ),
	RecvPropInt( RECVINFO(m_iLayer) ),
#endif

END_NETWORK_TABLE()

#ifndef CLIENT_DLL
BEGIN_DATADESC( CLayerSound )
	DEFINE_FIELD( m_bIsPlaying, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flSoundFinish, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSoundDuration, FIELD_FLOAT ),

	DEFINE_FIELD( m_nSoundName, FIELD_SOUNDNAME ),
	DEFINE_FIELD( m_bIsLoop, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_nSource, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_nPlayerListener, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_nPlayerExcept, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_iTeam, FIELD_INTEGER ),
	DEFINE_FIELD( m_iLayer, FIELD_INTEGER ),
END_DATADESC()
#endif

//====================================================================
// Creación en el mundo
//====================================================================
void CLayerSound::Spawn()
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	SetModelName( NULL_STRING );

	AddEffects( EF_NODRAW );
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

#ifdef CLIENT_DLL
	// Agregamos este sonido a nuestra interfaz
	LayerSoundManager->AddSound( this );

	SetNextClientThink( CLIENT_THINK_ALWAYS );
#endif

	BaseClass::Spawn();	
}

//====================================================================
// Inicia y obtiene la configuración del sonido
//====================================================================
bool CLayerSound::CreateSound()
{
#ifndef CLIENT_DLL
	// No hay información del sonido, probablemente no exista
	if ( !soundemitterbase->GetParametersForSound(m_nSoundName.Get(), m_nParameters, GENDER_NONE) )
		return false;
#else
	// No hay información del sonido, probablemente no exista
	if ( !soundemitterbase->GetParametersForSound(m_nSoundName, m_nParameters, GENDER_NONE) )
		return false;
#endif

	// Obtenemos la duración del sonido
	m_flSoundDuration = enginesound->GetSoundDuration( m_nParameters.soundname );

	#ifdef CLIENT_DLL

		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		// Necesitamos al Jugador local
		if ( !pPlayer )
			return false;

		// Usamos al Jugador que estamos espectando
		if ( !pPlayer->IsAlive() && (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE || pPlayer->GetObserverMode() == OBS_MODE_CHASE) && pPlayer->GetObserverTarget() )
			pPlayer = ToBasePlayer( pPlayer->GetObserverTarget() );

		int index = pPlayer->entindex();

		// Creamos nuevamente el sonido
		if ( m_nSoundPatch )
		{
			ENVELOPE_CONTROLLER.SoundDestroy( m_nSoundPatch );
			m_nSoundPatch = NULL;
		}

		CLocalPlayerFilter filter;

		// Proviene de una entidad
		if ( m_nSource.Get() )
		{
			CPASAttenuationFilter filter( m_nSource.Get(), m_nParameters.soundlevel );
			index = m_nSource.Get()->entindex();
		}

		// Creamos el sonido
		m_nSoundPatch = ENVELOPE_CONTROLLER.SoundCreate( filter, index, m_nParameters.channel, m_nParameters.soundname, m_nParameters.soundlevel );

		// Hubo un problema al crear el sonido
		if ( !m_nSoundPatch )
			return false;

	#endif // CLIENT_DLL

	return true;
}

//====================================================================
// Reproduce el sonido
//====================================================================
void CLayerSound::Play( float flVolume )
{
	// Ya estamos reproduciendo
	if ( IsPlaying() )
		return;

	// Algo ha ocurrido mal
	if ( !CreateSound() )
		return;

	// Debemos usar el volumen original
	if ( flVolume <= -1 )
		flVolume = m_nParameters.volume;

#ifdef CLIENT_DLL
	// No puedes escuchar el sonido
	if ( !CanHearSound() )
		return;

	DevMsg( "[CLayerSound::Play][CLIENT] Reproduciendo %s \n", m_nSoundName );

	ENVELOPE_CONTROLLER.Play( m_nSoundPatch, flVolume, m_nParameters.pitch );
	m_flVolume = flVolume;
#else
	DevMsg( "[CLayerSound::Play][SERVER] Reproduciendo %s \n", m_nSoundName.Get() );

	EntityMessageBegin( this );
		WRITE_BYTE( LAYER_SOUND_PLAY );
		WRITE_FLOAT( flVolume );
	MessageEnd();

	// Tag Sound
	if ( m_nTagSound )
		m_nTagSound->Play();
#endif

	m_bIsPlaying		= true;
	m_flSoundFinish		= gpGlobals->curtime + m_flSoundDuration;
}

//====================================================================
// Paramos el sonido
//====================================================================
void CLayerSound::Stop()
{
	// No estamos reproduciendo
	if ( !IsPlaying() )
		return;

#ifdef CLIENT_DLL
	// No se ha preparado el sonido
	if ( !m_nSoundPatch )
		return;

	DevMsg( "[CLayerSound::Stop][CLIENT] Parando: %s \n", m_nSoundName );

	SetVolume( 0.0f );
	ENVELOPE_CONTROLLER.Shutdown( m_nSoundPatch );
#else
	DevMsg( "[CLayerSound::Stop][SERVER] Parando: %s \n", m_nSoundName.Get() );

	EntityMessageBegin( this );
		WRITE_BYTE( LAYER_SOUND_STOP );
	MessageEnd();

	// Tag Sound
	if ( m_nTagSound )
		m_nTagSound->Stop();
#endif

	m_bIsPlaying = false;
}

//====================================================================
// Para el sonido desvaneciendolo
//====================================================================
void CLayerSound::Fadeout( float flRange, bool bDestroy )
{
	// No estamos reproduciendo
	if ( !IsPlaying() )
		return;

#ifdef CLIENT_DLL
	// No se ha preparado el sonido
	if ( !m_nSoundPatch )
		return;

	DevMsg( "[CLayerSound::Fadeout][CLIENT] Parando: %s en %.1f \n", m_nSoundName, flRange );

	ENVELOPE_CONTROLLER.SoundFadeOut( m_nSoundPatch, flRange, bDestroy );
#else
	DevMsg( "[CLayerSound::Fadeout][SERVER] Parando: %s en %.1f \n", m_nSoundName.Get(), flRange );

	EntityMessageBegin( this );
		WRITE_BYTE( LAYER_SOUND_FADEOUT );
		WRITE_FLOAT( flRange );
		WRITE_SHORT( bDestroy );
	MessageEnd();

	// Tag Sound
	if ( m_nTagSound )
		m_nTagSound->Fadeout( flRange, bDestroy );
#endif

	m_bIsPlaying = false;
}

//====================================================================
// Para el sonido desvaneciendolo
//====================================================================
void CLayerSound::SetVolume( float flValue )
{
#ifdef CLIENT_DLL
	// No se ha preparado el sonido
	if ( !m_nSoundPatch )
		return;

	// Hay un sonido con una capa mayor reproduciendose
	if ( IsBlocked() )
	{
		m_flVolume = flValue;
		return;
	}

	// Restauramos el volumen original
	if ( flValue <= VOLUME_ORIGINAL )
		flValue = m_nParameters.volume;

	if ( flValue > 1.0f )
		flValue = 1.0f;

	float delta = 0.5f;

	if ( flValue <= 0.0f )
		delta = 0.0f;

	ENVELOPE_CONTROLLER.SoundChangeVolume( m_nSoundPatch, flValue, delta );
	m_flVolume = flValue;
#else
	EntityMessageBegin( this );
		WRITE_BYTE( LAYER_SOUND_VOLUME );
		WRITE_FLOAT( flValue );
	MessageEnd();

	// Tag Sound
	if ( m_nTagSound )
		m_nTagSound->SetVolume( flValue );
#endif
}

//====================================================================
// Establece el Pitch
//====================================================================
void CLayerSound::SetPitch( float flValue )
{
#ifdef CLIENT_DLL
	// No se ha preparado el sonido
	if ( !m_nSoundPatch )
		return;

	float delta = 0.5f;

	if ( flValue <= 0.0f )
		delta = 0.0f;

	ENVELOPE_CONTROLLER.SoundChangePitch( m_nSoundPatch, flValue, delta );
#else
	EntityMessageBegin( this );
		WRITE_BYTE( LAYER_SOUND_PITCH );
		WRITE_FLOAT( flValue );
	MessageEnd();

	// Tag Sound
	if ( m_nTagSound )
		m_nTagSound->SetPitch( flValue );
#endif
}

#ifndef CLIENT_DLL

//====================================================================
// Crea un sonido por capa
//====================================================================
CLayerSound *CreateLayerSound( const char *pSoundName, int iLevel, bool bIsLoop )
{
	CLayerSound *pSound = (CLayerSound *)CreateEntityByName( "layer_sound" );

	DispatchSpawn( pSound );
	pSound->Activate();

	pSound->Prepare( pSoundName, (LayerLevel)iLevel, bIsLoop );

	return pSound;
}

//====================================================================
// Constructor
//====================================================================
void CLayerSound::Prepare( const char *pSoundName, LayerLevel iLevel, bool bIsLoop )
{
	memset( m_nSoundName.GetForModify(), 0, sizeof(m_nSoundName) );
	Q_strncpy( m_nSoundName.GetForModify(), pSoundName, 255 );

	m_bIsPlaying		= false;
	m_bIsLoop			= bIsLoop;
	m_flSoundFinish		= 0.0f;
	m_flSoundDuration	= 0.0f;

	m_nTagSound			= NULL;
	m_nSource			= NULL;
	m_nPlayerListener	= NULL;
	m_iTeam				= TEAM_ANY;
	m_iLayer			= iLevel;
}

//====================================================================
// Estable/cambia el sonido
//====================================================================
void CLayerSound::SetSoundName( const char *pSoundName )
{
	// ¡Es el mismo!
	if ( FStrEq(m_nSoundName.Get(), pSoundName) )
		return;

	// ¿Estamos reproduciendo ahora mismo?
	bool iPlaying = IsPlaying();

	// Paramos la reproducción
	Stop();

	// Establecemos el nuevo sonido
	Q_strncpy( m_nSoundName.GetForModify(), pSoundName, 255 );

	// Volvemos a reproducir
	if ( iPlaying )
		Play();
}

//====================================================================
// Establece el sonido "tag"
//====================================================================
void CLayerSound::SetTagSound( CLayerSound *pSound )
{
	m_nTagSound = pSound;
}

//====================================================================
// Establece el sonido "tag"
//====================================================================
void CLayerSound::SetTagSound( const char *pSoundName, CBasePlayer *pPlayer )
{
	m_nTagSound = CreateLayerSound( pSoundName, m_iLayer );
	m_nTagSound->SetOnlyTeam( m_iTeam );

	if ( !pPlayer && m_nPlayerListener )
		pPlayer = m_nPlayerListener;

	if ( pPlayer )
		m_nTagSound->SetListener( pPlayer, true );
}

//====================================================================
// Establece de donde vendrá el sonido
//====================================================================
void CLayerSound::SetSource( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return;

	m_nSource = pEntity;
}

//====================================================================
// Establece el Jugador que podrá escuchar el sonido
//====================================================================
void CLayerSound::SetListener( CBasePlayer *pPlayer, bool bExceptThis )
{
	m_nPlayerListener	= pPlayer;
	m_nPlayerExcept		= bExceptThis;
}

//====================================================================
// Establece el equipo que podrá escuchar el sonido
//====================================================================
void CLayerSound::SetOnlyTeam( int iTeam )
{
	m_iTeam = iTeam;
}

//====================================================================
//====================================================================
int CLayerSound::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

#else // CLIENT_DLL

//====================================================================
// Destructor
//====================================================================
CLayerSound::~CLayerSound()
{
	// Lo eliminamos de la interfaz
	LayerSoundManager->RemoveSound( this );
}

//====================================================================
// Pensamiento
//====================================================================
void CLayerSound::ClientThink()
{
	BaseClass::ClientThink();

	// Reproduciendo
	if ( IsPlaying() && !IsLoop() )
	{
		// El sonido ha acabado de reproducirse
		if ( gpGlobals->curtime >= m_flSoundFinish )
			m_bIsPlaying = false;
	}

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//====================================================================
// Devuelve si el Jugador local puede oir el sonido
//====================================================================
bool CLayerSound::CanHearSound()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	bool canHear = true;

	if ( !pPlayer )
		return false;

	// Usamos al Jugador que estamos espectando
	if ( !pPlayer->IsAlive() && (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE || pPlayer->GetObserverMode() == OBS_MODE_CHASE) && pPlayer->GetObserverTarget() )
		pPlayer = ToBasePlayer( pPlayer->GetObserverTarget() );

	if ( m_nPlayerListener )
	{
		// Solo un jugador no podrá oirlo
		if ( m_nPlayerExcept )
		{
			if ( m_nPlayerListener.Get() == pPlayer )
				canHear = false;
		}

		// Solo un Jugador puede oirlo
		else
		{
			if ( m_nPlayerListener.Get() != pPlayer )
				canHear = false;
		}
	}

	// Este sonido no es para tu equipo
	if ( m_iTeam != TEAM_ANY && m_iTeam != pPlayer->GetTeamNumber() )
		canHear = false;

	return canHear;
}

//====================================================================
// [Evento] Hemos recibido un mensaje del servidor
//====================================================================
void CLayerSound::ReceiveMessage( int classID, bf_read &msg )
{
	// Mensaje para la clase padre
	if ( classID != GetClientClass()->m_ClassID )
	{
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();

	switch ( messageType )
	{
		case LAYER_SOUND_PLAY:
			Play( msg.ReadBitFloat() );
		break;

		case LAYER_SOUND_STOP:
			Stop();
		break;

		case LAYER_SOUND_FADEOUT:
			Fadeout( msg.ReadBitFloat(), msg.ReadShort() );
		break;

		case LAYER_SOUND_VOLUME:
			SetVolume( msg.ReadBitFloat() );
		break;

		case LAYER_SOUND_PITCH:
			SetPitch( msg.ReadBitFloat() );
		break;
	}
}

#endif // CLIENT_DLL