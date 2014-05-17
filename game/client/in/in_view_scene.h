//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IN_VIEW_SCENE_H
#define IN_VIEW_SCENE_H

#ifdef _WIN32
#pragma once
#endif

#include "viewrender.h"

//-----------------------------------------------------------------------------
// Purpose: Implements the interview to view rendering for the client .dll
//-----------------------------------------------------------------------------
class CInViewRender : public CViewRender
{
public:
	CInViewRender();

	virtual void OnRenderStart();
	virtual void Render2DEffectsPreHUD( const CViewSetup &view );

	virtual bool AllowScreenspaceFade() { return false; }

private:
	void DoMotionBlur( const CViewSetup &view );
};

#endif //ASW_VIEW_SCENE_H