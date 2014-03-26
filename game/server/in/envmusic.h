//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef ENVMUSIC_H
#define ENVMUSIC_H

#pragma once

#include "soundent.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"

#define ENVELOPE_CONTROLLER (CSoundEnvelopeController::GetController())

//=========================================================
// >> EnvMusic
//=========================================================
class EnvMusic
{
public:
	DECLARE_CLASS_NOBASE( EnvMusic );

	EnvMusic( const char *pName );
	~EnvMusic();

	virtual float GetDuration() { return m_flSoundDuration; }
	virtual bool IsPlaying() { return m_bIsPlaying; }

	virtual bool Init();
	virtual void Update();

	virtual void Play( float flVolume = -1 );
	virtual void Stop();
	virtual void Fadeout( float flRange = 3.0f, bool bDestroy = false );

	virtual void SetVolume( float flValue = -1 );
	virtual void SetPitch( float flValue );

	virtual void SetSoundName( const char *pName );

	virtual void SetTagSound( const char *pName );
	virtual void SetTagSound( EnvMusic *pMusic );

	virtual void SetFrom( CBaseEntity *pEntity );
	virtual void SetOnlyToTeam( int iTeam );
	virtual void SetExceptPlayer( bool bValue ) { m_bExceptPlayer = bValue; }

protected:
	const char *m_nSoundName;
	float m_flSoundDuration;
	float m_flSoundFinish;
	bool m_bIsPlaying;

	CSoundPatch *m_nPatch;
	CSoundParameters m_nParameters;
	EnvMusic *m_nTagMusic;

	CBaseEntity *m_nFromEntity;
	int m_iTeam;
	bool m_bExceptPlayer;
	
};

#endif ENVMUSIC_H