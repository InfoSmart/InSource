//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef IN_FLASHLIGHTEFFECT_H
#define IN_FLASHLIGHTEFFECT_H

#pragma once

//=========================================================
// Comandos
//=========================================================

extern ConVar r_flashlightvisualizetrace;
extern ConVar r_flashlightladderdist;

extern ConVar r_flashlightlockposition;
extern ConVar r_flashlightshadowatten;

//extern ConVar mat_slopescaledepthbias_shadowmap;
//extern ConVar mat_depthbias_shadowmap;

//=========================================================
// >> CTraceFilterSkipPlayerAndViewModel
//=========================================================
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

//=========================================================
// >> CBaseFlashlightEffect
//=========================================================
class CBaseFlashlightEffect
{
public:
	CBaseFlashlightEffect( int iIndex, const char *pTextureName );
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

//=========================================================
// >> CFlashlightEffectManager
//=========================================================
class CFlashlightEffectManager
{
public:
	CFlashlightEffectManager() : m_pFlashlightEffect( NULL ), m_pFlashlightTextureName( NULL ), m_nFlashlightEntIndex( -1 ), m_flFov( 0.0f ),
								m_flFarZ( 0.0f ), m_flLinearAtten( 0.0f ), m_nMuzzleFlashFrameCountdown( 0 ), m_flMuzzleFlashBrightness( 1.0f ),
								m_bFlashlightOn( false ), m_nFXComputeFrame( -1 ), m_bFlashlightOverride( false ) {}

	void TurnOnFlashlight( int nEntIndex = 0, const char *pszTextureName = NULL, float flFov = 0.0f, float flFarZ = 0.0f, float flLinearAtten = 0.0f );
	void TurnOffFlashlight( bool bForce = false );

	bool IsFlashlightOn() const { return m_bFlashlightOn; }

	void UpdateFlashlight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, float flFov, bool castsShadows,
		float flFarZ, float flLinearAtten, const char* pTextureName = NULL )
	{
		if ( m_bFlashlightOverride )
		{
			// don't mess with it while it's overridden
			return;
		}

		bool bMuzzleFlashActive = ( m_nMuzzleFlashFrameCountdown > 0 ) || !m_muzzleFlashTimer.IsElapsed();

		if ( m_pFlashlightEffect )
		{
			m_flFov = flFov;
			m_flFarZ = flFarZ;
			m_flLinearAtten = flLinearAtten;
			m_pFlashlightEffect->Update( vecPos, vecDir, vecRight, vecUp );
			m_pFlashlightEffect->SetMuzzleFlashEnabled( bMuzzleFlashActive, m_flMuzzleFlashBrightness );
		}

		if ( !bMuzzleFlashActive && !m_bFlashlightOn && m_pFlashlightEffect )
		{
			delete m_pFlashlightEffect;
			m_pFlashlightEffect = NULL;
		}

		if ( bMuzzleFlashActive && !m_bFlashlightOn && !m_pFlashlightEffect )
		{
			m_pFlashlightEffect = new CBaseFlashlightEffect( m_nFlashlightEntIndex, NULL );
			m_pFlashlightEffect->SetMuzzleFlashEnabled( bMuzzleFlashActive, m_flMuzzleFlashBrightness );
		}

		if ( bMuzzleFlashActive && m_nFXComputeFrame != gpGlobals->framecount )
		{
			m_nFXComputeFrame = gpGlobals->framecount;
			m_nMuzzleFlashFrameCountdown--;
		}
	}

	void SetEntityIndex( int index )
	{
		m_nFlashlightEntIndex = index;
	}

	void TriggerMuzzleFlash()
	{
		m_nMuzzleFlashFrameCountdown = 2;
		m_muzzleFlashTimer.Start( 0.066f );		// show muzzleflash for 2 frames or 66ms, whichever is longer
		m_flMuzzleFlashBrightness = random->RandomFloat( 0.4f, 2.0f );
	}

	const char *GetFlashlightTextureName( void ) const
	{
		return m_pFlashlightTextureName;
	}

	int GetFlashlightEntIndex( void ) const
	{
		return m_nFlashlightEntIndex;
	}

	void EnableFlashlightOverride( bool bEnable )
	{
		m_bFlashlightOverride = bEnable;

		if ( !m_bFlashlightOverride )
		{
			// make sure flashlight is in its original state
			if ( m_bFlashlightOn && m_pFlashlightEffect == NULL )
			{
				TurnOnFlashlight( m_nFlashlightEntIndex, m_pFlashlightTextureName, m_flFov, m_flFarZ, m_flLinearAtten );
			}
			else if ( !m_bFlashlightOn && m_pFlashlightEffect )
			{
				delete m_pFlashlightEffect;
				m_pFlashlightEffect = NULL;
			}
		}
	}

	void UpdateFlashlightOverride(	bool bFlashlightOn, const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp,
									float flFov, bool castsShadows, ITexture *pFlashlightTexture, const Vector &vecBrightness )
	{
		Assert( m_bFlashlightOverride );
		if ( !m_bFlashlightOverride )
		{
			return;
		}

		if ( bFlashlightOn && !m_pFlashlightEffect )
		{
			m_pFlashlightEffect = new CBaseFlashlightEffect( m_nFlashlightEntIndex, NULL );			
		}
		else if ( !bFlashlightOn && m_pFlashlightEffect )
		{
			delete m_pFlashlightEffect;
			m_pFlashlightEffect = NULL;
		}

		if( m_pFlashlightEffect )
		{
			m_pFlashlightEffect->Update( vecPos, vecDir, vecRight, vecUp );
		}
	}

private:
	CBaseFlashlightEffect *m_pFlashlightEffect;
	const char *m_pFlashlightTextureName;
	int m_nFlashlightEntIndex;
	float m_flFov;
	float m_flFarZ;
	float m_flLinearAtten;

	int m_nMuzzleFlashFrameCountdown;
	CountdownTimer m_muzzleFlashTimer;
	float m_flMuzzleFlashBrightness;

	bool m_bFlashlightOn;
	int m_nFXComputeFrame;
	bool m_bFlashlightOverride;
};

CFlashlightEffectManager &FlashlightEffectManager( int32 nSplitscreenPlayerOverride = -1 );

#endif // IN_FLASHLIGHTEFFECT_H