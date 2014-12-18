//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_IN_PLAYER_H
#define C_IN_PLAYER_H

#pragma once

#include "in_playeranimstate.h"
#include "c_baseplayer.h"

#include "baseparticleentity.h"
#include "c_basetempentity.h"

#include "beamdraw.h"
#include "beam_shared.h"
#include "flashlighteffect.h"

#include "in_shareddefs.h"
#include "in_player_inventory.h"

//====================================================================
// Clase base para todos los Jugadores
//====================================================================
class C_IN_Player : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_IN_Player, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_IN_Player();
	~C_IN_Player();

	virtual bool IsInGround() { return (GetFlags() & FL_ONGROUND) != 0; }
	virtual bool IsGod() { return (GetFlags() & FL_GODMODE) != 0; }

	static C_IN_Player* GetLocalInPlayer();

	// Principales
	virtual void Spawn();
	virtual void PreThink();
	virtual void PostThink();
	virtual bool Simulate();

	// Utils
	virtual void CreateAnimationState();
	virtual CInPlayerAnimState *GetAnimation() { return m_nAnimation; }

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

	virtual bool GetEntityEyesView( C_BaseAnimating *pEntity, Vector& eyeOrigin, QAngle& eyeAngles, int secureDistance = 10000 );
	virtual void CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	virtual void DoPostProcessingEffects( PostProcessParameters_t &params );

	// Animaciones y Modelos
	virtual void UpdateLookAt();
	virtual CStudioHdr *OnNewModel();

	virtual bool ShouldDrawLocalPlayer();
	virtual bool ShouldDraw();

	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual IRagdoll* C_IN_Player::GetRepresentativeRagdoll() const;

	virtual Activity TranslateActivity( Activity actBase );

	virtual void InitializePoseParams();
	virtual void UpdatePoseParams() { }

	// Iluminación
	virtual ShadowType_t ShadowCastType();
	virtual bool ShouldReceiveProjectedTextures( int iFlags );

	virtual void UpdatePlayerFlashlight();
	virtual void UpdateFlashlight();
	virtual void ReleaseFlashlight();

	virtual void UpdateIncapHeadLight();

	virtual void GetFlashlightPosition( C_IN_Player *pPlayer, Vector &vecPosition, Vector &vecForward, Vector &vecRight, Vector &vecUp, const char *pWeaponAttach = "flashlight" );
	virtual const char *GetFlashlightTextureName() const;
	virtual float GetFlashlightFOV() const;
	virtual float GetFlashlightFarZ();
	virtual float GetFlashlightLinearAtten();
	
	virtual bool CreateLightEffects();

	// Armas
	virtual CBaseInWeapon *GetActiveInWeapon( ) const; // SHARED!
	virtual void OnFireBullets( int iBullets ); // SHARED!

	// Incapacitación
	virtual bool IsIncap() { return m_bIncap; }
	virtual bool IsClimbingIncap() { return m_bClimbingToHell; }
	virtual bool IsWaitingGroundDeath() { return m_bWaitingGroundDeath; }

	virtual float GetHelpProgress() { return m_flHelpProgress; }
	virtual float GetClimbingHold() { return m_flClimbingHold; }

	virtual bool InCombat() { return m_bInCombat; }

	// Inventario
	virtual C_PlayerInventory *GetInventory();

	// Comandos y Cheats
	virtual bool CreateMove( float flInputSampleTime, CUserCmd *pCmd );
	
protected:
	CInPlayerAnimState *m_nAnimation;
	EHANDLE	m_nRagdoll;
	EHANDLE m_nPlayerInventory;

	QAngle m_angEyeAngles;
	CInterpolatedVar< QAngle > iv_angEyeAngles;

	int m_iEyeAngleZ;
	bool m_bInCombat;
	bool m_bMovementDisabled;

	bool m_bIncap;
	bool m_bClimbingToHell;
	bool m_bWaitingGroundDeath;

	int m_iTimesDejected;
	float m_flHelpProgress;
	float m_flClimbingHold;

	int m_afButtonDisabled;

	// Linternas
	CFlashlightEffect *m_nFlashlight;
	CFlashlightEffect *m_nThirdFlashlight;
	CFlashlightEffect *m_nHeadLight;

	Beam_t *m_nFlashlightBeam;

	// Poses
	CountdownTimer m_nBlinkTimer;

	friend class CInPlayerAnimState;

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