//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//

#ifndef NPC_INFECTED_H
#define NPC_INFECTED_H

#pragma once

#include "ai_basemelee_npc.h"
#include "in_shareddefs.h"

//=========================================================
// >> CInfected
//=========================================================
class CInfected : public CAI_BaseMeleeNPC
{
public:
	DECLARE_CLASS( CInfected, CAI_BaseMeleeNPC );
	DECLARE_DATADESC();

	virtual	bool AllowedToIgnite() { return true; }

	// Principales
	virtual void Spawn();
	virtual void Precache();
	virtual void Think();

	Class_T Classify();

	virtual void SetInfectedModel();
	virtual void SetClothColor();

	virtual void HandleGesture();
	virtual void HandleAnimEvent( animevent_t *pEvent );

	virtual void CreateGoreGibs( int iBodyPart, const Vector &vecOrigin, const Vector &vecDir );

	// Finjir
	virtual void UpdateRelaxStatus();

	virtual void GoToSit();
	virtual void GoUp();

	// Sonidos
	virtual bool ShouldPlayIdleSound();
	virtual void IdleSound();

	virtual void PainSound( const CTakeDamageInfo &info );
	virtual void AlertSound();
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual void AttackSound();

	// Ataque
	virtual bool CanAttack();
	virtual void OnEnemyChanged( CBaseEntity *pOldEnemy, CBaseEntity *pNewEnemy );
	
	virtual int	OnTakeDamage( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );

	virtual void Dismembering( const CTakeDamageInfo &info );

	virtual int GetMeleeDamage();
	virtual int GetMeleeDistance();

	// Animaciones
	virtual bool CanPlayDeathAnim( const CTakeDamageInfo &info );
	virtual bool IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;
		
	Activity NPC_TranslateActivity( Activity pActivity );

	// Tareas
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	virtual int SelectSchedule();
	virtual int TranslateSchedule( int scheduleType );

	DEFINE_CUSTOM_AI;

protected:
	//bool m_bCanAttack;

	float m_flNextPainSound;
	float m_flNextAlertSound;
	float m_flAttackIn;

	int m_iFaceGesture;
	int m_iAttackLayer;

	int m_iNextRelaxing;
	bool m_bRelaxing;
};

#endif // NPC_INFECTED_H