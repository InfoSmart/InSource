//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"
#include "in_utils.h"

#include "physobj.h"
#include "collisionutils.h"
#include "movevars_shared.h"

#include "BasePropDoor.h"
#include "doors.h"
#include "ai_basenpc.h"
#include "func_breakablesurf.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//===============================================================================
//===============================================================================
void NormalizeAngle( float& fAngle )
{
	if (fAngle < 0.0f)
		fAngle += 360.0f;
	else if (fAngle >= 360.0f)
		fAngle -= 360.0f;
}

//===============================================================================
//===============================================================================
void Utils::DeNormalizeAngle( float& fAngle )
{
	if (fAngle < -180.0f)
		fAngle += 360.0f;
	else if (fAngle >= 180.0f)
		fAngle -= 360.0f;
}

//===============================================================================
//===============================================================================
void Utils::GetAngleDifference( QAngle const& angOrigin, QAngle const& angDestination, QAngle& angDiff )
{
	angDiff = angDestination - angOrigin;

	Utils::DeNormalizeAngle( angDiff.x );
	Utils::DeNormalizeAngle( angDiff.y );
}

//====================================================================
// Devuelve si la entidad es una pared que puede romperse
//====================================================================
bool Utils::IsBreakableSurf( CBaseEntity *pEntity )
{
	// No puede recibir daño
	if ( pEntity->m_takedamage != DAMAGE_YES )
		return false;

	// Es una superficie que se puede romper
	if ( (dynamic_cast<CBreakableSurface *>(pEntity)) )
		return true;

	// Es una pared que se puede romper
	if ( (dynamic_cast<CBreakable *>(pEntity)) )
		return true;

	return false;
}

//====================================================================
// Devuelve si la entidad es un objeto que puede romperse
//====================================================================
bool Utils::IsBreakable( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return false;

	// No puede recibir daño
	if ( pEntity->m_takedamage != DAMAGE_YES )
		return false;

	// Es una entidad que se puede romper
	if ( (dynamic_cast<CBreakableProp *>(pEntity)) )
		return true;

	// Es una pared que se puede romper
	if ( (dynamic_cast<CBreakable *>(pEntity)) )
		return true;

	// Es una superficie que se puede romper
	if ( (dynamic_cast<CBreakableSurface *>(pEntity)) )
	{
		CBreakableSurface *surf = static_cast< CBreakableSurface * >( pEntity );

		// Ya esta roto
		if ( surf->m_bIsBroken )
			return false;
		
		return true;
	}

	// Es una puerta
	if ( (dynamic_cast<CBasePropDoor *>(pEntity)) )
		return true;

	return false;
}

//====================================================================
// Devuelve si la entidad es una puerta
//====================================================================
bool Utils::IsDoor( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return false;

	CBaseDoor *pDoor = dynamic_cast<CBaseDoor *>( pEntity );

	if ( pDoor )
		return true;

	CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>( pEntity );

	if ( pPropDoor )
		return true;

	return false;
}

//====================================================================
// Devuelve el objeto con fisica más cercano
//====================================================================
CBaseEntity *Utils::FindNearestPhysicsObject( const Vector &vOrigin, float fMaxDist, float fMinMass, float fMaxMass, CBaseEntity *pFrom )
{
	CBaseEntity *pFinalEntity	= NULL;
	CBaseEntity *pThrowEntity	= NULL;
	float flNearestDist			= 0;

	// Buscamos los objetos que podemos lanzar
	do
	{
		// Objetos con físicas
		pThrowEntity = gEntList.FindEntityByClassnameWithin( pThrowEntity, "prop_physics", vOrigin, fMaxDist );
	
		// Ya no existe
		if ( !pThrowEntity )
			continue;

		// La entidad que lo quiere no puede verlo
		if ( pFrom )
		{
			if ( !pFrom->FVisible(pThrowEntity) )
				continue;
		}

		// No se ha podido acceder a la información de su fisica
		if ( !pThrowEntity->VPhysicsGetObject() )
			continue;

		// No se puede mover o en si.. lanzar
		if ( !pThrowEntity->VPhysicsGetObject()->IsMoveable() )
			continue;

		Vector v_center	= pThrowEntity->WorldSpaceCenter();
		float flDist	= UTIL_DistApprox2D( vOrigin, v_center );

		// Esta más lejos que el objeto anterior
		if ( flDist > flNearestDist && flNearestDist != 0 )
			continue;

		// Calcular la distancia al enemigo
		if ( pFrom && pFrom->IsNPC() )
		{
			CAI_BaseNPC *pNPC		= dynamic_cast<CAI_BaseNPC *>( pFrom );

			if ( pNPC && pNPC->GetEnemy() )
			{
				Vector vecDirToEnemy	= pNPC->GetEnemy()->GetAbsOrigin() - pNPC->GetAbsOrigin();
				vecDirToEnemy.z			= 0;

				Vector vecDirToObject = pThrowEntity->WorldSpaceCenter() - vOrigin;
				VectorNormalize(vecDirToObject);
				vecDirToObject.z = 0;

				if ( DotProduct(vecDirToEnemy, vecDirToObject) < 0.8 )
					continue;
			}
		}

		// Obtenemos su peso
		float pEntityMass = pThrowEntity->VPhysicsGetObject()->GetMass();

		// Muy liviano
		if ( pEntityMass < fMinMass && fMinMass > 0 )
			continue;
			
		// ¡Muy pesado!
		if ( pEntityMass > fMaxMass )
			continue;

		// No lanzar objetos que esten sobre mi cabeza
		if ( v_center.z > vOrigin.z )
			continue;

		if ( pFrom )
		{
			Vector vecGruntKnees;
			pFrom->CollisionProp()->NormalizedToWorldSpace( Vector(0.5f, 0.5f, 0.25f), &vecGruntKnees );

			vcollide_t *pCollide = modelinfo->GetVCollide( pThrowEntity->GetModelIndex() );
		
			Vector objMins, objMaxs;
			physcollision->CollideGetAABB(&objMins, &objMaxs, pCollide->solids[0], pThrowEntity->GetAbsOrigin(), pThrowEntity->GetAbsAngles());

			if ( objMaxs.z < vecGruntKnees.z )
				continue;
		}

		// Este objeto es perfecto, guardamos su distancia por si encontramos otro más cerca
		flNearestDist	= flDist;
		pFinalEntity	= pThrowEntity;

	} while( pThrowEntity );

	// No pudimos encontrar ningún objeto
	if ( !pFinalEntity )
		return NULL;

	return pFinalEntity;
}

//====================================================================
// Devuelve si la entidad es un objeto con fisica.
//====================================================================
bool Utils::IsPhysicsObject( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return false;

	if ( !pEntity->VPhysicsGetObject() )
		return false;

	if ( !pEntity->VPhysicsGetObject()->IsMoveable() )
		return false;		

	return true;
}

//====================================================================
// Devuelve si el motor se esta quedando sin espacios para
// las entidades
//====================================================================
bool Utils::RunOutEntityLimit( int iTolerance )
{
	if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - iTolerance) )
		return false;

	return true;
}

//====================================================================
// Crea la instancia de evento para crear una lección del Instructor
//====================================================================
IGameEvent *Utils::CreateLesson( const char *pLesson, CBaseEntity *pSubject )
{
	IGameEvent *pEvent = gameeventmanager->CreateEvent( pLesson, true );

	if ( pEvent )
	{
		if ( pSubject )
			pEvent->SetInt( "subject", pSubject->entindex() );
	}

	return pEvent;
}