//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef IN_UTILS_H
#define IN_UTILS_H

#pragma once

#include "in_player.h"

//====================================================================
// Utils
//
// >> Clase con utilidades
//====================================================================
class Utils
{
public:
	DECLARE_CLASS_NOBASE( Utils );

	static void NormalizeAngle( float& fAngle );
	static void DeNormalizeAngle( float& fAngle );
	static void GetAngleDifference( QAngle const& angOrigin, QAngle const& angDestination, QAngle& angDiff );

	static bool IsBreakable( CBaseEntity *pEntity );
	static bool IsBreakableSurf( CBaseEntity *pEntity );
	static bool IsDoor( CBaseEntity *pEntity );
	static CBaseEntity *FindNearestPhysicsObject( const Vector &vOrigin, float fMaxDist, float fMinMass = 0, float fMaxMass = 500, CBaseEntity *pFrom = NULL );
	static bool IsPhysicsObject( CBaseEntity *pEntity );

	static bool RunOutEntityLimit( int iTolerance = 20 );

	static IGameEvent *CreateLesson( const char *pLesson, CBaseEntity *pSubject = NULL );
};

#endif // IN_UTILS_H