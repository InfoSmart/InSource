//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef C_IN_RAGDOLL
#define C_IN_RAGDOLL

#pragma once

//==============================================
// >> C_IN_Ragdoll
//==============================================
class C_IN_Ragdoll : public C_BaseAnimatingOverlay
{
public:
	DECLARE_CLASS( C_IN_Ragdoll, C_BaseAnimatingOverlay );
	DECLARE_CLIENTCLASS();

	C_IN_Ragdoll();
	~C_IN_Ragdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	//virtual int GetPlayerEntIndex() const;
	virtual IRagdoll* GetIRagdoll() const;

	virtual void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );

	virtual void Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity  );
	virtual void CreateRagdoll();

private:
	C_IN_Ragdoll( const C_IN_Ragdoll & ) { }

private:

	EHANDLE	hPlayer;
	CNetworkVector( vecRagdollVelocity );
	CNetworkVector( vecRagdollOrigin );
};


#endif // C_IN_RAGDOLL