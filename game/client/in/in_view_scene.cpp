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
	CViewRender::OnRenderStart();
	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->SetFloatRenderingParameter( FLOAT_RENDERPARM_DEST_ALPHA_DEPTH_SCALE, mat_dest_alpha_range.GetFloat() );
}

//-----------------------------------------------------------------------------
// Purpose: Renders extra 2D effects in derived classes while the 2D view is on the stack
//-----------------------------------------------------------------------------
void CInViewRender::Render2DEffectsPreHUD( const CViewSetup &view )
{
#ifndef _X360
	// @TODO: Motion blur not supported on X360 yet due to EDRAM issues
	DoMotionBlur( view );
#endif
}


void CInViewRender::DoMotionBlur( const CViewSetup &view )
{
	/*
	if ( asw_motionblur.GetInt() == 0 )
	{
		g_bBlurredLastTime = false;
		return;
	}

	static float fNextDrawTime = 0.0f;

	bool found;
	IMaterialVar* mv = NULL;
	IMaterial *pMatScreen = NULL;
	//ITexture *pMotionBlur = NULL;
	ITexture *pOriginalTexture = NULL; 

	// Get the front buffer material
	pMatScreen = materials->FindMaterial( "swarm/effects/frontbuffer", TEXTURE_GROUP_OTHER, true );
	// Get our custom render target
	//pMotionBlur = g_pASWRenderTargets->GetASWMotionBlurTexture();
	// Store the current render target
	CMatRenderContextPtr pRenderContext( materials );
	ITexture *pOriginalRenderTarget = pRenderContext->GetRenderTarget();

	// Set the camera up so we can draw the overlay
	int oldX, oldY, oldW, oldH;
	pRenderContext->GetViewport( oldX, oldY, oldW, oldH );

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	// set our blur parameters, based on convars or the poison duration
	float add_alpha = asw_motionblur_addalpha.GetFloat();
	float blur_time = asw_motionblur_time.GetFloat();
	float draw_alpha = asw_motionblur_drawalpha.GetFloat();
	
	if (!g_bBlurredLastTime)
		add_alpha = 1.0f;	// add the whole buffer if this is the first time we're blurring after a while, so we don't end up with images from ages ago

	if ( fNextDrawTime - gpGlobals->curtime > 1.0f)
	{
		fNextDrawTime = 0.0f;
	}

	if( gpGlobals->curtime >= fNextDrawTime ) 
	{
		UpdateScreenEffectTexture( 0, view.x, view.y, view.width, view.height );

		// Set the alpha to whatever our console variable is
		mv = pMatScreen->FindVar( "$alpha", &found, false );
		if (found)
		{
			if ( fNextDrawTime == 0 )
			{
				mv->SetFloatValue( 1.0f );
			}
			else
			{
				mv->SetFloatValue( add_alpha );
			}
		}

		pRenderContext->SetRenderTarget( pMotionBlur );
		pRenderContext->DrawScreenSpaceQuad( pMatScreen );

		// Set the next draw time according to the convar
		fNextDrawTime = gpGlobals->curtime + blur_time;
	}

	// Set the alpha
	mv = pMatScreen->FindVar( "$alpha", &found, false );
	if (found)
	{
		mv->SetFloatValue( draw_alpha );
	}

	// Set the texture to our buffer
	mv = pMatScreen->FindVar( "$basetexture", &found, false );
	if (found)
	{
		pOriginalTexture = mv->GetTextureValue();
		mv->SetTextureValue( pMotionBlur );
	}

	// Pretend we were never here, set everything back
	pRenderContext->SetRenderTarget( pOriginalRenderTarget );
	pRenderContext->DrawScreenSpaceQuad( pMatScreen );

	// Set our texture back to _rt_FullFrameFB
	if (found)
	{
		mv->SetTextureValue( pOriginalTexture );
	}

	pRenderContext->DepthRange( 0.0f, 1.0f );
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	g_bBlurredLastTime = true;
	*/
}