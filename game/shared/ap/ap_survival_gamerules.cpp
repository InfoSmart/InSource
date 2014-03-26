//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ap_survival_gamerules.h"

//=========================================================
// Comandos
//=========================================================

//=========================================================
// Información y Red
//=========================================================

REGISTER_GAMERULES_CLASS( CAP_SurvivalGameRules );

//=========================================================
// Constructor
//=========================================================
CAP_SurvivalGameRules::CAP_SurvivalGameRules() : BaseClass()
{
	// Estas son las reglas del juego.
	InRules = this;
}

//=========================================================
// Devuelve el Modo de Juego
//=========================================================
int CAP_SurvivalGameRules::GameMode()
{
	return GAME_MODE_SURVIVAL;
}