#ifndef _INCLUDED_SDK_INPUT_H
#define _INCLUDED_SDK_INPUT_H

#ifdef _WIN32
#pragma once
#endif

#include "input.h"

//====================================================================
// Interfaz para la entrada del Jugador
//====================================================================
class CInInput : public CInput
{
public:
	CInInput();
};

extern CInInput *InInput();

#endif // _INCLUDED_SDK_INPUT_H