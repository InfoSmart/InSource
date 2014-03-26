//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_GAMERULES_H
#define IN_GAMERULES_H

#pragma once

#include "gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "voice_gamemgr.h"

#include "in_shareddefs.h"
#include "in_viewvectors.h"

#ifdef CLIENT_DLL
	#define CInGameRules C_InGameRules
	#define CInGameRulesProxy C_InGameRulesProxy
#endif


#ifdef APOCALYPSE
	class CAPGameRules;
	class CAP_SurvivalGameRules;
#endif

//=========================================================
// Comandos
//=========================================================

extern ConVar player_time_prone;
extern ConVar player_sprint_penalty;

extern ConVar player_walk_speed;
extern ConVar player_sprint_speed;
extern ConVar player_prone_speed;

extern ConVar player_pushaway_interval;
extern ConVar sv_showimpacts;

extern ConVar flashlight_weapon;
extern ConVar flashlight_realistic;

//=========================================================
// >> CInGameRulesProxy
//=========================================================
class CInGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CInGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

//=========================================================
//=========================================================
#ifndef CLIENT_DLL
class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		// Dead players can only be heard by other dead team mates
		if ( pTalker->IsAlive( ) == false )
		{
			if ( pListener->IsAlive( ) == false )
				return (pListener->InSameTeam( pTalker ));

			return false;
		}

		return (pListener->InSameTeam( pTalker ));
	}
};
#endif

//=========================================================
// >> CInGameRules
//=========================================================
class CInGameRules : public CGameRules
{
public:
	DECLARE_CLASS( CInGameRules, CGameRules );

	CInGameRules();
	~CInGameRules();
	
	// Source Legacy
	virtual bool IsDeathmatch() { return false; }
	virtual bool IsCoOp() { return false; }

	virtual bool HasDirector() { return true; }		// ¿Usa al director?
	virtual bool IsMultiplayer() { return false; }	// ¿Es una partida multijugador?

	// Modo de Juego
	virtual int GameMode() 
	{ 
		return GAME_MODE_NONE; 
	}

	virtual bool IsGameMode( int iMode ) 
	{ 
		return GameMode() == iMode; 
	}

	#ifdef APOCALYPSE
		virtual CAPGameRules *ApPointer() { return NULL; }
		virtual CAP_SurvivalGameRules *ApSurvivalPointer() { return NULL; }
	#endif

	// ¿Es una partida Survival Time? - Teamplay
	virtual bool IsSurvivalTime() 
	{ 
		return IsGameMode( GAME_MODE_SURVIVAL_TIME ); 
	}

	// ¿Es una partida Survival? - Deathmatch
	virtual bool IsSurvival() 
	{ 
		return IsGameMode( GAME_MODE_SURVIVAL ); 
	}

	// ¿Es una partida Coop? - Coop
	virtual bool IsCoop() 
	{ 
		return IsGameMode( GAME_MODE_COOP ); 
	}

	// Modo de Spawn
	virtual int SpawnMode() { return SPAWN_MODE_RANDOM; }
	virtual bool IsSpawnMode( int iMode ) { return SpawnMode() == iMode; }

	// Vectores de vista
	const CViewVectors* GetViewVectors() const;
	const InViewVectors* GetInViewVectors() const;

	virtual const char *GetGameDescription();
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

	// Damage query implementations.
	virtual bool	Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	virtual bool	Damage_ShouldGibCorpse( int iDmgType );		// Damage types that gib the corpse.
	virtual bool	Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	virtual bool	Damage_NoPhysicsForce( int iDmgType );		// Damage types that don't have to supply a physics force & position.
	virtual bool	Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP: These will go away once DamageTypes become enums.
	virtual int		Damage_GetTimeBased( void );
	virtual int		Damage_GetShouldGibCorpse( void );
	virtual int		Damage_GetShowOnHud( void );
	virtual int		Damage_GetNoPhysicsForce( void );
	virtual int		Damage_GetShouldNotBleed( void );

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();
#else
	DECLARE_SERVERCLASS_NOBASE();

	virtual void LoadConfig();

	virtual void Precache();
	virtual void Think();

	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void ClientDestroy( CBasePlayer *pPlayer );

	virtual void InitDefaultAIRelationships();
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	virtual void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, bool bIgnoreWorld = false );
	virtual bool DamageCanMakeSlow( const CTakeDamageInfo &info );

	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );

	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );
	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual bool FPlayerCanDejected( CBasePlayer *pPlayer );

	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );

	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual bool FPlayerCanRespawnNow( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );

	virtual void InitHUD( CBasePlayer *pPlayer );
	virtual bool AllowAutoTargetCrosshair();

	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor );
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );

	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual bool UseSuicidePenalty() { return true; }

	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem );
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount );

	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon );
	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon ) { return 0.0f; }
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon ) { return 0.0f; }
	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon ) { return vec3_origin; }

	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem ) { return 0.0f; }
	virtual Vector VecItemRespawnSpot( CItem *pItem ) { return vec3_origin; }
	virtual QAngle VecItemRespawnAngles( CItem *pItem ) { return vec3_angle; }

	virtual float FlHealthChargerRechargeTime() { return 0.0f; }
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity );

	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

	virtual CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	virtual const char *GetTeamID( CBaseEntity *pEntity ) { return ""; }
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker );
	virtual bool PlayFootstepSounds( CBasePlayer *pPlayer );

	virtual bool FAllowFlashlight();
	virtual bool FAllowNPCs();

	virtual bool IsSpawnFree( CBaseEntity *pSpawn );
	virtual void AddSpawnSlot( CBaseEntity *pSpawn );
	virtual void RemoveSpawnSlot( CBaseEntity *pSpawn );

	virtual bool CanPlayDeathAnim( CBaseEntity *pEntity, const CTakeDamageInfo &info );
	virtual bool CanPlayDeathAnim( const CTakeDamageInfo &info );
#endif

protected:
	CUtlVector<int> hSpawnSlots;
};

inline CInGameRules *InGameRules()
{
	return static_cast<CInGameRules *>(g_pGameRules);
}

extern CInGameRules *InRules;

#endif //IN_GAMERULES_H