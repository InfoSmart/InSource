//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============

#include "cbase.h"
#include "gamemovement.h"
#include "in_buttons.h"
#include "movevars_shared.h"

#ifdef CLIENT_DLL
	#include "c_in_player.h"
#else
	#include "in_player.h"
#endif

//==============================================
// >> CInGameMovement
//==============================================
class CInGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( CInGameMovement, CGameMovement );
	CInGameMovement();
};

static CInGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = (IGameMovement *) &g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement );

//==============================================
// Constructor
//==============================================
CInGameMovement::CInGameMovement()
{
}