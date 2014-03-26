//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef DIRECTOR_MUSIC_H
#define DIRECTOR_MUSIC_H

#pragma once

#include "envmusic.h"

//=========================================================
// >> CDirector_Music
//=========================================================
class CDirectorMusic
{
public:
	DECLARE_CLASS_NOBASE( CDirectorMusic );

	virtual void Precache();
	virtual void Init();

	virtual void OnNewMap();
	virtual void Think();

	virtual void Stop();
	virtual void Shutdown();

	virtual void Update();

protected:

	EnvMusic *m_nBossMusic;
	EnvMusic *m_nClimaxMusic;
	EnvMusic *m_nGameoverMusic;

	friend class CDirector;
};

#ifndef CUSTOM_DIRECTOR_MUSIC
extern CDirectorMusic *DirectorMusic;
#endif

#endif // DIRECTOR_MUSIC_H