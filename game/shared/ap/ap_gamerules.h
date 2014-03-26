//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_GAMERULES_H
#define AP_GAMERULES_H

#include "in_gamerules.h"

#pragma once

//=========================================================
// >> CAPGameRules
//=========================================================
class CAPGameRules : public CInGameRules
{
public:
	DECLARE_CLASS( CAPGameRules, CInGameRules );

	// ¿Es una partida multijugador?
	virtual bool IsMultiplayer() 
	{ 
		return true; 
	}

	// Modo de creación
	virtual int SpawnMode() 
	{ 
		return SPAWN_MODE_UNIQUE; 
	}

	//
	virtual CAPGameRules *ApPointer() 
	{ 
		return dynamic_cast<CAPGameRules *>( this ); 
	}

	// Principales
	CAPGameRules();

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();
#else
	DECLARE_SERVERCLASS_NOBASE();

	// Jugadores
	virtual void ConvertToInfected( CBasePlayer *pPlayer );
	virtual void ConvertToHuman( CBasePlayer *pPlayer );

#endif
};

#endif