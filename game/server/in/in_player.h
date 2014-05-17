//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_PLAYER_H
#define IN_PLAHER_H

#pragma once

#include "player.h"
#include "server_class.h"
#include "in_playeranimstate.h"

#include "in_player_inventory.h"
#include "layersound.h"

class CBaseInWeapon;

//====================================================================
// Comandos
//====================================================================

extern ConVar playermodel_sp;

extern ConVar respawn_afterdeath;
extern ConVar wait_deathcam;

extern ConVar stamina_drain;
extern ConVar stamina_infinite;

extern ConVar health_regeneration;
extern ConVar health_regeneration_recover;
extern ConVar health_regeneration_interval;

extern ConVar climb_height_check;
extern ConVar weapon_single;

//====================================================================
//====================================================================

static const char *m_nPlayerModels[] =
{
	"models/survivors/survivor_kat.mdl"
};

//====================================================================
// >> CTEPlayerAnimEvent
//====================================================================
class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
	{ }

	CNetworkHandle( CBasePlayer, hPlayer );
	CNetworkVar( int, iEvent );
	CNetworkVar( int, nData );
};

//====================================================================
// >> CIN_Player
//====================================================================
class CIN_Player : public CBasePlayer
{
public:
	DECLARE_CLASS( CIN_Player, CBasePlayer );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CIN_Player();
	~CIN_Player();

	virtual int MyDefaultTeam() 
	{ 
		return TEAM_HUMANS; 
	}

	virtual bool IsInGround()
	{
		return (GetFlags() & FL_ONGROUND) ? true : false;
	}

	virtual bool IsGod() 
	{ 
		return (GetFlags() & FL_GODMODE) ? true : false; 
	}

	// Estaticos
	static CIN_Player *CreatePlayer( const char *pClassName, edict_t *pEdict, const char *pPlayerName );

	// Utils
	virtual void CreateAnimationState();

	virtual bool IsPressingButton( int iButton );
	virtual bool IsButtonPressed( int iButton );
	virtual bool IsButtonReleased( int iButton );

	virtual const char *GetConVar( const char *pName );
	virtual void ExecCommand( const char *pName );

	virtual void SendLesson( const char *pLesson, bool bOnce = false, CBaseEntity *pSubject = NULL );

	// Principales
	virtual int UpdateTransmitState();
	virtual void InitialSpawn();

	virtual void Spawn();
	virtual void SpawnMultiplayer();

	virtual void Precache();
	virtual void PrecacheMultiplayer();

	virtual void PreThink();
	virtual void PostThink();

	// Selección
	virtual void PlayerUse();

	// Modelos
	virtual const char *GetPlayerModel();
	virtual const char* GetPlayerModelValidated();
	virtual bool IsValidModel( const char *pModel = NULL );

	// Sonidos
	virtual void PrepareMusic();
	virtual void StopMusic();

	virtual void EmitPlayerSound( const char *pSoundName );
	virtual void StopPlayerSound( const char *pSoundName );

	virtual const char *GetPlayerType();

	virtual void PainSound( const CTakeDamageInfo &info );
	virtual void DyingSound();
	virtual void DieSound();

	virtual void UpdateSounds();

	// Linterna
	virtual bool FlashlightTurnOn( bool playSound = true );
	virtual void FlashlightTurnOff( bool playSound = true );
	virtual int FlashlightIsOn();

	// Armas/Ataque
	virtual CBaseInWeapon *GetActiveInWeapon() const; // SHARED!
	virtual void OnFireBullets( int iBullets ); // SHARED!

	virtual CBaseInWeapon *GetPrimaryWeapon();
	virtual CBaseInWeapon *GetSecondaryWeapon();

	virtual void Weapon_Equip( CBaseCombatWeapon *pWeapon );
	virtual void Weapon_DropClass( int iWeaponClass );

	virtual bool InCombat() { return m_bInCombat; }
	virtual bool IsUnderAttack() { return m_bUnderAttack; }

	// Salud, muerte y daño.
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	virtual void Event_Killed( const CTakeDamageInfo &info );

	virtual void PlayerDeathThink();
	virtual void PlayerDeathPostThink();

	virtual void CreateRagdollEntity( const CTakeDamageInfo &info );

	virtual float GetStamina() { return m_flStamina; }

	virtual void UpdateStamina();
	virtual void UpdateHealthRegeneration();
	virtual void UpdateTiredEffects();
	virtual void UpdateFallEffects();
	virtual void UpdateFallToHell();

