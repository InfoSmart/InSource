//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_AP_PLAYER_H
#define C_AP_PLAYER_H

#pragma once

#include "c_in_player.h"

class C_AP_Player : public C_IN_Player
{
public:
	DECLARE_CLASS( C_AP_Player, C_IN_Player );
	DECLARE_CLIENTCLASS();

	// Utils
	virtual void CreateAnimationState();
};

#endif