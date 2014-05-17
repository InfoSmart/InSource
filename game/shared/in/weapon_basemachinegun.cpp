//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "weapon_basemachinegun.h"
#include "in_buttons.h"
#include "particle_parse.h"

#ifndef CLIENT_DLL
	#include "in_player.h"
#else
	#include "c_in_player.h"
#endif

//==============================================
// Información y Red
//==============================================

// DT_BaseWeaponMachineGun
IMPLEMENT_NETWORKCLASS_ALIASED( BaseWeaponMachineGun, DT_BaseWeaponMachineGun );

LINK_ENTITY_TO_CLASS( weapon_base_machinegun, CBaseWeaponMachineGun );

IMPLEMENT_ACTTABLE_FROM( CBaseWeaponMachineGun, SMG );

BEGIN_NETWORK_TABLE( CBaseWeaponMachineGun, DT_BaseWeaponMachineGun )
#ifndef CLIENT_DLL
	//SendPropInt( SENDINFO(iShotsFired) ),
#else
	//RecvPropInt( RECVINFO(iShotsFired) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CBaseWeaponMachineGun )
	DEFINE_PRED_FIELD( iShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

//=========================================================
// Constructor
//=========================================================
CBaseWeaponMachineGun::CBaseWeaponMachineGun()
{
	iShotsFired = 0;
}

//=========================================================
// Bucle de ejecución de tareas.
//=========================================================
void CBaseWeaponMachineGun::ItemPostFrame()
{
	CIN_Player *pPlayer = ToInPlayer( GetOwner( ) );

	// Solo los jugadores pueden usar esto
	if ( !pPlayer )
		return;

	BaseClass::ItemPostFrame();

	if ( !pPlayer->IsPressingButton( IN_ATTACK ) )
		iShotsFired = 0;
}

//=========================================================
// Devuelve si el arma puede efectuar un disparo primario.
// ¡Solo llamar desde PrimaryAttack()!
//=========================================================
bool CBaseWeaponMachineGun::CanPrimaryAttack()
{
	return BaseClass::CanPrimaryAttack();
}

//=========================================================
// El jugador desea disparar
//=========================================================
void CBaseWeaponMachineGun::PrimaryAttack()
{
	// 
	++iShotsFired;
	
	// ¡Ahi te van las balas!
	FireBullets();

	// Acciones post
	PostPrimaryAttack();

	// Humo por disparos
	if ( iShotsFired >= 3 )
	{
		DispatchParticleEffect( "weapon_muzzle_smoke", PATTACH_POINT_FOLLOW, GetPlayerOwner()->GetViewModel(), "muzzle", true );
		DispatchParticleEffect( "weapon_muzzle_smoke", PATTACH_POINT_FOLLOW, this, "muzzle", true );
	}
}

//=========================================================
// El jugador desea disparar la munición secundaria
//=========================================================
void CBaseWeaponMachineGun::SecondaryAttack()
{
	// ¡Una granada!
	//FireGrenade();

	// Acciones post
	PostSecondaryAttack();

	// Nos ponemos en "Idle" dentro de 5s
	SetNextIdleTime( 1.0 );
}

//=========================================================
// Devuelve la animación para el ataque primario
//=========================================================
Activity CBaseWeaponMachineGun::GetPrimaryAttackActivity()
{
	if ( iShotsFired <= 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( iShotsFired <= 3 )
		return ACT_VM_RECOIL1;

	if ( iShotsFired <= 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//=========================================================
// Despliega el arma.
//=========================================================
bool CBaseWeaponMachineGun::Deploy()
{
	iShotsFired = 0;
	return BaseClass::Deploy();
}

//=========================================================
// Informa al arma que el jugador desea recargar
//=========================================================
bool CBaseWeaponMachineGun::Reload()
{
	CIN_Player *pPlayer = GetPlayerOwner();

	if ( !pPlayer )
	{
		Warning("[CBaseWeaponMachineGun::Reload] Mi propietario no es un jugador. \n");
		return false;
	}

	int iPlayerAmmo = pPlayer->GetAmmoCount( GetPrimaryAmmoType() );

	// No tenemos munición.
	if ( iPlayerAmmo <= 0 )
		return false;

	// Verificamos si realmente necesitamos recargar.
	// En caso de que si: 
	// - Se ejecuta la animación "ACT_VM_RELOAD" en el Viewmodel (Primera persona)
	// - Se ejecuta la animación "PLAYER_RELOAD" en el modelo del jugador. (Tercera persona)
	// - Se reproducen los sonidos de recarga.
	// - Se pone en "cola" la recarga.
	int iResult = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );

	// Al parecer no.
	if ( !iResult )
		return false;

	iShotsFired = 0;
	return true;
}

//=========================================================
//=========================================================
void CBaseWeaponMachineGun::AddViewKick()
{
	CIN_Player *pPlayer = GetPlayerOwner();

	// Solo los jugadores pueden usar esto.
	if ( !pPlayer )
		return;

	float flPunch = GetFireKick();
	QAngle anglePunch;

	anglePunch.x = -flPunch;
	anglePunch.y = RandomFloat(0.1, 0.5);
	anglePunch.z = RandomFloat(0.1, 0.5);

	pPlayer->ViewPunch( anglePunch );
}