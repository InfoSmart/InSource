//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef IN_UTILS_H
#define IN_UTILS_H

#pragma once

//=========================================================
//=========================================================
class Utils
{
public:
	DECLARE_CLASS_NOBASE( Utils );

	static bool IsBreakable( CBaseEntity *pEntity );
	static CBaseEntity *FindNearestPhysicsObject( const Vector &vOrigin, float fMaxDist, float fMinMass = 0, float fMaxMass = 500, CBaseEntity *pFrom = NULL );
	static bool IsPhysicsObject( CBaseEntity *pEntity );

	static bool RunOutEntityLimit( int iTolerance = 20 );
};

#endif // IN_UTILS_H