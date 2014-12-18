//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Responsible for drawing the scene
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "in_view_scene.h"
#include "view_scene.h"
#include "precache_register.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/IMaterialVar.h"
#include "renderparm.h"
#include "c_in_player.h"
#include "functionproxy.h"
#include "imaterialproxydict.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

bool g_bBlurredLastTime = false;

ConVar asw_motionblur( "asw_motionblur", "0", 0, "Motion Blur" );			// motion blur on/off
ConVar asw_motionblur_addalpha("asw_motionblur_addalpha", "0.1", 0, "Motion Blur Alpha");	// The amount of alpha to use when adding the FB to our custom buffer
ConVar asw_motionblur_drawalpha("asw_motionblur_drawalpha", "1", 0, "Motion Blur Draw Alpha");		// The amount of alpha to use when adding our custom buffer to the FB
ConVar asw_motionblur_time("asw_motionblur_time", "0.05", 0, "The amount of time to wait until updating the FB");	// Delay to add between capturing the FB

// @TODO: move this parameter to an entity property rather than convar
ConVar mat_dest_alpha_range( "mat_dest_alpha_range", "1000", 0, "Amount to scale depth values before writing into destination alpha ([0,1] range)." );

PRECACHE_REGISTER_BEGIN( GLOBAL, ASWPrecacheViewRender )
	PRECACHE( MATERIAL, "swarm/effects/frontbuffer" )
	PRECACHE( MATERIAL, "effects/object_motion_blur" )
PRECACHE_REGISTER_END()

static CInViewRender g_ViewRender;

IViewRender *GetViewRenderInstance()
{
	return &g_ViewRender;
}

CInViewRender::CInViewRender()
{
	
}

void CInViewRender::OnRenderStart()
{
	BaseClass::OnRenderStart();

	//CMatRenderContextPtr pRenderContext( materials );
	//pRenderContext->SetFloatRenderingParameter( FLOAT_RENDERPARM_DEST_ALPHA_DEPTH_SCALE, mat_dest_alpha_range.GetFloat() );
}