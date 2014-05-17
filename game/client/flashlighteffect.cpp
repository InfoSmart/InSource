//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "flashlighteffect.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender.h"
#include "view.h"
#include "engine/ivdebugoverlay.h"
#include "tier0/vprof.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Comandos
//=========================================================

static ConVar r_flashlightoffsetx( "r_flashlightoffsetx", "5.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsety( "r_flashlightoffsety", "-5.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsetz( "r_flashlightoffsetz", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightnear( "r_flashlightnear", "2.0", FCVAR_CHEAT );
static ConVar r_flashlightconstant( "r_flashlightconstant", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightquadratic( "r_flashlightquadratic", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightambient( "r_flashlightambient", "0.0", FCVAR_CHEAT );

//=========================================================
// Constructor
//=========================================================
CFlashlightEffect::CFlashlightEffect( int nEntIndex, const char *pTextureName ) : BaseClass( nEntIndex, pTextureName )
{
}

//=========================================================
// Inicia la configuración recomendada
//=========================================================
void CFlashlightEffect::Init()
{
	SetOffset( r_flashlightoffsetx.GetFloat(), r_flashlightoffsety.GetFloat(), r_flashlightoffsetz.GetFloat() );
	SetNear( r_flashlightnear.GetFloat() );
	SetConstant( r_flashlightconstant.GetFloat() );
	SetQuadratic( r_flashlightquadratic.GetFloat() );
	SetAlpha( r_flashlightambient.GetFloat() );
}

CHeadlightEffect::CHeadlightEffect() 
{

}

CHeadlightEffect::~CHeadlightEffect()
{
	
}

void CHeadlightEffect::UpdateLight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance )
{
	if ( IsOn() == false )
		 return;

	FlashlightState_t state;
	Vector basisX, basisY, basisZ;
	basisX = vecDir;
	basisY = vecRight;
	basisZ = vecUp;
	VectorNormalize(basisX);
	VectorNormalize(basisY);
	VectorNormalize(basisZ);

	BasisToQuaternion( basisX, basisY, basisZ, state.m_quatOrientation );
		
	state.m_vecLightOrigin = vecPos;

	state.m_fHorizontalFOVDegrees	= 45.0f;
	state.m_fVerticalFOVDegrees		= 30.0f;
	state.m_fQuadraticAtten			= r_flashlightquadratic.GetFloat();
	state.m_fLinearAtten			= 100.0f;
	state.m_fConstantAtten			= r_flashlightconstant.GetFloat();

	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 1.0f;
	state.m_Color[2] = 1.0f;
	state.m_Color[3] = r_flashlightambient.GetFloat();

	state.m_NearZ					= r_flashlightnear.GetFloat();
	state.m_FarZ					= 100.0f;
	state.m_bEnableShadows			= true;
	state.m_pSpotlightTexture		= m_nFlashlightTexture;
	state.m_nSpotlightTextureFrame	= 0;
	
	if( GetFlashlightHandle() == CLIENTSHADOW_INVALID_HANDLE )
	{
		SetFlashlightHandle( g_pClientShadowMgr->CreateFlashlight( state ) );
	}
	else
	{
		g_pClientShadowMgr->UpdateFlashlightState( GetFlashlightHandle(), state );
	}
	
	g_pClientShadowMgr->UpdateProjectedTexture( GetFlashlightHandle(), true );
}