//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//

#ifndef AI_BASEMELEE_NPC_H
#define AI_BASEMELEE_NPC_H

#pragma once

#include "ai_basenpc.h"
#include "ai_blended_movement.h"

#include "gib.h"

#include "in_shareddefs.h"

class CAI_BaseMeleeNPC : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CAI_BaseMeleeNPC, CAI_BaseNPC );
	DECLARE_DATADESC();

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
		return 1;
	}

	virtual bool CanAttack()
	{
		return true;
	}

	// Principales
	virtual const char *GetGender();
	virtual void SetupGlobalModelData();

	virtual CGib *CreateGib( const char *pModelName, const Vector &vecOrigin, const Vector &vecDir, int iNum = 0, int iSkin = 0 );

	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void DeathThink();

	virtual bool CanPlayDeathAnim( const CTakeDamageInfo &info );

	// Ataque
	virtual CBaseEntity *MeleeAttack();

	virtual CBaseEntity *AttackEntity( CBaseEntity *pEntity );
	virtual CBaseEntity *AttackCharacter( CBaseCombatCharacter *pCharacter );

	virtual int MeleeAttack1Conditions( float flDot, float flDist );

	// Velocidad
	virtual float GetIdealSpeed() const;
	virtual float GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );

	// Animaciones
	virtual bool OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );

	// Tareas
	virtual int SelectSchedule();
	virtual int TranslateSchedule( int scheduleType );

	DEFINE_CUSTOM_AI;

protected:
	int m_nMoveXPoseParam;
	int m_nMoveYPoseParam;
	int m_nLeanYawPoseParam;

	CBaseEntity *m_nPotentialEnemy;
	CBaseEntity *m_nLastEnemy;

	CTakeDamageInfo m_pLastDamage;
};

#endif // AI_BASEMELEE_NPC_H