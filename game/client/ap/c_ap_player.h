//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_AP_PLAYER_H
#define C_AP_PLAYER_H

#pragma once

#include "c_in_player.h"

//====================================================================
// Define un Jugador en Apocalypse
//====================================================================
class C_AP_Player : public C_IN_Player
{
public:
	DECLARE_CLASS( C_AP_Player, C_IN_Player );
	DECLARE_CLIENTCLASS();

	// Equipos
	virtual bool IsInfected() { return ( GetTeamNumber() == TEAM_INFECTED ); }
	virtual bool IsSurvivor() { return ( GetTeamNumber() == TEAM_HUMANS ); }
	virtual bool IsSoldier() { return ( GetTeamNumber() == TEAM_SOLDIERS ); }

	// Utils
	virtual void CreateAnimationState();

	// Renderizado
	virtual void DoPostProcessingEffects( PostProcessParameters_t &params );

	// Sangre, hambre, etc
	virtual float GetBloodLevel() { return m_flBloodLevel; }

	virtual int GetHungryLevel() { return m_iHungryLevel; }
	virtual bool IsHungry() { return ( m_iHungryLevel <= 20 ); }

	virtual int GetThirstLevel() { return m_iThirstLevel; }
	virtual bool IsThirsty() { return ( m_iThirstLevel <= 20 ); }

	// Animaciones
	virtual void UpdatePoseParams();

protected:
	
	float m_flBloodLevel;
	int m_iHungryLevel;
	int m_iThirstLevel;
	int m_iExpPoints;
};

#endif