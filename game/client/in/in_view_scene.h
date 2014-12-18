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
	DECLARE_CLASS( CInViewRender, CViewRender );
	CInViewRender();

	virtual void OnRenderStart();
	virtual bool AllowScreenspaceFade() { return true; }
};

#endif //ASW_VIEW_SCENE_H