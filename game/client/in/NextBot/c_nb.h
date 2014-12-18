//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_NEXTBOT_H
#define C_NEXTBOT_H

#pragma once

#include "nb_playeranimstate.h"

#include "utlvector.h"
#include "util_shared.h"

//====================================================================
//====================================================================
class C_NextBot : public C_BaseCombatCharacter
{
public:
	DECLARE_CLASS( C_NextBot, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();

	// Principales
	C_NextBot();
	~C_NextBot();

	virtual void Spawn();

	// Animaciones
	virtual void UpdateClientSideAnimation();

	// Movimiento
	virtual bool IsRunning() const { return m_bIsRunning; }

protected:

	// Movimiento
	CNBAnimState *m_nBotAnimState;

	CountdownTimer m_nBlinkTimer;

	int m_iSequence;

	float m_flCurrentHeadPitch;
	float m_flCurrentHeadYaw;

	Vector m_lookAt;

	bool m_bIsRunning;

};

inline C_NextBot *ToNextBot( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	return dynamic_cast<C_NextBot *>( pEntity );
}

#endif // C_NEXTBOT_H