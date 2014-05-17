//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "behavior_melee_character.h"

#include "ai_basenpc.h"
#include "in_shareddefs.h"

#include "in_utils.h"

//====================================================================
// Devuelve si es posible atacar a la entidad
//====================================================================
bool CB_MeleeCharacter::CanAttackEntity( CBaseEntity *pEntity )
{
	// Es algo que se puede romper
	if ( Utils::IsBreakable(pEntity) )
		return true;

	// Es algo que se puede mover (Pero no necesariamente algo ligero)
	if ( Utils::IsPhysicsObject(pEntity) )
		return true;

	// Soy un NPC y es mi enemigo
	if ( GetOuter()->IsNPC() )
	{
		if ( pEntity == GetOuter()->MyNPCPointer()->GetEnemy() )
			return true;
	}

	return false;
}

//====================================================================
// Realiza un ataque cuerpo a cuerpo
//====================================================================
CBaseEntity *CB_MeleeCharacter::MeleeAttack()
{
	ConVarRef ai_show_hull_attacks( "ai_show_hull_attacks" );

	// Información
	CBaseEntity *pVictim	= NULL;
	int iDamage				= GetMeleeDamage();
	int iStrikes			= 0;

	do
	{
		pVictim = gEntList.FindEntityInSphere( pVictim, GetOuter()->GetAbsOrigin(), GetMeleeDistance() );

		// Ya hemos cumplido
		if ( iStrikes >= GetMaxObjectsToStrike() )
			break;

		// Ya no esta vivo
		if ( !pVictim || !pVictim->IsAlive() )
			continue;

		// Soy yo
		if ( pVictim == GetOuter() )
			continue;

		// No lo podemos ver (No esta enfrente de nosotros)
		if ( !GetOuter()->FInViewCone(pVictim) || !GetOuter()->FVisible(pVictim) )
			continue;

		//
		// Es un Jugador o un NPC
		//
		if ( pVictim->MyCombatCharacterPointer() )
		{
			// Soy un NPC
			if ( GetOuter()->IsNPC() )
			{
				// Es mi amigo
				if ( GetOuter()->IRelationType(pVictim) == D_LIKE )
				{
					// Somos inmunes al ataque aliado
					if ( GetOuter()->MyNPCPointer()->CapabilitiesGet() & bits_CAP_FRIENDLY_DMG_IMMUNE )
						continue;

					// Hacemos menos daño
					iDamage = RandomInt(1, 3);
				}
			}

			pVictim = AttackCharacter( pVictim->MyCombatCharacterPointer() );
		}
		else
		{
			pVictim = AttackEntity( pVictim );
		}

		// Hemos dado con algo/alguien
		if ( pVictim )
		{
			++iStrikes;
			OnEntityAttacked( pVictim );

			// Fuerza del ataque
			Vector vecForce = pVictim->WorldSpaceCenter() - GetOuter()->WorldSpaceCenter();
			VectorNormalize( vecForce );
			vecForce *= 5 * 24;

			// Realizamos el daño
			CTakeDamageInfo info( GetOuter(), GetOuter(), vecForce, GetOuter()->GetAbsOrigin(), iDamage, DMG_SLASH );
			pVictim->TakeDamage( info );
		}

	} while ( pVictim );

	// Veces que puedo golpear a un objeto y el numero de objetos que he golpeado
	/*int iMaxStrikes		= GetMaxObjectsToStrike();
	int iStrikes		= 0;
	
	trace_t	tr;

	// Realizamos un bucle para golpear a varias entidades
	for ( int i = 0; i < iMaxStrikes; ++i )
	{
		// Trazamos una línea enfrente de mi
		//UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin() + vecForward * GetMeleeDistance(), WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, GetOuter(), GetOuter()->GetCollisionGroup(), &tr );

		Ray_t ray;
		ray.Init( GetAbsOrigin(), GetAbsOrigin() + vecForward * GetMeleeDistance(), WorldAlignMins(), WorldAlignMaxs() );

		CTraceFilterSimple traceFilter( GetOuter(), GetOuter()->GetCollisionGroup() );
		enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

		// Handy debuging tool to visualize HullAttack trace
		if ( ai_show_hull_attacks.GetBool() )
		{
			float length	 = ( tr.endpos - tr.startpos ).Length();
			Vector direction = ( tr.endpos - tr.startpos );
			VectorNormalize( direction );

			Vector vecHullMaxs	= WorldAlignMaxs();
			vecHullMaxs.x		= length + vecHullMaxs.x;

			NDebugOverlay::BoxDirection( tr.startpos, WorldAlignMins(), WorldAlignMaxs(), direction, 100,255,255,20,1.0 );
			NDebugOverlay::BoxDirection( tr.startpos, WorldAlignMins(), vecHullMaxs, direction, 255,0,0,20,1.0 );
		}

		// Hemos dado con nada...
		if ( tr.fraction == 1.0 || !tr.m_pEnt )
			continue;
		
		// Es el mismo que he golpeado hace poco
		if ( tr.m_pEnt == pVictim )
			continue;

		CBaseCombatCharacter *pCharacter	= dynamic_cast<CBaseCombatCharacter *>( tr.m_pEnt );
		CAI_BaseNPC *pNPC					= dynamic_cast<CAI_BaseNPC *>( tr.m_pEnt );

		//
		// Es un Jugador o un NPC
		//
		if ( pCharacter )
		{
			// Soy un NPC
			if ( GetOuter()->IsNPC() )
			{
				// Es mi amigo
				if ( GetOuter()->MyNPCPointer()->IRelationType(pCharacter) == D_LIKE )
				{
					// Somos inmunes al ataque aliado
					if ( pNPC && pNPC->CapabilitiesGet() & bits_CAP_FRIENDLY_DMG_IMMUNE )
						continue;

					// Hacemos menos daño
					damage = RandomInt(1, 3);
				}
			}

			pVictim = AttackCharacter( pCharacter );
		}

		//
		// Es un objeto
		//
		else
		{
			pVictim = AttackEntity( tr.m_pEnt );
		}

		// Hemos dado con algo/alguien
		if ( pVictim )
		{
			++strikes;
			OnEntityAttacked( pVictim );

			// Fuerza del ataque
			Vector vecForce = pVictim->WorldSpaceCenter() - GetOuter()->WorldSpaceCenter();
			VectorNormalize( vecForce );
			vecForce *= 5 * 24;

			// Realizamos el daño
			CTakeDamageInfo info( GetOuter(), GetOuter(), vecForce, GetOuter()->GetAbsOrigin(), damage, damageBits );
			pVictim->TakeDamage( info );
		}
	}*/

	//m_nLastEnemy = pVictim;

	// FIXME: Por ahora solo devolvemos nuestra última victima
	return pVictim;
}

