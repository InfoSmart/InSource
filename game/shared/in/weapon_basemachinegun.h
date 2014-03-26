//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef WEAPON_BASEMACHINEGUN_H
#define WEAPON_BASEMACHINEGUN_H

#pragma once

#include "weapon_inbase.h"

#define	KICK_MIN_X			0.2f	//Degrees
#define	KICK_MIN_Y			0.2f	//Degrees
#define	KICK_MIN_Z			0.1f	//Degrees

#ifdef CLIENT_DLL
	#define CBaseWeaponMachineGun C_BaseWeaponMachineGun
#endif

//==============================================
// >> CBaseWeaponMachineGun
//
// Base para todas las metralletas.
//==============================================
class CBaseWeaponMachineGun : public CBaseInWeapon
{
public:
	DECLARE_CLASS( CBaseWeaponMachineGun, CBaseInWeapon );
	DECLARE_NETWORKCLASS();
	DECLARE_ACTTABLE();

#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#endif

	CBaseWeaponMachineGun();

	virtual void ItemPostFrame();

	// Disparo y Ataque
	virtual bool CanPrimaryAttack();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();

	// Animaciones
	virtual Activity GetPrimaryAttackActivity();

	// Utils
	virtual bool Deploy();
	virtual bool Reload();

	virtual void AddViewKick();

protected:

	int iShotsFired;

private:
	CBaseWeaponMachineGun( const CBaseWeaponMachineGun & );
};

//==============================================
// Macro para crear una metralleta sencilla.
//==============================================

#define DEFINE_MACHINEGUN( _className, _entityName, _actBaseFrom, _weaponID ) \
	class C##_className : public CBaseWeaponMachineGun{ \
	public: \
		DECLARE_CLASS( C##_className, CBaseWeaponMachineGun ); \
		DECLARE_NETWORKCLASS( ); \
		DECLARE_PREDICTABLE( ); \
		DECLARE_ACTTABLE(); \
		C##_className( ) { }\
		virtual int GetWeaponID() const { return _weaponID; } \
	private: \
		C##_className( const C##_className & ); \
	}; \
	IMPLEMENT_NETWORKCLASS_ALIASED( _className, DT_##_className ); \
	BEGIN_NETWORK_TABLE( C##_className, DT_##_className ) \
	END_NETWORK_TABLE() \
	BEGIN_PREDICTION_DATA( C##_className ) \
	END_PREDICTION_DATA( ) \
	LINK_ENTITY_TO_CLASS( weapon_##_entityName, C##_className ); \
	PRECACHE_WEAPON_REGISTER( weapon_##_entityName ); \
	IMPLEMENT_ACTTABLE_FROM( C##_className, _actBaseFrom );
	

#endif // WEAPON_BASEMACHINEGUN_H