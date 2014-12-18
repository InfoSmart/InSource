//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef IN_FLASHLIGHTEFFECT_H
#define IN_FLASHLIGHTEFFECT_H

#pragma once

//===============================================================================
// Comandos
//===============================================================================

extern ConVar r_flashlightvisualizetrace;
extern ConVar r_flashlightladderdist;

extern ConVar r_flashlightlockposition;
extern ConVar r_flashlightshadowatten;
extern ConVar r_projectedtexture_filter;

extern ConVar mat_slopescaledepthbias_shadowmap;
extern ConVar mat_depthbias_shadowmap;

//===============================================================================
// >> CTraceFilterSkipPlayerAndViewModel
//===============================================================================
class CTraceFilterSkipPlayerAndViewModel : public CTraceFilter
{
public:
	CTraceFilterSkipPlayerAndViewModel( C_BasePlayer *pPlayer, bool bTracePlayers )
	{
		m_pPlayer		= pPlayer;
		m_bSkipPlayers	= !bTracePlayers;
		m_pLowerBody	= NULL;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		C_BaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( !pEntity )
			return true;

		if ( ( ToBaseViewModel( pEntity ) != NULL ) ||
			 ( m_bSkipPlayers && pEntity->IsPlayer() ) ||
			 pEntity == m_pPlayer ||
			 pEntity == m_pLowerBody ||
			 pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS ||
			 pEntity->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		{
			return false;
		}

		return true;
	}

private:
	C_BaseEntity *m_pPlayer;
	C_BaseEntity *m_pLowerBody;

	bool m_bSkipPlayers;
};

//===============================================================================
// Clase base para la creación de una linterna
//===============================================================================
class CBaseFlashlightEffect
{
public:
	DECLARE_CLASS_NOBASE( CBaseFlashlightEffect );

	CBaseFlashlightEffect( int index, const char *pTextureName );
	~CBaseFlashlightEffect();

	virtual bool IsOn() { return m_bIsOn; }

	virtual void TurnOn();
	virtual void TurnOff();

	virtual void Think();
	virtual void Update( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );

	virtual void UpdateFlashlightTexture( const char* pTextureName );
	virtual void UpdateLightProjection( FlashlightState_t &state );

	virtual bool UpdateDefaultFlashlightState(	FlashlightState_t& state, const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp );
	virtual bool ComputeLightPosAndOrientation( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, Vector& vecFinalPos, Quaternion& quatOrientation );

	ClientShadowHandle_t GetFlashlightHandle() { return m_nFlashlightHandle; }
	virtual void SetFlashlightHandle( ClientShadowHandle_t Handle ) { m_nFlashlightHandle = Handle;	}

	// Establecer

	virtual void SetDie( float flValue ) { m_flDie = flValue; }
	virtual void SetFOV( float flValue ) { m_flFOV = flValue; }
	virtual void SetOffset( float flX, float flY, float flZ ) 
	{
		m_flOffsetX = flX;
		m_flOffsetY = flY;
		m_flOffsetZ = flZ;
	}
	virtual void SetNear( float flValue ) { m_flNear = flValue; }
	virtual void SetFar( float flValue ) { m_flFar = flValue; }
	virtual void SetConstant( float flValue ) { m_flConstant = flValue; }
	virtual void SetQuadratic( float flValue ) { m_flQuadratic = flValue; }

	virtual void SetBright( float flValue ) 
	{ 
		m_flBrightness				= flValue; 
		m_flMuzzleFlashBrightness	= flValue;
	}
	virtual void SetAlpha( float flValue ) { m_flAlpha = flValue; }

	virtual void SetMuzzleFlashEnabled( bool bValue, float flBright = 15.0f ) 
	{ 
		m_bMuzzleFlashEnabled		= bValue;
		m_flMuzzleFlashBrightness	= flBright;
	}

protected:
	bool m_bIsOn;
	int m_iEntIndex;

	float m_flCurrentPullBackDist;
	char m_textureName[64];

	ClientShadowHandle_t m_nFlashlightHandle;
	CTextureReference m_nFlashlightTexture;
	CTextureReference m_nMuzzleFlashTexture;

	bool m_bMuzzleFlashEnabled;
	float m_flMuzzleFlashBrightness;

	float m_flFOV;
	float m_flOffsetX;
	float m_flOffsetY;
	float m_flOffsetZ;
	float m_flNear;
	float m_flFar;
	bool  m_bCastsShadows;

	float m_flConstant;
	float m_flQuadratic;

	float m_flBrightness;
	float m_flAlpha;

	float m_flDie;
};

#endif // IN_FLASHLIGHTEFFECT_H