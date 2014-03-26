//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "c_ap_player_infected.h"

#include "input.h"
#include "iinput.h"

#include "tier0/memdbgon.h"

#ifdef CAP_PlayerInfected
	#undef CAP_PlayerInfected
#endif

//=========================================================
// Comandos
//=========================================================

//=========================================================
// Información y Red
//=========================================================

IMPLEMENT_CLIENTCLASS_DT( C_AP_PlayerInfected, DT_PlayerInfected, CAP_PlayerInfected )
END_RECV_TABLE()

//=========================================================
// Constructor
//=========================================================
C_AP_PlayerInfected::C_AP_PlayerInfected()
{
	::input->CAM_ToThirdPersonShoulder();
}