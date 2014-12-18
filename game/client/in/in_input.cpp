//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_input.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CInInput g_Input;

// Expose this interface
IInput *input = ( IInput * )&g_Input;

CInInput *InInput() 
{ 
	return &g_Input;
}

CInInput::CInInput()
{

}