//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "player_command.h"
#include "igamemovement.h"
#include "in_buttons.h"
#include "ipredictionsystem.h"
#include "iservervehicle.h"

#include "in_player.h"

//=========================================================
//=========================================================

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;
extern IGameMovement *g_pGameMovement;

//=========================================================
// >> CInPlayerMove
//=========================================================
class CInPlayerMove : public CPlayerMove
{
public:
	DECLARE_CLASS( CInPlayerMove, CPlayerMove );

	virtual void StartCommand( CBasePlayer *player, CUserCmd *cmd );
	virtual void SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );
	virtual void RunCommand( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper );
};

static CInPlayerMove g_PlayerMove;

//=========================================================
//=========================================================
CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}

void CInPlayerMove::StartCommand( CBasePlayer *player, CUserCmd *cmd )
{
	BaseClass::StartCommand( player, cmd );
}

void CInPlayerMove::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	BaseClass::SetupMove( player, ucmd, pHelper, move );

	/*IServerVehicle *pVehicle = player->GetVehicle();

	if ( pVehicle && gpGlobals->frametime != 0 )
		pVehicle->SetupMove( player, ucmd, pHelper, move );*/

	g_pGameMovement->SetupMovementBounds( move );
}

void CInPlayerMove::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );

	/*IServerVehicle *pVehicle = player->GetVehicle( );

	if ( pVehicle && gpGlobals->frametime != 0 )
		pVehicle->FinishMove( player, ucmd, move );*/
}

void CInPlayerMove::RunCommand( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	BaseClass::RunCommand( player, ucmd, moveHelper );
}