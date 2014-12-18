//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "in_flashlighteffect.h"

#include "dlight.h"
#include "iefx.h"

#include "iviewrender.h"
#include "view.h"

#include "engine/ivdebugoverlay.h"

//#include "deferred/deferred_shared_common.h"

#include "tier0/vprof.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//===============================================================================
// Comandos
//===============================================================================

extern ConVar r_flashlightdepthres;
extern ConVar r_flashlightdepthtexture;

ConVar r_projectedtexture_filter("r_projectedtexture_filter", "0.5", FCVAR_ARCHIVE);

ConVar r_flashlightvisualizetrace( "r_flashlightvisualizetrace", "0", FCVAR_CHEAT );
ConVar r_flashlightladderdist( "r_flashlightladderdist", "40.0", FCVAR_CHEAT );

ConVar r_flashlightlockposition( "r_flashlightlockposition", "0", FCVAR_CHEAT );
ConVar r_flashlightshadowatten( "r_flashlightshadowatten", "0.35", FCVAR_CHEAT );

ConVar r_flashlightmuzzleflashfov( "r_flashlightmuzzleflashfov", "120", FCVAR_CHEAT );

ConVar r_flashlightnearoffsetscale( "r_flashlightnearoffsetscale", "1.0", FCVAR_CHEAT );
ConVar r_flashlighttracedistcutoff( "r_flashlighttracedistcutoff", "128" );
ConVar r_flashlightbacktraceoffset( "r_flashlightbacktraceoffset", "0.4", FCVAR_CHEAT );

//==============================================================================
// Constructor
//==============================================================================
CBaseFlashlightEffect::CBaseFlashlightEffect( int index, const char *pTextureName )
{
	m_nFlashlightHandle		= CLIENTSHADOW_INVALID_HANDLE;
	m_iEntIndex				= index;

	m_bIsOn					= false;
	m_flDie					= 0;
	m_flCurrentPullBackDist = 1.0f;
	m_bCastsShadows			= true;

	m_bMuzzleFlashEnabled		= false;
	m_flMuzzleFlashBrightness	= 100.0f;

	// Iniciamos la textura de la luz
	UpdateFlashlightTexture( pTextureName );
}

//===============================================================================
// Destructor
//===============================================================================
CBaseFlashlightEffect::~CBaseFlashlightEffect()
{
	// Apagamos la luz
	TurnOff();
}

//===============================================================================
// Enciende la linterna
//===============================================================================
void CBaseFlashlightEffect::TurnOn()
{
	m_bIsOn					= true;
	m_flCurrentPullBackDist = 1.0f;
}

//===============================================================================
// Apaga la linterna
//===============================================================================
void CBaseFlashlightEffect::TurnOff()
{
	// No esta encendida
	if ( !m_bIsOn )
		return;

	m_bIsOn = false;

	// Clear out the light
	if ( m_nFlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_nFlashlightHandle );
		m_nFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}

#ifndef NO_TOOLFRAMEWORK
	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "FlashlightState" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetInt( "entindex", m_iEntIndex );
		msg->SetInt( "flashlightHandle", m_nFlashlightHandle );
		msg->SetPtr( "flashlightState", NULL );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
#endif
}

//===============================================================================
// Pensamiento
//===============================================================================
void CBaseFlashlightEffect::Think()
{
	if ( m_flDie > 0 )
	{
		if ( gpGlobals->curtime > m_flDie )
		{
			TurnOff();
			m_flDie = 0;
		}
	}
}

