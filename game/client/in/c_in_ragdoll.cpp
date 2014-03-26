//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "c_in_ragdoll.h"
#include "c_in_player.h"

//=========================================================
// Información y Red
//=========================================================

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_IN_Ragdoll, DT_InRagdoll, CIN_Ragdoll )
	RecvPropVector( RECVINFO( vecRagdollOrigin ) ),
	RecvPropEHandle( RECVINFO( hPlayer ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),

	RecvPropInt( RECVINFO( m_nForceBone ) ),
	RecvPropVector( RECVINFO( m_vecForce ) ),
	RecvPropVector( RECVINFO( vecRagdollVelocity ) )
END_RECV_TABLE( )

//=========================================================
// Constructor
//=========================================================
C_IN_Ragdoll::C_IN_Ragdoll()
{
}

//=========================================================
// Destructor
//=========================================================
C_IN_Ragdoll::~C_IN_Ragdoll()
{
	PhysCleanupFrictionSounds( this );

	if ( hPlayer )
	{
		hPlayer->CreateModelInstance();
	}
}

//=========================================================
//=========================================================
void C_IN_Ragdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;

	VarMapping_t *pSrc = pSourceEntity->GetVarMapping( );
	VarMapping_t *pDest = GetVarMapping( );

	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count( ); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		for ( int j = 0; j < pSrc->m_Entries.Count( ); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName( ),
				pDestEntry->watcher->GetDebugName( ) ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

//=========================================================
//=========================================================
void C_IN_Ragdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject( );

	if ( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;

		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime( );
}

//=========================================================
//=========================================================
void C_IN_Ragdoll::CreateRagdoll()
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_IN_Player *pPlayer = dynamic_cast< C_IN_Player* >(hPlayer.Get( ));

	if ( pPlayer && !pPlayer->IsDormant( ) )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping( );

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer( ));
		if ( bRemotePlayer )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles( ) );
			GetRotationInterpolator( ).Reset( gpGlobals->curtime );

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence( ) );
			m_flPlaybackRate = pPlayer->GetPlaybackRate( );
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( vecRagdollOrigin );

			SetAbsAngles( pPlayer->GetRenderAngles( ) );

			SetAbsVelocity( vecRagdollVelocity );

			int iSeq = LookupSequence( "walk_lower" );
			if ( iSeq == -1 )
			{
				Assert( false );	// missing walk_lower?
				iSeq = 0;
			}

			SetSequence( iSeq );	// walk_lower, basic pose
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( vecRagdollOrigin );

		SetAbsOrigin( vecRagdollOrigin );
		SetAbsVelocity( vecRagdollVelocity );

		Interp_Reset( GetVarMapping( ) );

	}

	SetModelIndex( m_nModelIndex );

	// Make us a ragdoll..
	SetRenderFX( hPlayer->GetRenderFX() );

	matrix3x4a_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4a_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4a_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;

	if ( pPlayer && !pPlayer->IsDormant( ) )
	{
		pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	}
	else
	{
		GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	}

	InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
}

//=========================================================
//=========================================================
void C_IN_Ragdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateRagdoll();
	}
}

//=========================================================
//=========================================================
IRagdoll* C_IN_Ragdoll::GetIRagdoll( ) const
{
	return m_pRagdoll;
}