//====================================================================
// Realiza el ataque a una entidad
//====================================================================
CBaseEntity *CB_MeleeCharacter::AttackEntity( CBaseEntity *pEntity )
{
	IPhysicsObject *pPhysObj = pEntity->VPhysicsGetObject();

	// No es algo que pueda romper o mover...
	if ( !Utils::IsBreakable(pEntity) && !pPhysObj || pEntity->IsWorld() )
		return NULL;

	Vector vecForward;
	QAngle eyeAngles = GetOuter()->EyeAngles();

	AngleVectors( eyeAngles, &vecForward, NULL, NULL );

	// Tiene físicas, darle un empujon
	if ( pPhysObj )
	{
		float flEntityMass		= pPhysObj->GetMass();
		float flMaxMass			= GetMaxMassObject();

		// ¡No es pesado! Lo golpearemos sin problemas
		if ( flEntityMass <= flMaxMass )
		{
			PhysicsImpactSound( pEntity, pPhysObj, CHAN_STATIC, pPhysObj->GetMaterialIndex(), pPhysObj->GetMaterialIndex(), 1.0, 800 );

			// Impulso hacia adelante y hacia arriba
			vecForward		= vecForward * (flMaxMass*8);
			vecForward.z	+= (flMaxMass*2);

			// Ahora aplicamos el impulso de verdad
			AngularImpulse angVelocity( RandomFloat(-80, 80), 20, RandomFloat(-160, 160) );
			pPhysObj->AddVelocity( &vecForward, &angVelocity );
		}
	}

	return pEntity;
}

//====================================================================
// Realiza el ataque a un personaje
//====================================================================
CBaseEntity *CB_MeleeCharacter::AttackCharacter( CBaseCombatCharacter *pCharacter )
{
	if ( GetOuter()->IsNPC() )
	{
		if ( !GetOuter()->MyNPCPointer()->GetEnemy() )
			return NULL;
	}

	// FIXME: Punch
	GetOuter()->EmitSound("Infected.Hit");

	// Nuestra victima es un jugador
	if ( pCharacter->IsPlayer() )
		pCharacter->ViewPunch( QAngle(5, 0, -5) );

	return pCharacter;
}	