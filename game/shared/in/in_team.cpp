//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "in_team.h"

#ifndef CLIENT_DLL
	#include "entitylist.h"
#else
	#include "hud.h"
	#include "recvproxy.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Información y Red
//=========================================================

#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS( in_team_manager, CInTeam );

#else

#endif

// DT_InTeam
BEGIN_NETWORK_TABLE_NOBASE( CInTeam, DT_InTeam )
END_NETWORK_TABLE( )

//=========================================================
// Devuelve el manejador de equipo
//=========================================================
CInTeam *GetGlobalInTeam( int iIndex )
{
	return (CInTeam *)GetGlobalTeam( iIndex );
}

#ifndef CLIENT_DLL

//=========================================================
//=========================================================
void CInTeam::Init( const char *pName, int iNumber  )
{
	BaseClass::Init( pName, iNumber );

	NetworkProp()->SetUpdateInterval( 0.75f );
}

#endif