//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef BEHAVIOR_MELEE_CHARACTER_H
#define BEHAVIOR_MELEE_CHARACTER_H

#pragma once

class CB_MeleeCharacter
{
public:
	virtual int GetMeleeDamage()
	{
		return 1;
	}

	virtual int GetMeleeDistance()
	{
		return 100;
	}

	virtual float GetMaxMassObject()
	{
		return 100;
	}

	virtual int GetMaxObjectsToStrike()
	{
		return 5;
	}

	virtual bool CanAttack()
	{
		return true;
	}

	virtual void SetOuter( CBaseCombatCharacter *pEntity ) { m_nParent = pEntity; }
	virtual CBaseCombatCharacter *GetOuter() { return m_nParent; }

	virtual bool CanAttackEntity( CBaseEntity *pEntity );
	virtual CBaseEntity *MeleeAttack();

	virtual CBaseEntity *AttackEntity( CBaseEntity *pEntity );
	virtual CBaseEntity *AttackCharacter( CBaseCombatCharacter *pCharacter );

	virtual void OnEntityAttacked( CBaseEntity *pVictim ) { }

protected:
	CBaseCombatCharacter *m_nParent;
};

#endif