//===============================================================================
// Actualiza la ubicación de la luz
//===============================================================================
void CBaseFlashlightEffect::Update( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	// No esta encendida
	if ( !IsOn() )
		return;

	FlashlightState_t state;

	if ( UpdateDefaultFlashlightState( state, vecPos, vecForward, vecRight, vecUp ) == false )
		return;

	UpdateLightProjection( state );

#ifndef NO_TOOLFRAMEWORK
	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "FlashlightState" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetInt( "entindex", m_iEntIndex );
		msg->SetInt( "flashlightHandle", m_nFlashlightHandle );
		msg->SetPtr( "flashlightState", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
#endif
}

//===============================================================================
// Actualiza la textura de la luz
//===============================================================================
void CBaseFlashlightEffect::UpdateFlashlightTexture( const char* pTextureName )
{
	static const char *pEmptyString = "";

	if ( pTextureName == NULL )
		pTextureName = pEmptyString;

	if ( !m_nFlashlightTexture.IsValid() || V_stricmp( m_textureName, pTextureName ) != 0 )
	{
		if ( pTextureName == pEmptyString )
		{
			m_nFlashlightTexture.Init( "effects/flashlight001", TEXTURE_GROUP_OTHER, true );
		}
		else
		{
			m_nFlashlightTexture.Init( pTextureName, TEXTURE_GROUP_OTHER, true );
		}

		V_strncpy( m_textureName, pTextureName, sizeof(m_textureName) );
	}

	// Iniciamos la textura del Flash
	if ( !m_nMuzzleFlashTexture.IsValid() )
		m_nMuzzleFlashTexture.Init( "effects/muzzleflash_light", TEXTURE_GROUP_OTHER, true );
}

//===============================================================================
//===============================================================================
void CBaseFlashlightEffect::UpdateLightProjection( FlashlightState_t &state )
{
	if ( m_nFlashlightHandle == CLIENTSHADOW_INVALID_HANDLE )
	{
		m_nFlashlightHandle = g_pClientShadowMgr->CreateFlashlight( state );
	}
	else
	{
		if ( !r_flashlightlockposition.GetBool() )
		{
			g_pClientShadowMgr->UpdateFlashlightState( m_nFlashlightHandle, state );
		}
	}

	g_pClientShadowMgr->UpdateProjectedTexture( m_nFlashlightHandle, true );
}

//===============================================================================
//===============================================================================
bool CBaseFlashlightEffect::UpdateDefaultFlashlightState( FlashlightState_t& state, const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
	VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	// No esta encendida
	if ( !IsOn() )
		return false;

	if ( ComputeLightPosAndOrientation( vecPos, vecForward, vecRight, vecUp, state.m_vecLightOrigin, state.m_quatOrientation ) == false )
		return false;

	state.m_fQuadraticAtten		= m_flQuadratic;
	state.m_fConstantAtten		= m_flConstant;

	// Color de la luz
	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 1.0f;
	state.m_Color[2] = 1.0f;
	state.m_Color[3] = m_flAlpha;

	// Distancia y FOV
	state.m_NearZ					= m_flNear + r_flashlightnearoffsetscale.GetFloat() * m_flCurrentPullBackDist;
	state.m_FarZ					= state.m_FarZAtten = m_flFar;
	state.m_fHorizontalFOVDegrees	= state.m_fVerticalFOVDegrees = m_flFOV;

	// Es el efecto de un "MuzzleFlash"
	if ( m_bMuzzleFlashEnabled )
	{
		state.m_pSpotlightTexture		= m_nMuzzleFlashTexture;
		state.m_fLinearAtten			= m_flMuzzleFlashBrightness;

		state.m_bShadowHighRes			= false;
		state.m_nShadowQuality			= 0;
		state.m_flShadowFilterSize		= 2.0f;
	}

	// Es una linterna normal
	else
	{
		state.m_pSpotlightTexture		= m_nFlashlightTexture;
		state.m_fLinearAtten			= m_flBrightness;

		state.m_bShadowHighRes			= true;
		state.m_nShadowQuality			= 1;
		state.m_flShadowFilterSize		= r_projectedtexture_filter.GetFloat();
	}

	state.m_pProjectedMaterial			= NULL;
	state.m_nSpotlightTextureFrame		= 0;

	// Propiedades de las sombras generadas
	state.m_bEnableShadows				= m_bCastsShadows;
	state.m_flShadowAtten				= r_flashlightshadowatten.GetFloat();
	state.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();
	state.m_flShadowDepthBias			= g_pMaterialSystemHardwareConfig->GetShadowDepthBias();

	return true;
}

//===============================================================================
//===============================================================================
bool CBaseFlashlightEffect::ComputeLightPosAndOrientation( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, Vector& vecFinalPos, Quaternion& quatOrientation )
{
	vecFinalPos = vecPos;
	BasisToQuaternion( vecForward, vecRight, vecUp, quatOrientation );
	return true;

	const float flEpsilon	= 0.1f;			// Offset flashlight position along vecUp
	float flDistCutoff		= r_flashlighttracedistcutoff.GetFloat();
	const float flDistDrag	= 0.2;
	bool bDebugVis			= r_flashlightvisualizetrace.GetBool();

	C_BasePlayer *pPlayer	= UTIL_PlayerByIndex( m_iEntIndex );

	if ( !pPlayer )
	{
		pPlayer = C_BasePlayer::GetLocalPlayer();

		if ( !pPlayer )
			return false;
	}

	// We will lock some of the flashlight params if player is on a ladder, to prevent oscillations due to the trace-rays
	bool bPlayerOnLadder = ( pPlayer->GetMoveType() == MOVETYPE_LADDER );

	CTraceFilterSkipPlayerAndViewModel traceFilter( pPlayer, true );

	//	Vector vOrigin = vecPos + r_flashlightoffsety.GetFloat() * vecUp;
	Vector vecOffset;
	pPlayer->GetFlashlightOffset( vecForward, vecRight, vecUp, &vecOffset );
	Vector vOrigin = vecPos + vecOffset;

	// Not on ladder...trace a hull
	if ( !bPlayerOnLadder ) 
	{
		Vector vecPlayerEyePos = pPlayer->GetRenderOrigin() + pPlayer->GetViewOffset();

		trace_t pmOriginTrace;
		UTIL_TraceHull( vecPlayerEyePos, vOrigin, Vector(-2, -2, -2), Vector(2, 2, 2), ( MASK_SOLID & ~(CONTENTS_HITBOX) ) | CONTENTS_WINDOW | CONTENTS_GRATE, &traceFilter, &pmOriginTrace );//1

		if ( bDebugVis )
		{
			debugoverlay->AddBoxOverlay( pmOriginTrace.endpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 0, 255, 0, 16, 0 );
			if ( pmOriginTrace.DidHit() || pmOriginTrace.startsolid )
			{
				debugoverlay->AddLineOverlay( pmOriginTrace.startpos, pmOriginTrace.endpos, 255, 128, 128, true, 0 );
			}
			else
			{
				debugoverlay->AddLineOverlay( pmOriginTrace.startpos, pmOriginTrace.endpos, 255, 0, 0, true, 0 );
			}
		}

		if ( pmOriginTrace.DidHit() || pmOriginTrace.startsolid )
		{
			vOrigin = pmOriginTrace.endpos;
		}
		else
		{
			if ( pPlayer->m_vecFlashlightOrigin != vecPlayerEyePos )
			{
				vOrigin = vecPos;
			}
		}
	}
	else // on ladder...skip the above hull trace
	{
		vOrigin = vecPos;
	}

	// Now do a trace along the flashlight direction to ensure there is nothing within range to pull back from
	int iMask = MASK_OPAQUE_AND_NPCS;
	iMask &= ~CONTENTS_HITBOX;
	iMask |= CONTENTS_WINDOW | CONTENTS_GRATE | CONTENTS_IGNORE_NODRAW_OPAQUE;

	Vector vTarget = vOrigin + vecForward * m_flFar;

	// Work with these local copies of the basis for the rest of the function
	Vector vDir   = vTarget - vOrigin;
	Vector vRight = vecRight;
	Vector vUp    = vecUp;
	VectorNormalize( vDir   );
	VectorNormalize( vRight );
	VectorNormalize( vUp    );

	// Orthonormalize the basis, since the flashlight texture projection will require this later...
	vUp -= DotProduct( vDir, vUp ) * vDir;
	VectorNormalize( vUp );
	vRight -= DotProduct( vDir, vRight ) * vDir;
	VectorNormalize( vRight );
	vRight -= DotProduct( vUp, vRight ) * vUp;
	VectorNormalize( vRight );

	AssertFloatEquals( DotProduct( vDir, vRight ), 0.0f, 1e-3 );
	AssertFloatEquals( DotProduct( vDir, vUp    ), 0.0f, 1e-3 );
	AssertFloatEquals( DotProduct( vRight, vUp  ), 0.0f, 1e-3 );

	trace_t pmDirectionTrace;
	UTIL_TraceHull( vOrigin, vTarget, Vector( -1.5, -1.5, -1.5 ), Vector( 1.5, 1.5, 1.5 ), iMask, &traceFilter, &pmDirectionTrace );//.5

	if ( bDebugVis )
	{
		debugoverlay->AddBoxOverlay( pmDirectionTrace.endpos, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), QAngle( 0, 0, 0 ), 0, 0, 255, 16, 0 );
		debugoverlay->AddLineOverlay( vOrigin, pmDirectionTrace.endpos, 255, 0, 0, false, 0 );
	}

	float flTargetPullBackDist = 0.0f;
	float flDist = (pmDirectionTrace.endpos - vOrigin).Length();

	if ( flDist < flDistCutoff )
	{
		// We have an intersection with our cutoff range
		// Determine how far to pull back, then trace to see if we are clear
		float flPullBackDist = bPlayerOnLadder ? r_flashlightladderdist.GetFloat() : flDistCutoff - flDist;	// Fixed pull-back distance if on ladder

		flTargetPullBackDist = flPullBackDist;

		if ( !bPlayerOnLadder )
		{
			trace_t pmBackTrace;
			// start the trace away from the actual trace origin a bit, to avoid getting stuck on small, close "lips"
			UTIL_TraceHull( vOrigin - vDir * ( flDistCutoff * r_flashlightbacktraceoffset.GetFloat() ), vOrigin - vDir * ( flPullBackDist - flEpsilon ),
				Vector( -1.5f, -1.5f, -1.5f ), Vector( 1.5f, 1.5f, 1.5f ), iMask, &traceFilter, &pmBackTrace );

			if ( bDebugVis )
			{
				debugoverlay->AddLineOverlay( pmBackTrace.startpos, pmBackTrace.endpos, 255, 0, 255, true, 0 );
			}

			if( pmBackTrace.DidHit() )
			{
				// We have an intersection behind us as well, so limit our flTargetPullBackDist
				float flMaxDist = (pmBackTrace.endpos - vOrigin).Length() - flEpsilon;
				flTargetPullBackDist = MIN( flMaxDist, flTargetPullBackDist );
				//m_flCurrentPullBackDist = MIN( flMaxDist, m_flCurrentPullBackDist );	// possible pop
			}
		}
	}

	if ( bDebugVis )
	{
		// visualize pullback
		debugoverlay->AddBoxOverlay( vOrigin - vDir * m_flCurrentPullBackDist, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 255, 255, 0, 16, 0 );
		debugoverlay->AddBoxOverlay( vOrigin - vDir * flTargetPullBackDist, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), QAngle( 0, 0, 0 ), 128, 128, 0, 16, 0 );
	}

	m_flCurrentPullBackDist = Lerp( flDistDrag, m_flCurrentPullBackDist, flTargetPullBackDist );
	m_flCurrentPullBackDist = MIN( m_flCurrentPullBackDist, flDistCutoff );	// clamp to max pullback dist
	vOrigin = vOrigin - vDir * m_flCurrentPullBackDist;

	vecFinalPos = vOrigin;
	BasisToQuaternion( vDir, vRight, vUp, quatOrientation );

	return true;
}
