//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AI_BEHAVIOR_CLIMB_H
#define AI_BEHAVIOR_CLIMB_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior.h"

//====================================================================
// Permite escalar muros y paredes
//====================================================================
class CAI_ClimbBehavior : public CAI_SimpleBehavior
{
public:
	DECLARE_CLASS( CAI_ClimbBehavior, CAI_SimpleBehavior );
	DECLARE_DATADESC();

	CAI_ClimbBehavior();

	virtual const char *GetName() {	return "Climb Walls"; }
	virtual bool IsClimbing() { return m_bIsClimbing; }

	virtual bool HitsWall( float flDistance, int iHeight );
	virtual bool HitsWall( float flDistance, int iHeight, trace_t *trace  );

	virtual Activity GetClimbActivity( int iHeight );

	virtual bool CanClimb();
	virtual void UpdateClimb();

	virtual void MoveClimbStart();
	virtual void MoveClimbStop();
	virtual void MoveClimbExecute();

	virtual void GatherConditions();

	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	virtual int	SelectSchedule();

	enum
	{
		COND_CLIMB_START = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,

		TASK_CLIMB_START = BaseClass::NEXT_TASK,
		NEXT_TASK,

		SCHED_CLIMB_START = BaseClass::NEXT_SCHEDULE,
		NEXT_SCHEDULE,
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

protected:
	int m_iClimbHeight;
	float m_flClimbYaw;

	bool m_bIsClimbing;
	float m_flNextClimb;
};

#endif // AI_BEHAVIOR_CLIMB_H