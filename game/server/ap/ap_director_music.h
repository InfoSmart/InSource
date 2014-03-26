//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef AP_DIRECTOR_MUSIC_H
#define AP_DIRECTOR_MUSIC_H

#pragma once

#include "director_music.h"

class CAP_DirectorMusic : public CDirectorMusic
{
public:
	DECLARE_CLASS( CAP_DirectorMusic, CDirectorMusic );

	virtual void Precache();
	virtual void Init();

	virtual void Think();

	virtual void Stop();
	virtual void Shutdown();

	virtual void Update();

	virtual void UpdateNormal();
	virtual void UpdateSurvival();

	virtual void UpdateAmbient();

protected:
	EnvMusic *m_nAngry;
	EnvMusic *m_nSurprise;
	EnvMusic *m_nTooClose;
	EnvMusic *m_nTerror;

	EnvMusic *m_nPanic;
	EnvMusic *m_nPanicBackground;

	CountdownTimer m_nNextAmbientMusic;
	CountdownTimer m_nNextEffectsMusic;

	int m_iLastVisible;
};

extern CAP_DirectorMusic *DirectorMusic;

#endif // AP_DIRECTOR_H