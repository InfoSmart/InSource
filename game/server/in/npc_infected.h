//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef NPC_INFECTED_H
#define NPC_INFECTED_H

#pragma once

#include "ai_base_infected.h"
#include "in_shareddefs.h"

#include "doors.h"

//=========================================================
// >> CInfected
//=========================================================
class CInfected : public CBaseInfected
{
public:
	DECLARE_CLASS( CInfected, CBaseInfected );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	// Principales
	virtual void Spawn();
	virtual void Precache();
	virtual void NPCThink();

	Class_T Classify();

	virtual void UpdateFall();
	virtual void UpdateFallOn();
	virtual void UpdateClimb();

	virtual void SetInfectedModel();
	virtual void SetClothColor();

	virtual void UpdateGesture();
	virtual void HandleAnimEvent( animevent_t *pEvent );

	virtual void CreateGoreGibs( int iBodyPart, const Vector &vecOrigin, const Vector &vecDir );

	// Relajarse / Dormir
	virtual bool IsRelaxing() { return m_bRelaxing; }
	virtual bool IsSleeping() { return m_bSleeping; }

	virtual void UpdateRelaxStatus();

	virtual void GoToRelax();
	virtual void GoRelaxToStand();

	virtual void GoToSleep();
	virtual void GoWake();

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
	virtual void OnTouch( CBaseEntity *pActivator );
	
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
	virtual void GatherConditions();

	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	//virtual int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual int SelectSchedule();
	virtual int SelectIdleSchedule();
	virtual int SelectCombatSchedule();
	virtual int TranslateSchedule( int scheduleType );

protected:
	//bool m_bCanAttack;

	float m_flNextPainSound;
	float m_flNextAlertSound;
	float m_flAttackIn;

	int m_iFaceGesture;

	int m_iNextRelaxing;
	int m_iNextSleep;

	bool m_bRelaxing;
	bool m_bSleeping;
	bool m_bCanSprint;

	bool m_bFalling;

	int m_iRelaxingSequence;
	int m_iSleepSequence;
};

#endif // NPC_INFECTED_H