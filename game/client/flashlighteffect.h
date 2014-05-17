//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FLASHLIGHTEFFECT_H
#define FLASHLIGHTEFFECT_H
#ifdef _WIN32
#pragma once
#endif

struct dlight_t;
#include "in_flashlighteffect.h"

//=========================================================
// >> CFlashlightEffect
//=========================================================
class CFlashlightEffect : public CBaseFlashlightEffect
{
public:
	DECLARE_CLASS( CFlashlightEffect, CBaseFlashlightEffect );

	CFlashlightEffect( int nEntIndex = 0, const char *pTextureName = "effects/flashlight001" );
	virtual void Init();
};

//=========================================================
// >> CHeadlightEffect
//=========================================================
class CHeadlightEffect : public CFlashlightEffect
{
public:
	
	CHeadlightEffect();
	~CHeadlightEffect();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance);
};

#endif // FLASHLIGHTEFFECT_H
