//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "in_ragdoll.h"
#include "in_player.h"

//==============================================
// Información y Red
//==============================================

LINK_ENTITY_TO_CLASS( in_ragdoll, CIN_Ragdoll );

// DT_INRagdoll
IMPLEMENT_SERVERCLASS_ST_NOBASE( CIN_Ragdoll, DT_InRagdoll )
	SendPropVector( SENDINFO(vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( hPlayer ) ),
	SendPropVector( SENDINFO( vecRagdollVelocity ) ),

	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
END_SEND_TABLE()

//==============================================
// Estado de la transmisión
//==============================================
int CIN_Ragdoll::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}