//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "weapon_basemachinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
	#define CWeaponBizon C_WeaponBizon
	#define CWeaponAR15 C_WeaponAR15
	#define CWeaponM4 C_WeaponM4
	#define CWeaponMP5 C_WeaponMP5
	#define CWeaponMRC C_WeaponMRC
	#define CWeaponAK47 C_WeaponAK47
#endif

// PP19 Bizon - weapon_bizon
// http://youtu.be/CzkE_koeyJE
DEFINE_MACHINEGUN( WeaponBizon, bizon, SMG, WEAPON_BIZON );

// AR-15 con Lanzagranadas - weapon_ar15
// http://youtu.be/MhtTHAv0HSc
DEFINE_MACHINEGUN( WeaponAR15, ar15, SMG, WEAPON_AR15 );

// M4 con Lanzagranadas - weapon_m4
// http://youtu.be/7wn51LhCqzI
DEFINE_MACHINEGUN( WeaponM4, m4, SMG, WEAPON_M4 );

// MP5-K - weapon_mp5
// http://youtu.be/mBwj2PrHRKY
DEFINE_MACHINEGUN( WeaponMP5, mp5, SMG, WEAPON_MP5 );

// MR-C - weapon_mrc
// http://es.wikipedia.org/wiki/MR-C
DEFINE_MACHINEGUN( WeaponMRC, mrc, SMG, WEAPON_MRC );

// AK47 - weapon_ak47
DEFINE_MACHINEGUN( WeaponAK47, ak47, SMG, WEAPON_AK47 );