//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef LAYERSOUND_H
#define LAYERSOUND_H

#pragma once

//#include "soundent.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"

enum LayerLevel
{
	LAYER_NO = 0,		// Capa para ignorar las capas
	LAYER_VERYLOW,
	LAYER_LOW,			// Música de ambiente
	LAYER_MEDIUM,		
	LAYER_HIGH,			// Música del Climax y Jefe
	LAYER_VERYHIGH,		// Música de muerte del Jugador, cuando esta a punto de morir y al estar trepando
	LAYER_TOP,			// Música de Gameover

	LAYER_MAX_COUNT
};

enum
{
	LAYER_SOUND_PLAY = 1,
	LAYER_SOUND_STOP,
	LAYER_SOUND_FADEOUT,
	LAYER_SOUND_VOLUME,
	LAYER_SOUND_PITCH,
};

#define VOLUME_ORIGINAL -1
#define ENVELOPE_CONTROLLER (CSoundEnvelopeController::GetController())

#ifdef CLIENT_DLL
	#define CLayerSound C_LayerSound
#endif

//====================================================================
// CLayerSound
//
// >> Permite reproducir y administrar sonidos por niveles de capas, 
// bajando el volumen/prioridad de las capas más pequeñas
//====================================================================
class CLayerSound : public CBaseEntity
{
public:
	DECLARE_CLASS( CLayerSound, CBaseEntity );
	DECLARE_NETWORKCLASS();

	virtual void Spawn();
	virtual bool CreateSound();

	virtual void Play( float flVolume = VOLUME_ORIGINAL );
	virtual void Stop();
	virtual void Fadeout( float flRange = 2.0f, bool bDestroy = true );

	virtual void SetVolume( float flValue );
	virtual void SetPitch( float flValue );

	virtual bool IsPlaying() { return m_bIsPlaying; }
	virtual bool IsLoop() { return m_bIsLoop; }

	virtual float GetDuration() { return m_flSoundDuration; }

#ifndef CLIENT_DLL
	DECLARE_DATADESC();

	virtual const char *GetSoundName() { return m_nSoundName.Get(); }
	virtual LayerLevel GetLayer() { return (LayerLevel)m_iLayer.Get(); }

	virtual int UpdateTransmitState();
	virtual void Prepare( const char *pSoundName, LayerLevel iLevel = LAYER_MEDIUM, bool bIsLoop = false );

	virtual void SetSoundName( const char *pSoundName );

	virtual void SetTagSound( CLayerSound *pSound );
	virtual void SetTagSound( const char *pSoundName, CBasePlayer *pPlayer = NULL );

	virtual void SetSource( CBaseEntity *pEntity );
	virtual void SetListener( CBasePlayer *pPlayer, bool bExceptThis = false );
	virtual void SetOnlyTeam( int iTeam );
#else
	~CLayerSound();
	virtual LayerLevel GetLayer() { return (LayerLevel)m_iLayer; }
	 const char *GetSoundName() { return m_nSoundName; }

	virtual void ClientThink();

	virtual bool CanHearSound();
	virtual void ReceiveMessage( int classID, bf_read &msg );

	virtual void SetBlock( bool bValue ) { m_bIsBlocked = bValue; }
	virtual bool IsBlocked() { return m_bIsBlocked; }

	virtual float GetVolume() { return m_flVolume; }
#endif

protected:
	bool m_bIsPlaying;

	float m_flSoundFinish;
	float m_flSoundDuration;

	CSoundParameters m_nParameters;

#ifdef CLIENT_DLL
	CSoundPatch *m_nSoundPatch;

	char m_nSoundName[255];
	bool m_bIsLoop;

	bool m_bIsBlocked;
	float m_flVolume;

	EHANDLE m_nSource;
	EHANDLE m_nPlayerListener;
	bool m_nPlayerExcept;

	int m_iTeam;
	int m_iLayer;

	friend class CLayerSoundManager;
#else
	CLayerSound *m_nTagSound;

	CNetworkString( m_nSoundName, 255 );
	CNetworkVar( bool, m_bIsLoop );

	CNetworkHandle( CBaseEntity, m_nSource );
	CNetworkHandle( CBasePlayer, m_nPlayerListener );
	CNetworkVar( bool, m_nPlayerExcept );

	CNetworkVar( int, m_iTeam );
	CNetworkVar( int, m_iLayer );
#endif
};

#ifndef CLIENT_DLL
	CLayerSound *CreateLayerSound( const char *pSoundName, int iLevel = LAYER_MEDIUM, bool bIsLoop = false );
#endif

#endif // LAYERSOUND_H