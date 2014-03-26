//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef AP_DIRECTOR_H
#define AP_DIRECTOR_H

#pragma once

#include "in_shareddefs.h"
#include "director.h"

//=========================================================
// >> CAP_Director
//=========================================================
class CAP_Director : public CDirector
{
public:
	DECLARE_CLASS( CAP_Director, CDirector );

	// Utilidades
	virtual float MaxDistance();

	// Calculos/Verificación
	virtual bool IsMode( int iStatus );

	virtual void OnAngryMusicPlay();

	// Hijos
	virtual int MaxChilds();
	virtual bool CheckNewEnemy( CBaseEntity *pEntity );

	// Estados
	virtual void Panic( CBasePlayer *pCaller = NULL, int iSeconds = 0, bool bInfinite = false );

protected:
	Vector m_vecPanicPosition;
};

extern CAP_Director *Director;

#endif