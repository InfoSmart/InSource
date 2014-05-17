//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef DIRECTOR_MUSIC_H
#define DIRECTOR_MUSIC_H

#pragma once

#include "envmusic.h"
#include "layersound.h"

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

	CLayerSound *m_nBossMusic;
	CLayerSound *m_nClimaxMusic;
	CLayerSound *m_nGameoverMusic;

	friend class CDirector;
};

#ifndef CUSTOM_DIRECTOR_MUSIC
	extern CDirectorMusic *DirectorMusic;
#endif

#endif // DIRECTOR_MUSIC_H