//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef LAYERSOUND_MANAGER_H
#define LAYERSOUND_MANAGER_H

#pragma once

#include "layersound.h"

//====================================================================
// CLayerSoundManager
//
// >> Procesa todos los sonidos por capas (CLayerSound) y realiza los
// cambios necesarios en cliente
//====================================================================
class CLayerSoundManager : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS( CLayerSoundManager, CAutoGameSystemPerFrame );

	virtual void LevelInitPreEntity();

	virtual void Update( float frametime );
	virtual void UpdateSounds( LayerLevel iMaxLevel );

	virtual void AddSound( CLayerSound *pSound );
	virtual void AddSound( CLayerSound *pSound, LayerLevel iLevel );

	virtual void RemoveSound( CLayerSound *pSound );

protected:
	CUtlVector<CLayerSound *> m_nAllSounds;
	CUtlVector<CLayerSound *> m_nSounds[LAYER_MAX_COUNT];
};

extern CLayerSoundManager *LayerSoundManager;

#endif // LAYERSOUND_MANAGER_H