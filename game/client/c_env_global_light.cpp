//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Sunlight shadow control entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_baseplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_flashlightdepthres;
extern ConVar r_flashlightdepthreshigh;

//====================================================================
// Comandos
//====================================================================

ConVar cl_globallight_freeze( "cl_globallight_freeze", "0" );
ConVar cl_globallight_xoffset( "cl_globallight_xoffset", "0" );
ConVar cl_globallight_yoffset( "cl_globallight_yoffset", "0" );

ConVar mat_shadow_level( "mat_shadow_level", "3", FCVAR_ARCHIVE );

//====================================================================
// Purpose : Sunlights shadow control entity
//====================================================================
class C_GlobalLight : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_GlobalLight, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_GlobalLight();
	~C_GlobalLight();

	virtual void OnDataChanged( DataUpdateType_t updateType );

	virtual void Spawn();
	virtual bool ShouldDraw();

	virtual void ClientThink();
	virtual FlashlightState_t PrepareLight();

private:
	int m_iLightID;

	Vector m_shadowDirection;
	bool m_bEnabled;
	char m_TextureName[ MAX_PATH ];
	CTextureReference m_SpotlightTexture;
	color32	m_LightColor;
	Vector m_CurrentLinearFloatLightColor;
	float m_flCurrentLinearFloatLightAlpha;
	float m_flColorTransitionTime;
	float m_flSunDistance;
	float m_flFOV;
	float m_flNearZ;
	float m_flNorthOffset;
	bool m_bEnableShadows;
	bool m_bOldEnableShadows;

	ClientShadowHandle_t m_nLightHandle;
};

//====================================================================
// Información y Red
//====================================================================

IMPLEMENT_CLIENTCLASS_DT( C_GlobalLight, DT_GlobalLight, CGlobalLight )
	RecvPropInt( RECVINFO(m_iLightID) ),
	RecvPropVector(RECVINFO(m_shadowDirection)),
	RecvPropBool(RECVINFO(m_bEnabled)),
	RecvPropString(RECVINFO(m_TextureName)),
	RecvPropInt(RECVINFO(m_LightColor), 0, RecvProxy_Int32ToColor32),
	RecvPropFloat(RECVINFO(m_flColorTransitionTime)),
	RecvPropFloat(RECVINFO(m_flSunDistance)),
	RecvPropFloat(RECVINFO(m_flFOV)),
	RecvPropFloat(RECVINFO(m_flNearZ)),
	RecvPropFloat(RECVINFO(m_flNorthOffset)),
	RecvPropBool(RECVINFO(m_bEnableShadows)),
END_RECV_TABLE()

//====================================================================
// Constructor
//====================================================================
C_GlobalLight::C_GlobalLight()
{
	m_nLightHandle = CLIENTSHADOW_INVALID_HANDLE;
}

