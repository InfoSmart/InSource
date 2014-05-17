//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AI_BASEMELEE_NPC_H
#define AI_BASEMELEE_NPC_H

#pragma once

#include "ai_basenpc.h"
#include "ai_blended_movement.h"
#include "ai_behavior.h"

#include "gib.h"

#include "in_shareddefs.h"
#include "behavior_melee_character.h"
#include "ai_behavior_climb.h"

typedef CAI_BlendingHost<CAI_BehaviorHost<CAI_BaseNPC>> CAI_BaseInfected;

class CBaseInfected : public CAI_BaseInfected, public CB_MeleeCharacter
{
public:
	DECLARE_CLASS( CBaseInfected, CAI_BaseInfected );
	DECLARE_DATADESC();

	virtual	bool AllowedToIgnite() { return true; }
	virtual bool IsRunning() { return ( m_flGroundSpeed >= 200 ); }

	virtual bool HasActivity( Activity iAct )
	{
		return ( SelectWeightedSequence( NPC_TranslateActivity(iAct) ) == ACTIVITY_NOT_AVAILABLE ) ? false : true;
	}

	// Principales
	virtual void Spawn();
	virtual void NPCThink();

	virtual bool CreateBehaviors();

	virtual void UpdateFall();
	virtual void UpdateClimb();

	virtual const char *GetGender();
	virtual void SetupGlobalModelData();

	virtual CGib *CreateGib( const char *pModelName, const Vector &vecOrigin, const Vector &vecDir, int iNum = 0, int iSkin = 0 );

	// Obstrucción
	virtual CBaseEntity *GetBlockingEnt() { return m_nBlockingEnt.Get(); }
	virtual bool HasObstruction();

	virtual bool OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual bool OnObstruction( AILocalMoveGoal_t *pMoveGoal, CBaseEntity *pEntity, float distClear, AIMoveResult_t *pResult );
	virtual bool OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, float distClear, AIMoveResult_t *pResult );
	virtual bool OnUpcomingPropDoor( AILocalMoveGoal_t *pMoveGoal, CBasePropDoor *pDoor, float distClear, AIMoveResult_t *pResult );

	virtual bool HitsWall( float flDistance = 15, int iHeight = 0 );

	// Ataque
	virtual void SetIdealHealth( int iHealth, int iMulti = 1 );

	virtual int	OnTakeDamage( const CTakeDamageInfo &info );
	virtual int MeleeAttack1Conditions( float flDot, float flDist );

	// Muerte
	virtual void Event_Killed( const CTakeDamageInfo &info );

	virtual void PreDeath( const CTakeDamageInfo &info );
	virtual void DeathThink();

	virtual bool CanPlayDeathAnim( const CTakeDamageInfo &info );

	// Velocidad
	virtual float GetIdealSpeed() const;
	virtual float GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );

	// Animaciones
	virtual bool OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );

	// Tareas
	virtual void GatherConditions();

	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	virtual int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
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
	CAI_ClimbBehavior m_nClimbBehavior;

	EHANDLE m_nBlockingEnt;
	float m_flEntBashYaw;
	bool m_bFalling;
};

#endif // AI_BASEMELEE_NPC_H