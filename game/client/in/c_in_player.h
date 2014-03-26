//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef C_IN_PLAYER_H
#define C_IN_PLAYER_H

#pragma once

#include "in_playeranimstate.h"
#include "c_baseplayer.h"

#include "baseparticleentity.h"
#include "c_basetempentity.h"

#include "beamdraw.h"
#include "flashlighteffect.h"

#include "in_shareddefs.h"

//=========================================================
// >> C_IN_Player
//=========================================================
class C_IN_Player : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_IN_Player, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_IN_Player();
	~C_IN_Player();

	static C_IN_Player* GetLocalInPlayer();

	// Principales
	virtual void PreThink();

	// Utils
	virtual bool IsPressingButton( int iButton );
	virtual bool IsButtonPressed( int iButton );
	virtual bool IsButtonReleased( int iButton );

	// Información
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	
	// Renderizado
	virtual const QAngle& GetRenderAngles();
	virtual void UpdateClientSideAnimation();

	virtual const QAngle &EyeAngles();

	virtual void CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void InitializePoseParams();

	// Animaciones y Modelos
	virtual CStudioHdr *OnNewModel();

	virtual bool ShouldDrawLocalPlayer();
	virtual bool ShouldDraw();

	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	virtual C_BaseAnimating *BecomeRagdollOnClient( );
	virtual IRagdoll* C_IN_Player::GetRepresentativeRagdoll() const;

	virtual Activity TranslateActivity( Activity actBase );

	// Iluminación
	virtual ShadowType_t ShadowCastType();
	virtual bool ShouldReceiveProjectedTextures( int iFlags );

	virtual void AddEntity();
	virtual void UpdateFlashlight();
	virtual void ReleaseFlashlight();

	virtual bool CreateLightEffects();

	// Armas
	virtual CBaseInWeapon *GetActiveInWeapon( ) const; // SHARED!
	virtual void OnFireBullets( int iBullets ); // SHARED!

	// Salud/Daño
	virtual bool IsDejected() { return m_bDejected; }
	virtual bool InCombat() { return m_bInCombat; }
	
protected:
	CInPlayerAnimState *m_nAnimState;
	EHANDLE	m_nRagdoll;

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	iv_angEyeAngles;

	bool m_bDejected;
	bool m_bInCombat;

	// Linternas
	CFlashlightEffect *m_nFlashlight;
	CFlashlightEffect *m_nThirdFlashlight;
	CFlashlightEffect *m_hFireEffect;

	Beam_t	*m_nFlashlightBeam;

	// Poses
	int iHeadYawPoseParam;
	int	iHeadPitchPoseParam;
	float flHeadYawMin;
	float flHeadYawMax;
	float flHeadPitchMin;
	float flHeadPitchMax;
	float flLastBodyYaw;
	float flCurrentHeadYaw;
	float flCurrentHeadPitch;

private:
	C_IN_Player( const C_IN_Player & );
};

inline C_IN_Player *ToInPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer( ) )
		return NULL;

	return dynamic_cast<C_IN_Player *>(pEntity);
}

//=========================================================
// >> C_TEPlayerAnimEvent
//=========================================================
class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		// Create the effect.
		C_IN_Player *pPlayer = static_cast< C_IN_Player * >(hPlayer.Get( ));

		if ( pPlayer && !pPlayer->IsDormant( ) )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t) iEvent.Get( ), nData );
		}
	}

public:
	CNetworkHandle( CBasePlayer, hPlayer );
	CNetworkVar( int, iEvent );
	CNetworkVar( int, nData );
};

#endif // C_IN_PLAYER_H