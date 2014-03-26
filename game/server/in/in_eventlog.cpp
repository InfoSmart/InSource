//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "../EventLog.h"
#include "KeyValues.h"

//=========================================================
// >> CInEventLog
//=========================================================
class CInEventLog : public CEventLog
{
public:
	DECLARE_CLASS( CInEventLog, CEventLog );

	virtual bool PrintEvent( IGameEvent *pEvent );
};

//=========================================================
//=========================================================

CInEventLog g_InEventLog;

//=========================================================
//=========================================================
CEventLog *GameLogSystem()
{
	return &g_InEventLog;
}

//=========================================================
//=========================================================
bool CInEventLog::PrintEvent( IGameEvent *pEvent )
{
	return BaseClass::PrintEvent( pEvent );
}