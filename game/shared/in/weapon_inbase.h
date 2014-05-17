//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef WEAPON_INBASE_H
#define WEAPON_INBASE_H

#pragma once

#include "in_shareddefs.h"
#include "in_weapon_parse.h"

#ifdef CLIENT_DLL
	#define CWeaponInBase C_WeaponInBase
#endif

class CIN_Player;

//==============================================
// Comandos
//==============================================

extern ConVar weapon_infiniteammo;
extern ConVar weapon_automatic_reload;

extern ConVar weapon_autofire;
extern ConVar sk_plr_dmg_grenade;

//====================================================================
// Macros
//====================================================================

//{ ACT_MP_SWIM, ACT_HL2MP_SWIM_##_baseAct##, false }, \
	//{ ACT_MP_SWIM_IDLE, ACT_HL2MP_SWIM_IDLE_##_baseAct##, false }, \

#define IMPLEMENT_ACTTABLE_FROM( _weaponClass, _baseAct ) \
	acttable_t _weaponClass::m_acttable[] = { \
	{ ACT_MP_STAND_IDLE, ACT_IDLE_##_baseAct##, false }, \
	{ ACT_MP_WALK, ACT_WALK_##_baseAct##, false }, \
	{ ACT_MP_RUN, ACT_RUN_##_baseAct##, false }, \
	{ ACT_MP_CROUCH_IDLE, ACT_CROUCHIDLE_##_baseAct##, false }, \
	{ ACT_MP_CROUCHWALK, ACT_RUN_CROUCH_##_baseAct##, false }, \
	{ ACT_MP_JUMP, ACT_JUMP_##_baseAct##, false }, \
	{ ACT_MP_JUMP_START, ACT_JUMP_##_baseAct##, false }, \
	{ ACT_MP_AIRWALK, ACT_IDLE_##_baseAct##, false }, \
		\
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_PRIMARYATTACK_##_baseAct##, false }, \
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_PRIMARYATTACK_##_baseAct##, false }, \
	{ ACT_MP_RELOAD_STAND, ACT_RELOAD_##_baseAct##, false }, \
	{ ACT_MP_RELOAD_CROUCH, ACT_RELOAD_##_baseAct##, false }, \
};\
	IMPLEMENT_ACTTABLE( _weaponClass );

/*#define IMPLEMENT_ACTTABLE_FROM( _weaponClass, _baseAct ) \
	acttable_t _weaponClass::m_acttable[] = { \
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_##_baseAct##, false }, \
	{ ACT_MP_WALK, ACT_HL2MP_WALK_##_baseAct##, false }, \
	{ ACT_MP_RUN, ACT_HL2MP_RUN_##_baseAct##, false }, \
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_##_baseAct##, false }, \
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_##_baseAct##, false }, \
	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_##_baseAct##, false }, \
	{ ACT_MP_JUMP_START, ACT_HL2MP_JUMP_##_baseAct##, false }, \
	{ ACT_MP_AIRWALK, ACT_HL2MP_RUN_##_baseAct##, false }, \
	{ ACT_MP_SWIM, ACT_HL2MP_SWIM_##_baseAct##, false }, \
	{ ACT_MP_SWIM_IDLE, ACT_HL2MP_SWIM_IDLE_##_baseAct##, false }, \
		\
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_##_baseAct##, false }, \
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_##_baseAct##, false }, \
	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_##_baseAct##, false }, \
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_##_baseAct##, false }, \
};\
	IMPLEMENT_ACTTABLE( _weaponClass );

#define IMPLEMENT_ACTTABLE_NPC_FROM( _weaponClass, _baseAct ) \
	acttable_t _weaponClass::m_acttable[] = { \
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_##_baseAct##, true }, \
	{ ACT_RELOAD, ACT_RELOAD_##_baseAct##, true }, \
	{ ACT_IDLE, ACT_IDLE_##_baseAct##, true }, \
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_##_baseAct##, true }, \
	{ ACT_WALK, ACT_WALK_RIFLE, true }, \
	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true }, \
	{ ACT_IDLE_RELAXED, ACT_IDLE_##_baseAct##_RELAXED, false }, \
	{ ACT_IDLE_STIMULATED, ACT_IDLE_##_baseAct##_STIMULATED, false }, \
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_##_baseAct##, false }, \
	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false }, \
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, false }, \
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, false }, \
	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false }, \
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, false }, \
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, false }, \
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_##_baseAct##_RELAXED, false }, \
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, false }, \
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_##_baseAct##, false }, \
	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false }, \
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, false }, \
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, false }, \
	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false }, \
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, false }, \
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, false }, \
	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true }, \
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true }, \
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true }, \
	{ ACT_RUN, ACT_RUN_RIFLE, true }, \
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true }, \
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true }, \
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true }, \
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_##_baseAct##, true }, \
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_##_baseAct##_LOW, true }, \
	{ ACT_COVER_LOW, ACT_COVER_##_baseAct##_LOW, false }, \
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_##_baseAct##_LOW, false }, \
	{ ACT_RELOAD_LOW, ACT_RELOAD_##_baseAct##_LOW, false }, \
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_##_baseAct##, true }, \
	\
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_##_baseAct##, false }, \
	{ ACT_MP_WALK, ACT_HL2MP_WALK_##_baseAct##, false }, \
	{ ACT_MP_RUN, ACT_HL2MP_RUN_##_baseAct##, false }, \
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_##_baseAct##, false }, \
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_##_baseAct##, false }, \
	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_##_baseAct##, false }, \
	{ ACT_MP_JUMP_START, ACT_HL2MP_JUMP_##_baseAct##, false }, \
	{ ACT_MP_AIRWALK, ACT_HL2MP_RUN_##_baseAct##, false }, \
	{ ACT_MP_SWIM, ACT_HL2MP_SWIM_##_baseAct##, false }, \
	{ ACT_MP_SWIM_IDLE, ACT_HL2MP_SWIM_IDLE_##_baseAct##, false }, \
	\
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_##_baseAct##, false }, \
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_##_baseAct##, false }, \
	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_##_baseAct##, false }, \
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_##_baseAct##, false }, \
}; \
	IMPLEMENT_ACTTABLE( _weaponClass );*/

//====================================================================
// >> CBaseInWeapon
//
// Base para todas las armas.
//====================================================================
class CBaseInWeapon : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CBaseInWeapon, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	#ifndef CLIENT_DLL
		DECLARE_DATADESC();
	#endif

	CBaseInWeapon();

	bool IsPredicted() const;

	virtual int GetWeaponID()
	{
		return WEAPON_NONE;
	}

	virtual int GetWeaponClass()
	{
		return GetInWpnData().m_iClassification;
	}

	virtual bool InReload()
	{
		return m_bInReload;
	}

	CIN_Player *GetPlayerOwner() const;
	CInWeaponInfo const &GetInWpnData() const;

	virtual Vector ShootPosition();

	// Principales
	virtual void Precache();
	virtual void ItemPostFrame();

	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual void Drop( const Vector &vecVelocity );

	virtual void DefaultTouch( CBaseEntity *pOther );

	// Disparo y Ataque
	virtual bool CanPrimaryAttack();
	virtual bool CanSecondaryAttack();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();

	virtual void PostPrimaryAttack();
	virtual void PostSecondaryAttack();

	virtual void FireBullets();
	virtual void FireBullet();

	virtual void FireGrenade();

	virtual float GetFireRate();
	virtual Vector GetBulletsSpread();
	virtual float GetFireSpread();

	virtual float GetFireKick();

	virtual void SetNextAttack( float flTime = 0.0 );
	virtual void SetNextPrimaryAttack( float flTime = 0.0 );
	virtual void SetNextSecondaryAttack( float flTime = 0.0 );
	virtual void SetNextIdleTime( float flTime = 0.0 );

	// Vista de Hierro
	virtual Vector GetIronsightPosition() const;
	virtual QAngle GetIronsightAngles() const;
	virtual float GetIronsightFOV() const;

	virtual bool HasIronsight() { return true; }
	virtual bool IsIronsighted();
	virtual void ToggleIronsight();
	virtual void EnableIronsight();
	virtual void DisableIronsight();
	virtual void SetIronsightTime();

	// Sonido
	virtual void WeaponSound( WeaponSound_t pType, float flSoundTime = 0.0f );

#ifndef CLIENT_DLL
	virtual void Spawn();
	virtual void SendReloadEvents();
#else
	virtual void OnDataChanged( DataUpdateType_t pType );
	virtual bool ShouldPredict();
#endif

protected:
	bool bIsPrimaryAttackReleased;

	CNetworkVar( bool, m_bIsIronsighted );
	CNetworkVar( float, m_flIronsightedTime );

	friend class CBaseViewModel;

private:
	CBaseInWeapon( const CBaseInWeapon & );
};

//====================================================================
// Devuelve la ID del alias del arma
//====================================================================
inline int AliasToWeaponID( const char *pAlias )
{
	if ( pAlias )
	{
		for ( int i = 0; sWeaponAlias[i] != NULL; ++i )
		{
			if ( !Q_stricmp( sWeaponAlias[i], pAlias ) )
				return i;
		}
	}

	return WEAPON_NONE;
}

//====================================================================
// Devuelve el Alias de la ID del arma
//====================================================================
inline const char *WeaponIDToAlias( int id )
{
	if ( id >= IN_WEAPON_MAX || id < 0 )
		return NULL;

	return sWeaponAlias[id];
}

#endif //WEAPON_INBASE_H