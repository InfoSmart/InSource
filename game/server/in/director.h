//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

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
extern ConVar director_max_common_alive;

extern ConVar director_min_distance;
extern ConVar director_max_distance;
extern ConVar director_danger_distance;

extern ConVar director_max_boss_alive;
extern ConVar director_no_boss;

extern ConVar director_spawn_outview;
extern ConVar director_allow_sleep;
extern ConVar director_mode;

extern ConVar director_spawn_ambient_interval;
extern ConVar director_check_unreachable;

extern ConVar director_gameover;
extern ConVar director_gameover_wait;

//====================================================================
// Macros
//====================================================================

#define PRINT_DEBUG		director_debug.GetBool()

#define MAX_ALIVE		director_max_common_alive.GetInt()
#define GAMEOVER_WAIT	director_gameover_wait.GetInt()

#define MIN_DISTANCE	director_min_distance.GetInt()
#define MAX_DISTANCE	director_max_distance.GetInt()
#define DANGER_DISTANCE director_danger_distance.GetInt()

#define MAX_BOSS_ALIVE	director_max_boss_alive.GetInt()
#define NO_BOSS			director_no_boss.GetBool()

#define SPAWN_OUTVIEW	director_spawn_outview.GetBool()
#define ALLOW_SLEEP		director_allow_sleep.GetBool()

#define SPAWN_AMBIENT_INTERVAL	director_spawn_ambient_interval.GetInt()
#define CHECK_UNREACHABLE		director_check_unreachable.GetBool()
#define ALLOW_GAMEOVER			director_gameover.GetBool()

//====================================================================
// >> CDir_Gamemode
//
// Interfaz para la creación de modos de juego para el
// Director
//====================================================================
abstract_class CDR_Gamemode
{
public:
	virtual void Work() = 0;

	virtual void OnSetStatus( DirectorStatus iStatus ) { }
	virtual void OnSetPhase( DirectorPhase iPhase, int iValue = 0 ) { }

	virtual bool CanSpawnChilds() { return true; }
	virtual bool CanSpawnBoss() { return true; }

	virtual void OnSpawnChild( CBaseEntity *pEntity ) { }
	virtual void OnSpawnBoss( CBaseEntity *pEntity ) { }
	virtual void OnSpawnSpecial( CBaseEntity *pEntity ) { }
	virtual void OnSpawnAmbient( CBaseEntity *pEntity ) { }

	virtual void OnPlayerHurt( CBasePlayer *pPlayer ) { }
	virtual void OnPlayerKilled( CBasePlayer *pPlayer ) { }
};

