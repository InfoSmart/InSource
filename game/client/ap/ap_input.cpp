//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ap_input.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CAPInput g_Input;
IInput *input = ( IInput * )&g_Input;

CAPInput *ApInput()
{
	return &g_Input;
}

//====================================================================
// 
//====================================================================
/*void CAPInput::CAM_Think()
{
	CInput::CAM_Think();
}*/