//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef DIRECTOR_H
#define DIRECTOR_H

#pragma once

#include "in_shareddefs.h"
#include "directordefs.h"

#include "info_director.h"

//=========================================================
// Comandos de consola
//=========================================================

extern ConVar director_debug;
extern ConVar director_max_alive;

extern ConVar director_min_distance;
extern ConVar director_max_distance;

extern ConVar director_max_boss_alive;
extern ConVar director_danger_distance;

extern ConVar director_spawn_outview;
extern ConVar director_allow_sleep;
extern ConVar director_mode;

extern ConVar director_spawn_ambient_interval;

//=========================================================
// >> CDirector
//=========================================================
class CDirector : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS( CDirector, CAutoGameSystemPerFrame );

	//
	virtual bool Is( int iStatus ) 
	{ 
		return m_iStatus == iStatus; 
	}

	virtual bool IsAngry( DirectorAngry iAngry )
	{
		return m_iAngry == iAngry;
	}

	virtual int TeamGood()
	{
		return TEAM_HUMANS;
	}

	virtual int TeamEvil()
	{
		return TEAM_INFECTED;
	}

	CDirector();

	// Devolución
	virtual int InDangerZone() { return m_iChildsInDangerZone; }
	virtual int ChildsVisibles() { return m_iChildsVisibles; }
	virtual int GetLastAttackTime() { return m_iLastAttackTime; }
	
	// Principales
	virtual bool Init();

	virtual void NewMap();
	virtual void NewCampaign();
	
	virtual void FindInfoDirector();

	virtual void Stop();
	virtual void Shutdown();

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();

	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();

	virtual void FrameUpdatePreEntityThink();
	virtual void FrameUpdatePostEntityThink();

	virtual void Work();
	virtual void ModWork();

	// Modos de juego
	virtual void NormalUpdate();
	virtual void SurvivalUpdate();
	virtual void SurvivalTimeUpdate();

	// Calculos/Verificación
	virtual void CheckGameover();
	virtual void UpdateAngry();
	virtual bool IsMode( int iStatus );

	// Depuración
	virtual const char *GetStatusName( DirectorStatus iStatus = INVALID );
	virtual const char *GetAngryName( DirectorAngry iStatus = ANGRY_INVALID );
	virtual const char *GetStatsName( int iStatus = 0 );

	virtual void PrintDebugInfo();

	// Utilidades
	virtual float MinDistance();
	virtual float MaxDistance();

	virtual void RestartPanic();
	virtual void RestartBoss();

	virtual bool IsTooFar( CBaseEntity *pEntity );
	virtual bool IsTooClose( CBaseEntity *pEntity );

	virtual void SetPopulation( const char *pName ) 
	{
		m_hPopulation = AllocPooledString(pName); 
	}

	virtual void KillChild( CBaseEntity *pEntity );

	// Eventos
	virtual void OnSpawnChild( CAI_BaseNPC *pNPC );
	virtual void OnSpawnBoss( CAI_BaseNPC *pNPC );
	virtual void OnSpawnSpecial( CAI_BaseNPC *pNPC );
	virtual void OnSpawnAmbient( CAI_BaseNPC *pNPC );

	virtual void OnPlayerHurt( CBasePlayer *pPlayer );
	virtual void OnPlayerKilled( CBasePlayer *pPlayer );

	// Estados
	virtual void Set( DirectorStatus iStatus );

	virtual bool CanSleep();
	virtual bool CanRelax();
	virtual bool CanPanic();
	virtual bool CanBoss();
	virtual bool CanClimax();

	virtual bool IsInfinitePanic();

	virtual void Sleep();
	virtual void Relaxed();
	virtual void Panic( CBasePlayer *pCaller = NULL, int iSeconds = 0, bool bInfinite = false );
	virtual void Boss();
	virtual void Climax( bool bMini = false );

	// Hijos
	virtual const char *ForceChildClass();
	virtual int MaxChilds();

	virtual int GetChildType( CBaseEntity *pEntity );

	virtual void ManageChilds();
	virtual void CheckChild( CBaseEntity *pEntity );

	virtual bool CheckDistance( CBaseEntity *pEntity );
	virtual bool CheckUnreachable( CBaseEntity *pEntity );
	virtual bool CheckNewEnemy( CBaseEntity *pEntity );

	virtual bool CanSpawnChilds();

	virtual void HandleAll();
	virtual void HandleChilds();

	// Hijos de ambiente
	virtual bool CanSpawnAmbient();
	virtual void HandleAmbient();

	// Jefes
	virtual bool CanSpawnBoss();
	virtual void HandleBoss();
	virtual void TryBoss();

	// Especiales
	virtual bool CanSpawnSpecial();
	virtual void HandleSpecials();

public:
	bool m_bDisabled;

protected:
	// Estado
	DirectorStatus m_iStatus;
	DirectorAngry m_iAngry;
	float m_flStatusTime;
	bool m_bSpawning;
	bool m_bBlockAllStatus;
	int m_iNextWork;
	bool m_bMusicEnabled;

	string_t m_hPopulation;
	int m_iDirectorMode;
	int m_iPlayersDead;
	int m_iLastAttackTime;

	CInfoDirector *m_pInfoDirector;

	// Modos
	bool m_bSurvivalTimeStarted;
	int m_iSurvivalTime;

	// Cronometros
	CountdownTimer m_hLeft4Boss;
	CountdownTimer m_hLeft4Panic;
	CountdownTimer m_hLeft4FinishPanic;
	CountdownTimer m_hDisabledTime;
	CountdownTimer m_hEnd;
	CountdownTimer m_nPostPanicLimit;

	

	// Pánico
	int m_iPanicChilds;
	int m_iPanicCount;
	bool m_bPanicSuspended;

	IntervalTimer m_nLastPanicDuration;
	int m_iLastPanicDeaths;

	// Hijos
	int m_iSpawnQueue;
	int m_iChildsSpawned;
	int m_iChildsAlive;
	int m_iChildsInDangerZone;
	int m_iChildsVisibles;

	// Ambiente
	int m_iAmbientSpawned;
	int m_iAmbientAlive;
	int m_iNextAmbientSpawn;

	// Especiales
	int m_iSpecialsSpawned;
	int m_iSpecialsAlive;

	// Jefes
	bool m_bBossPendient;
	int m_iBossSpawned;
	int m_iBossAlive;
	int m_iBossKilled;

	int m_iMaxBossAlive;

	friend class CDirectorManager;
	friend class CInfoDirector;
	friend class CAP_DirectorMusic;
};


#ifndef DIRECTOR_CUSTOM
	extern CDirector *Director;
#endif

//=========================================================
// Imprime información de depuración del Director
//=========================================================
inline void Director_StatePrintf( int iLine, const char *pMsg, ... )
{
	// Format the string.
	char str[4096];
	va_list marker;
	va_start( marker, pMsg );
	Q_vsnprintf( str, sizeof( str ), pMsg, marker );
	va_end( marker );

	// Show it with Con_NPrintf.
	engine->Con_NPrintf(iLine, "%s", str);
}

#endif // DIRECTOR_H