//====================================================================
// >> CDirector
//====================================================================
class CDirector : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS( CDirector, CAutoGameSystemPerFrame );

	// Devuelve si el Director se encuentra en un estado
	virtual bool IsStatus( int iStatus ) 
	{ 
		return m_iStatus == (DirectorStatus)iStatus; 
	}

	// Devuelve si el Director se encuentra en una fase
	virtual bool IsPhase( int iPhase ) 
	{ 
		return m_iPhase == (DirectorPhase)iPhase; 
	}

	// Devuelve si el Director esta en en el nivel de enojo
	virtual bool IsAngry( int iAngry )
	{
		return m_iAngry == (DirectorAngry)iAngry;
	}

	// Devuelve el equipo bueno
	virtual int GetTeamGood()
	{
		return TEAM_HUMANS;
	}

	// Devuelve el equipo malo
	virtual int GetTeamEvil()
	{
		return TEAM_INFECTED;
	}

	CDirector();

	// Devolución
	virtual int InDangerZone() { return m_iCommonInDangerZone; }
	virtual int ChildsVisibles() { return m_iCommonVisibles; }
	virtual int GetLastAttackTime() { return m_iLastAttackTime; }
	
	// Principales
	virtual bool Init();
	virtual void SetupGamemodes();

	virtual void NewMap();
	virtual void NewCampaign();
	
	virtual void PrepareVM();
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

	// Calculos/Verificación
	virtual void RestartMap();
	virtual bool IsGameover();
	virtual void UpdateAngry();

	virtual bool IsMode( int iStatus );

	// Depuración
	virtual const char *GetStatusName( int iStatus = ST_INVALID );
	virtual const char *GetPhaseName( int iPhase = PH_INVALID );
	virtual const char *GetAngryName( int iStatus = ANGRY_INVALID );
	virtual const char *GetStatsName( int iStatus = 0 );

	virtual void PrintDebugInfo();

	// Utilidades
	virtual CDR_Gamemode *GameMode();

	virtual float MinDistance();
	virtual float MaxDistance();

	virtual bool IsTooFar( CBaseEntity *pEntity );
	virtual bool IsTooClose( CBaseEntity *pEntity );

	virtual void SetPopulation( const char *pName ) 
	{
		m_hPopulation = AllocPooledString(pName); 
	}

	virtual void KillChild( CBaseEntity *pEntity );

	// Eventos
	virtual void OnSpawnChild( CBaseEntity *pEntity );
	virtual void OnSpawnBoss( CBaseEntity *pEntity );
	virtual void OnSpawnSpecial( CBaseEntity *pEntity );
	virtual void OnSpawnAmbient( CBaseEntity *pEntity );

	virtual void OnPlayerHurt( CBasePlayer *pPlayer );
	virtual void OnPlayerKilled( CBasePlayer *pPlayer );

	// Estados
	virtual bool CanSetStatus( DirectorStatus iNewStatus );
	virtual bool CanSetPhase( DirectorPhase iNewPhase );

	virtual void SetStatus( DirectorStatus iStatus );
	virtual void SetPhase( DirectorPhase iPhase, int iValue = 0 );

	virtual void StartCombat( CBaseEntity *pActivator = NULL, int iWaves = 2, bool bInfinite = false );

	// Hijos
	virtual int MaxChilds();
	virtual int GetChildType( CBaseEntity *pEntity );

	virtual void FreeChildSlot( CBaseEntity *pEntity );
	virtual void FreeChildSlot( int iSlot, bool bRemove = false );
	virtual int AddChildSlot( CBaseEntity *pEntity );

	virtual void CheckChilds();
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
	virtual void RestartCountdownBoss();
	virtual bool CanSpawnBoss();
	virtual void HandleBoss();
	virtual void TryBoss();

	// Especiales
	virtual bool CanSpawnSpecial();
	virtual void HandleSpecials();

public:
	bool m_bDisabled;
	CDR_Gamemode *m_pGameModes[ LAST_GAME_MODE ];

	// Estado
	DirectorStatus m_iStatus;
	float m_flStatusTime;

	DirectorPhase m_iPhase;
	int m_iPhaseValue;
	float m_flPhaseTime;

	DirectorAngry m_iAngry;
	bool m_bSpawning;
	bool m_bBlockAllStatus;
	int m_iNextWork;
	bool m_bMusicEnabled;

	string_t m_hPopulation;
	int m_iDirectorMode;
	int m_iPlayersDead;
	int m_iLastAttackTime;

	CInfoDirector *m_pInfoDirector;

	// Cronometros
	CountdownTimer m_hLeft4Boss;
	CountdownTimer m_hEndGame;

	// Combate
	int m_iCombatChilds;
	int m_iCombatsCount;
	int m_iCombatWaves;

	IntervalTimer m_nLastCombatDuration;
	int m_iLastCombatSeconds;
	int m_iLastCombatDeaths;

	// Hijos
	int m_iCommonSpawned;
	int m_iCommonAlive;
	int m_iCommonInDangerZone;
	int m_iCommonVisibles;

	int m_iCommonLimit;

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

#ifdef APOCALYPSE
	#include "ap_director.h"
	#include "ap_director_music.h"
#endif

#ifndef CUSTOM_DIRECTOR
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