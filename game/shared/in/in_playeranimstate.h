//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef IN_PLAYERANIMSTATE_H
#define IN_PLAYERANIMSTATE_H

#pragma once

#include "convar.h"
#include "multiplayer_animstate.h"
#include "in_shareddefs.h"

#ifdef CLIENT_DLL
	class C_IN_Player;
	class C_BaseInWeapon;

	#define CBaseInWeapon C_BaseInWeapon
	#define CIN_Player C_IN_Player
#else
	class CBaseInWeapon;
	class CIN_Player;
#endif

//==============================================
// >> CInPlayerAnimState
//
// Administra y reproduce las animaciones de los
// Jugadores en cliente y servidor
//==============================================
class CInPlayerAnimState : public CMultiPlayerAnimState
{
public:
	DECLARE_CLASS( CInPlayerAnimState, CMultiPlayerAnimState );

	CInPlayerAnimState();
	CInPlayerAnimState( CIN_Player *pPlayer, MultiPlayerMovementData_t &pMovementData );
	
	virtual Activity TranslateActivity( Activity actDesired );

	virtual void Update();
	virtual Activity CalcMainActivity();

	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	virtual void Teleport( const Vector *pNewOrigin, const QAngle *pNewAngles, CBasePlayer *pPlayer );

	virtual bool HandleSwimming( Activity &idealActivity );
	virtual bool HandleMoving( Activity &idealActivity );
	virtual bool HandleDucking (Activity &idealActivity );

	virtual void GrabEarAnimation();
	CIN_Player *GetPlayer() { return m_nPlayer; }

protected:
	virtual bool SetupPoseParameters( CStudioHdr *pStudioHdr );
	virtual void ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	virtual void ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );

	CIN_Player *m_nPlayer;
};

CInPlayerAnimState *CreatePlayerAnimationState( CIN_Player *pPlayer );

#endif //IN_PLAYERANIMSTATE_H