	// Incapacitación
	virtual bool IsIncap() { return m_bIncap; }
	virtual bool IsClimbingIncap() { return m_bClimbingToHell; }
	virtual bool IsWaitingGroundDeath() { return m_bWaitingGroundDeath; }

	virtual void StartIncap( bool bClimbing = false );
	virtual void StopIncap();

	virtual void OnStartIncap() { }
	virtual void OnStopIncap() { }

	virtual void StartClimbIncap();
	virtual void StopClimbIncap();

	virtual void UpdateIncap();

	virtual void HelpIncap( CIN_Player *pPlayer );
	virtual float GetHelpProgress() { return m_flHelpProgress; }
	virtual float GetClimbingHold() { return m_flClimbingHold; }

	// Cordura
	virtual void UpdateSanity();
	virtual void RemoveSanity( float flValue );
	virtual float GetSanity() { return m_flSanity; }

	// Movimiento
	virtual void UpdateSpeed();

	virtual void StartSprint();
	virtual void StopSprint();
	virtual bool IsSprinting() { return m_bIsSprinting; }

	virtual void Jump();

	virtual void OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea );
	virtual void AddLastKnowArea( CNavArea *pArea, bool bAround = true );
	virtual bool InLastKnowAreas( CNavArea *pArea );

	// Modelos
	virtual void CreateViewModel( int iIndex = 0 );

	// Animaciones
	virtual void HandleAnimEvent( animevent_t *pEvent );

	virtual void SetAnimation( PLAYER_ANIM playerAnim  );
	virtual void DoAnimationEvent( PlayerAnimEvent_t pEvent, int nData = 0 );

	virtual Activity TranslateActivity( Activity actBase ); // SHARED!

	// Espectador
	virtual void Spectate( CIN_Player *pTarget = NULL );

	// Inventario
	virtual CPlayerInventory *GetInventory() { return m_nPlayerInventory; }

	// Comandos y cheats
	virtual bool ClientCommand( const CCommand &args );
	virtual void CheatImpulseCommands( int iImpulse );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper );
	virtual void ProcessUsercmds( CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused );

	//virtual void DisableButtons( int iButtons );
	//virtual void EnableButtons( int iButtons );

	virtual void EnableMovement() { m_bMovementDisabled = false; }
	virtual void DisableMovement() { m_bMovementDisabled = true; }

	// Otros
	virtual CBaseEntity *EntSelectSpawnPoint();
	virtual int ObjectCaps();

	virtual void SnapEyeAnglesZ( int iAngle );

public:
	CUtlVector<const char*> m_nLessonsList;

protected:
	CNetworkQAngle( m_angEyeAngles );
	CNetworkHandle( CBaseEntity, m_nRagdoll );

	CInPlayerAnimState *m_nAnimState;
	CBaseEntity *m_nSpawnSpot;
	CNetworkHandle( CPlayerInventory, m_nPlayerInventory );

	//CUtlDict<const char *, int> m_nValidModels;

	bool m_bIsSprinting;
	CNetworkVar( bool, m_bMovementDisabled );

	float m_flStamina;

	CountdownTimer m_nSlowTime;
	CountdownTimer m_nFinishCombat;
	CountdownTimer m_nFinishUnderAttack;

	float m_flSanity;
	
	CNetworkVar( bool, m_bInCombat );
	bool m_bUnderAttack;

	CNetworkVar( bool, m_bIncap );
	CNetworkVar( bool, m_bClimbingToHell );
	CNetworkVar( bool, m_bWaitingGroundDeath );

	CNetworkVar( int, m_iTimesDejected );
	CNetworkVar( float, m_flHelpProgress );
	CNetworkVar( float, m_flClimbingHold );

	float m_flNextPainSound;
	float m_flNextDyingSound;

	int m_iNextRegeneration;
	int m_iNextFadeout;
	int m_iNextEyesHurt;
	int m_iNextDejectedHurt;

	CNetworkVar( int, m_iEyeAngleZ );

	CTakeDamageInfo m_nLastDamageInfo;
	Vector m_vecDeathOrigin;

	CNavArea *m_nLastNavAreas[1005];
	int m_iLastNavAreaIndex;
	int m_iNextNavCheck;

	friend class CInPlayerAnimState;
};

inline CIN_Player *ToInPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CIN_Player *>( pEntity );
}

#endif //IN_PLAYER_H