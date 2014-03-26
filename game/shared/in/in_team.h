//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef IN_TEAM_H
#define IN_TEAM_H

#pragma once

#include "utlvector.h"

#ifndef CLIENT_DLL
	#include "team.h"
#else
	#define CTeam C_Team
	#define CInTeam C_InTeam

	#include "c_team.h"
#endif

//=========================================================
// >> CInTeam
//=========================================================
class CInTeam : public CTeam
{
public:
	DECLARE_CLASS( CInTeam, CTeam );

#ifndef CLIENT_DLL
	DECLARE_SERVERCLASS_NOBASE();

	virtual void Init( const char *pName, int iNumber );
#else
	DECLARE_CLIENTCLASS_NOBASE( );
#endif

	
};

extern CInTeam *GetGlobalInTeam( int iIndex );

#endif // IN_TEAM_H