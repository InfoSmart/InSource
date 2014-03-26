//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef IN_RAGDOLL_H
#define IN_RAGDOLL_H

#pragma once

//==============================================
// >> CIN_Ragdoll
//==============================================
class CIN_Ragdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CIN_Ragdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	virtual int UpdateTransmitState();

public:
	CNetworkHandle( CBaseEntity, hPlayer );
	CNetworkVector( vecRagdollVelocity );
	CNetworkVector( vecRagdollOrigin );
};

inline CIN_Ragdoll *ToRagdoll( CBaseEntity *pEntity )
{
	return dynamic_cast<CIN_Ragdoll *>( pEntity );
}

#endif //IN_RAGDOLL_H