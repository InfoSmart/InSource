//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef DIRECTOR_ITEM_MANAGER_H
#define DIRECTOR_ITEM_MANAGER_H

#pragma once

#include "director_spawn.h"

class CDirectorItemManager : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS( CDirectorItemManager, CAutoGameSystemPerFrame );

	// Principales
	virtual bool Init();

	//virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();

	//virtual void LevelShutdownPreEntity();
	//virtual void LevelShutdownPostEntity();

	//virtual void FrameUpdatePreEntityThink();
	virtual void FrameUpdatePostEntityThink();

	// Utils
	virtual void FindSpawns();

	// Creación
	virtual void Update();

protected:
	CUtlVector<CDirectorSpawn *> m_nSpawnEntities;
};

extern CDirectorItemManager *DirectorItemManager;

#endif