//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_DIRECTOR_H
#define AP_DIRECTOR_H

#pragma once

#include "ap_player.h"
#include "in_shareddefs.h"
#include "director.h"

//=========================================================
// >> CAP_Director
//=========================================================
class CAP_Director : public CDirector
{
public:
	DECLARE_CLASS( CAP_Director, CDirector );

	virtual void LevelInitPreEntity();

	// Utilidades
	virtual float MaxDistance();
	virtual void Disclose( CBaseEntity *pEntity );

	// Calculos/Verificación
	virtual bool IsMode( int iStatus );

	// Estados
	virtual void SetStatus( DirectorStatus iStatus );

	// Hijos
	virtual int MaxChilds();
	virtual void CheckChilds();

	virtual bool CheckNewEnemy( CBaseEntity *pEntity );

	// Eventos
	virtual void OnSpawnChild( CBaseEntity *pEntity );

	// Estados
	virtual void StartCombat( CBaseEntity *pActivator = NULL, int iWaves = 2, bool bInfinite = false );

	// Jugadores
	virtual void EmitPlayerSound( PlayerSound pSound );

protected:
	int m_iLastPlayerSound[ PL_LAST_SOUND ];
};

extern CAP_Director *Director;

#endif