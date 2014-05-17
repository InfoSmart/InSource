//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_PLAYERANIMSTATE_H
#define AP_PLAYERANIMSTATE_H

#pragma once

#include "in_playeranimstate.h"

#ifdef CLIENT_DLL
	class C_AP_Player;

	#define CAP_Player C_AP_Player
#else
	class CAP_Player;
#endif

//==============================================
// >> CInPlayerAnimState
//
// Administra y reproduce las animaciones de los
// Jugadores en cliente y servidor
//==============================================
class CAPPlayerAnimState : public CInPlayerAnimState
{
public:
	DECLARE_CLASS( CAPPlayerAnimState, CInPlayerAnimState );

	//CAPPlayerAnimState();
	CAPPlayerAnimState( CAP_Player *pPlayer, MultiPlayerMovementData_t &pMovementData );

	virtual bool ShouldUpdateAnimState();
	virtual Activity CalcMainActivity();

	//virtual bool HandleClimbingToHell( Activity &idealActivity );
	virtual bool HandleIncap( Activity &idealActivity );
	virtual bool HandleFalling( Activity &idealActivity );

	CAP_Player *GetPlayer() { return m_nApPlayer; }

protected:
	CAP_Player *m_nApPlayer;

	bool m_bFalling;
};

CAPPlayerAnimState *CreateApPlayerAnimationState( CAP_Player *pPlayer );

#endif