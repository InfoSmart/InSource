//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//

#ifndef NPC_BOSS_H
#define NPC_BOSS_H

#pragma once

#include "ai_base_infected.h"
#include "in_shareddefs.h"

//=========================================================
// >> CBoss
//=========================================================
class CBoss : public CBaseInfected
{
public:
	DECLARE_CLASS( CBoss, CBaseInfected );
	DECLARE_DATADESC();

	virtual float GetMaxMassObject()
	{
		return 1800;
	}

	virtual int GetMaxObjectsToStrike()
	{
		return 10;
	}

	// Principales
	virtual void Spawn();
	virtual void Precache();
	virtual void NPCThink();

	virtual void UpdateRealAttack();
	virtual void UpdateStress();
	virtual void UpdateEnemyChase();

	Class_T Classify();
	virtual void HandleAnimEvent( animevent_t *pEvent );

	// Sonidos
	virtual void IdleSound();
	virtual void PainSound( const CTakeDamageInfo &info );
	virtual void AlertSound();
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual void AttackSound();

	// Ataque
	virtual bool CanAttack();
	virtual void OnEnemyChanged( CBaseEntity *pOldEnemy, CBaseEntity *pNewEnemy );

	virtual void OnEntityAttacked( CBaseEntity *pVictim );
	virtual void DismemberingEntity( CBaseEntity *pEntity );

	virtual int GetMeleeDamage();
	virtual int GetMeleeDistance();

	// Animaciones
	virtual bool IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;
	Activity NPC_TranslateActivity( Activity pActivity );

	// Tareas
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	//virtual int SelectSchedule();
	virtual int TranslateSchedule( int scheduleType );

	DEFINE_CUSTOM_AI;

protected:
	bool m_bCanAttack;
	bool m_bISeeAEnemy;

	float m_flNextPainSound;
	float m_flNextAlertSound;

	CountdownTimer m_nAttackStress;

	int m_iAttackLayer;
};

#endif