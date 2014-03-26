//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_SURVIVAL_GAMERULES_H
#define AP_SURVIVAL_GAMERULES_H

#pragma once

#include "ap_gamerules.h"

class CAP_SurvivalGameRules : public CAPGameRules
{
public:
	DECLARE_CLASS( CAP_SurvivalGameRules, CAPGameRules );

	CAP_SurvivalGameRules();

	virtual int GameMode();
	virtual bool IsMultiplayer() { return true; }
};

#endif // AP_SURVIVAL_GAMERULES_H