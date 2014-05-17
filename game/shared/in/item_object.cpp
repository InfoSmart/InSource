//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "item_object.h"

#include "in_gamerules.h"

//====================================================================
// Comandos
//====================================================================

//====================================================================
// Información y Red
//====================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( ItemObject, DT_ItemObject );

BEGIN_NETWORK_TABLE( CItemObject, DT_ItemObject )
END_NETWORK_TABLE()

#ifndef CLIENT_DLL

//====================================================================
// Creación en el mundo
//====================================================================
void CItemObject::Spawn()
{
	BaseClass::Spawn();

	Precache();

	SetModel( GetItemModelName() );
	SetNetworkQuantizeOriginAngAngles( true );

	// Movimiento
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolid( SOLID_BBOX );
	SetBlocksLOS( false );
	//AddEFlags( EFL_NO_ROTORWASH_PUSH );

	// Colisiones con otros objetos
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	// Iván: ¿Esto para que es?
	Vector vecOrigin = GetAbsOrigin();
	QAngle angAngles = GetAbsAngles();

	NetworkQuantize( vecOrigin, angAngles );

	SetAbsOrigin( vecOrigin );
	SetAbsAngles( angAngles );
}

//====================================================================
// Guardamos en caché
//====================================================================
void CItemObject::Precache()
{
	PrecacheModel( GetItemModelName() );
}

//====================================================================
// Algo nos intenta usar
//====================================================================
void CItemObject::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Solo los Jugadores
	if ( !pActivator->IsPlayer() )
		return;

	CIN_Player *pPlayer = ToInPlayer( pActivator );

	if ( !pPlayer )
		return;

	// Can I even pick stuff up?
	if ( !pPlayer->IsAllowedToPickupWeapons() )
		return;

	// No puedo obtener este objeto
	if ( !InRules->CanHaveItem( pPlayer, this ) )
		return;

	Equip( pPlayer );
}

//====================================================================
// Nos equipamos en el inventario de un Jugador
//====================================================================
void CItemObject::Equip( CIN_Player *pPlayer )
{
	// No tiene inventario...
	if ( !pPlayer->GetInventory() )
		return;

	// Nos agregamos al inventario del Jugador
	bool bResult = pPlayer->GetInventory()->AddItem( this );

	// Estamos en el inventario del Jugador
	if ( bResult )
	{
		SetAbsVelocity( vec3_origin );
		FollowEntity( pPlayer );
		SetOwnerEntity( pPlayer );

		SetMoveType( MOVETYPE_NONE );
		SetSolid( SOLID_NONE );
		AddSolidFlags( FSOLID_TRIGGER );
		SetCollisionGroup( COLLISION_GROUP_NONE );

		AddEffects( EF_NODRAW );

		SetUse( NULL );
		SetThink( NULL );

		VPhysicsDestroyObject();
	}
}

//====================================================================
// El Jugador nos ha soltado de su inventario
//====================================================================
void CItemObject::OnDrop()
{
	FollowEntity( NULL );
	SetOwnerEntity( NULL );
	SetParent( NULL );

	int nSolidFlags = GetSolidFlags() | FSOLID_NOT_STANDABLE;

	if ( VPhysicsInitNormal( SOLID_VPHYSICS, nSolidFlags, false ) == NULL )
	{
		SetSolid( SOLID_BBOX );
		AddSolidFlags( nSolidFlags );

		// If it's not physical, drop it to the floor
		if ( UTIL_DropToFloor(this, MASK_SOLID) == 0 )
		{
			Warning( "Item %s fell out of level at %f,%f,%f\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);

			UTIL_Remove( this );
			return;
		}
	}

	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	if ( IsEffectActive(EF_NODRAW) )
	{
		RemoveEffects( EF_NODRAW );
		DoMuzzleFlash();
	}

	SetUse( &CItemObject::Use );
}
#endif

//====================================================================
// OBJETOS
//====================================================================

CREATE_ITEM_OBJECT( ItemBandage, item_bandage, "models\\props\\cs_office\\cardboard_box02.mdl" );

#ifndef CLIENT_DLL

void CItemBandage::UseItem( CBasePlayer *pPlayer )
{
	CIN_Player *pInPlayer = ToInPlayer( pPlayer );

	if ( !pInPlayer )
		return;

	pInPlayer->GetInventory()->Drop( this );
	UTIL_Remove( this );
}

#endif