//====================================================================
// Destructor
//====================================================================
C_GlobalLight::~C_GlobalLight()
{
	// Debemos apagar y destruir la luz
	if ( m_nLightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_nLightHandle );
		m_nLightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

//====================================================================
//====================================================================
void C_GlobalLight::OnDataChanged( DataUpdateType_t updateType )
{
	// Iniciamos la textura a proyectar
	if ( updateType == DATA_UPDATE_CREATED )
		m_SpotlightTexture.Init( m_TextureName, TEXTURE_GROUP_OTHER, true );

	BaseClass::OnDataChanged( updateType );
}

//====================================================================
// Creación
//====================================================================
void C_GlobalLight::Spawn()
{
	BaseClass::Spawn();

	m_bOldEnableShadows = m_bEnableShadows;

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//====================================================================
// We don't draw...
//====================================================================
bool C_GlobalLight::ShouldDraw()
{
	return false;
}

//====================================================================
//====================================================================
void C_GlobalLight::ClientThink()
{
	VPROF("C_GlobalLight::ClientThink");
	bool bSupressWorldLights = false;

	// Congelado
	// TODO: ¿Quitar esta opción en el cliente?
	if ( cl_globallight_freeze.GetBool() )
		return;

	if ( m_bEnabled )
	{
		Vector vLinearFloatLightColor( m_LightColor.r, m_LightColor.g, m_LightColor.b );
		float flLinearFloatLightAlpha = m_LightColor.a;

		if ( m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha )
		{
			float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

			m_CurrentLinearFloatLightColor.x = Approach( vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed );
			m_CurrentLinearFloatLightColor.y = Approach( vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed );
			m_CurrentLinearFloatLightColor.z = Approach( vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed );
			m_flCurrentLinearFloatLightAlpha = Approach( flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed );
		}

		// Calculamos la dirección de las sombras y la ubicación de la luz
		Vector vDirection = m_shadowDirection;
		VectorNormalize( vDirection );

		Vector vSunDirection2D	= vDirection;
		vSunDirection2D.z		= 0.0f;

		HACK_GETLOCALPLAYER_GUARD( "C_GlobalLight::ClientThink" );

		if ( !C_BasePlayer::GetLocalPlayer() )
			return;

		Vector vPos;
		QAngle EyeAngles;
		float flZNear, flZFar, flFov;

		C_BasePlayer::GetLocalPlayer()->CalcView( vPos, EyeAngles, flZNear, flZFar, flFov );
		vPos = ( vPos + vSunDirection2D * m_flNorthOffset ) - vDirection * m_flSunDistance;
		vPos += Vector( cl_globallight_xoffset.GetFloat(), cl_globallight_yoffset.GetFloat(), 0.0f );

		QAngle angAngles;
		VectorAngles( vDirection, angAngles );

		Vector vForward, vRight, vUp;
		AngleVectors( angAngles, &vForward, &vRight, &vUp );

		// Configuramos la luz
		FlashlightState_t state = PrepareLight();
		state.m_vecLightOrigin	= vPos;

		//
		BasisToQuaternion( vForward, vRight, vUp, state.m_quatOrientation );

		// Se ha desactivado o activado las sombras
		if ( m_bOldEnableShadows != m_bEnableShadows )
		{
			// Debemos recrear la luz actual
			if ( m_nLightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				g_pClientShadowMgr->DestroyFlashlight( m_nLightHandle );
				m_nLightHandle = CLIENTSHADOW_INVALID_HANDLE;
			}

			m_bOldEnableShadows = m_bEnableShadows;
		}

		// Debemos crear la luz
		if ( m_nLightHandle == CLIENTSHADOW_INVALID_HANDLE )
		{
			m_nLightHandle = g_pClientShadowMgr->CreateFlashlight( state );
		}
		else
		{
			// Ya esta creada, la actualizamos
			g_pClientShadowMgr->UpdateFlashlightState( m_nLightHandle, state );
			g_pClientShadowMgr->UpdateProjectedTexture( m_nLightHandle, true );
		}

		bSupressWorldLights = m_bEnableShadows;
	}
	else if ( m_nLightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_nLightHandle );
		m_nLightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}

	g_pClientShadowMgr->SetShadowFromWorldLightsEnabled( !bSupressWorldLights );
	BaseClass::ClientThink();
}

//====================================================================
// Devuelve la configuración para la luz
//====================================================================
FlashlightState_t C_GlobalLight::PrepareLight()
{
	ConVarRef r_flashlightshadowatten("r_flashlightshadowatten");
	ConVarRef r_projectedtexture_filter("r_projectedtexture_filter");

	FlashlightState_t state;

	state.m_fQuadraticAtten			= 0.0f;
	state.m_fLinearAtten			= m_flSunDistance * 2.0f;
	state.m_fConstantAtten			= 0.0f;
	state.m_FarZAtten				= m_flSunDistance * 2.0f;
	state.m_fHorizontalFOVDegrees	= 5.0f;
	state.m_fVerticalFOVDegrees		= 5.0f;

	// Color de la luz
	state.m_Color[0]	= m_CurrentLinearFloatLightColor.x * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
	state.m_Color[1]	= m_CurrentLinearFloatLightColor.y * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
	state.m_Color[2]	= m_CurrentLinearFloatLightColor.z * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
	state.m_Color[3]	= 0.0f;

	// Distancia y brillo
	state.m_NearZ				= 4.0f;
	state.m_FarZ				= m_flSunDistance * 2.0f;
	state.m_fBrightnessScale	= 2.0f;
	state.m_bGlobalLight		= true;

	// Ortho Light
	state.m_bOrtho			= true;
	state.m_fOrthoLeft		= -m_flFOV;
	state.m_fOrthoTop		= -m_flFOV;
	state.m_fOrthoRight		= m_flFOV;
	state.m_fOrthoBottom	= m_flFOV;

	//
	state.m_pSpotlightTexture		= m_SpotlightTexture;
	state.m_pProjectedMaterial		= NULL;
	state.m_nSpotlightTextureFrame	= 0;

	// Propiedades de las sombras generadas
	state.m_bEnableShadows				= m_bEnableShadows;
	state.m_flShadowAtten				= r_flashlightshadowatten.GetFloat();	
	state.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();
	state.m_flShadowDepthBias			= g_pMaterialSystemHardwareConfig->GetShadowDepthBias();
	
	state.m_bShadowHighRes				= true;
	state.m_nShadowQuality				= 1;


	//
	// mat_shadow_level 2:
	// 1 & 2: 2K
	// 3+: 1K
	//
	// mat_shadow_level 1:
	// 1+: 1K
	// 3+: Off
	//
	// mat_shadow_level 0
	// 1+: 1K and Low
	// 3+: Off
	//

	{
		// Es la primera luz, debe tener mayor calidad en las sombras
		if ( m_iLightID == 1 )
		{
			state.m_flShadowFilterSize = r_projectedtexture_filter.GetFloat();
		}
		else
		{
			state.m_flShadowFilterSize = ( r_projectedtexture_filter.GetFloat() + (0.1f*m_iLightID) );
		}

		// mat_shadow_level 1 = 2+ luz en 1K
		// mat_shadow_level 0 = Todas en 1K
		// 3+ = 1K
		if ( mat_shadow_level.GetInt() <= 1 && m_iLightID >= 2 || mat_shadow_level.GetInt() <= 0 || m_iLightID >= 3 )
		{
			state.m_bShadowHighRes	= false;
			state.m_nShadowQuality	= 0;
		}

		// mat_shadow_level 1 = 3+ luz sin sombras
		// La cuarta luz y las siguientes no tendrán sombras
		// Iván: Por ahora queremos Juegos de baja gama
		if ( mat_shadow_level.GetInt() <= 1 && m_iLightID >= 3 || m_iLightID >= 4 )
		{
			state.m_bEnableShadows = false;
		}	
	}

	return state;
}