//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_AP_PLAYER_INFECTED_H
#define C_AP_PLAYER_INFECTED_H

#pragma once

#include "c_ap_player.h"

//=========================================================
// >> C_IN_Player
//=========================================================
class C_AP_PlayerInfected : public C_AP_Player
{
public:
	DECLARE_CLASS( C_AP_PlayerInfected, C_AP_Player );
	DECLARE_CLIENTCLASS();

	// Principales
	//C_AP_PlayerInfected();
	virtual void Spawn();
};

#endif  // C_AP_PLAYER_INFECTED_H