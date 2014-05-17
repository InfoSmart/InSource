//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_player_inventory.h"

#include "in_gamerules.h"

#ifdef CLIENT_DLL
	#include "cliententitylist.h"
#else
	#include "item_object.h"
#endif

//====================================================================
// Comandos
//====================================================================

//====================================================================
// Información y Red
//====================================================================

LINK_ENTITY_TO_CLASS( player_inventory, CPlayerInventory );

IMPLEMENT_NETWORKCLASS_ALIASED( PlayerInventory, DT_PlayerInventory );

BEGIN_NETWORK_TABLE( CPlayerInventory, DT_PlayerInventory )

#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO(m_nPlayer) ),
	RecvPropArray3( RECVINFO_ARRAY(m_nItemsList), RecvPropInt( RECVINFO(m_nItemsList[0]) ) ),
#else
	SendPropEHandle( SENDINFO(m_nPlayer) ),
	SendPropArray3( SENDINFO_ARRAY3(m_nItemsList), SendPropInt( SENDINFO_ARRAY(m_nItemsList) ) ),
#endif

END_NETWORK_TABLE()

#define FOR_EACH_ITEM for ( int i = 0; i < MAX_INVENTORY_ITEMS; ++i )

//====================================================================
// Creación en el mundo
//====================================================================
void CPlayerInventory::Spawn()
{
	BaseClass::Spawn();	

	// No somos solidos
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	// No tenemos un modelo
	SetModelName( NULL_STRING );

	// Somos invisibles pero transmitimos datos
	AddEffects( EF_NODRAW );
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	// Limpiamos
	Clear();
}

//====================================================================
//====================================================================
void CPlayerInventory::Prepare( CBasePlayer *pPlayer )
{
	m_nPlayer = pPlayer;
}

//====================================================================
// Devuelve la entidad que se encuentra en el Slot del inventario
//====================================================================
CBaseEntity *CPlayerInventory::GetItem( int iSlot )
{
	int index = GetItemIndex( iSlot );

	if ( index <= -1 )
		return NULL;

#ifndef CLIENT_DLL
	if ( !INDEXENT( index ) )
		return NULL;

	if ( !INDEXENT( index )->GetUnknown() )
		return NULL;

	return INDEXENT( index )->GetUnknown()->GetBaseEntity();
#else
	return ClientEntityList().GetBaseEntity( index );
#endif
}

//====================================================================
// Devuelve la ENTINDEX de la entidad que se encuentra en el Slot del
// inventario
//====================================================================
int CPlayerInventory::GetItemIndex( int iSlot )
{
	return m_nItemsList.Get( iSlot );
}

//====================================================================
// Devuelve si la entidad se encuentra en el inventario
//====================================================================
bool CPlayerInventory::HasItem( CBaseEntity *pItem )
{
	return HasItem( pItem->entindex() );
}

//====================================================================
// Devuelve si la ENTINDEX se encuentra en el inventario
//====================================================================
bool CPlayerInventory::HasItem( int iEdict )
{
	FOR_EACH_ITEM
	{
		if ( GetItemIndex(i) == iEdict )
			return true;
	}

	return false;
}

//====================================================================
//====================================================================
void CPlayerInventory::Clear()
{
	FOR_EACH_ITEM
	{
		m_nItemsList.Set( i, -1 );
	}
}

#ifndef CLIENT_DLL

//====================================================================
//====================================================================
void CPlayerInventory::Drop( CBaseEntity *pItem )
{
	Drop( pItem->entindex() );
}

//====================================================================
//====================================================================
void CPlayerInventory::Drop( int iEdict )
{
	// No tenemos este objeto
	if ( !HasItem(iEdict) )
		return;

	FOR_EACH_ITEM
	{
		// Lo soltamos
		if ( GetItemIndex(i) == iEdict )
		{
			DropBySlot( i );
			return;
		}
	}
}

//====================================================================
//====================================================================
void CPlayerInventory::DropBySlot( int iSlot )
{
	CBaseEntity *pItem = GetItem( iSlot );
	
	if ( !pItem )
		return;

	CItemObject *pItemObject = ToItemObject( pItem );

	if ( pItemObject )
	{
		pItemObject->OnDrop();
	}

	{
		CBaseEntity *pItem = GetItem( iSlot );

		if ( pItem )
		{
			DevMsg( "[CPlayerInventory::DropBySlot] Soltando el objeto %s... \n", pItem->GetClassname() );
		}
	}

	m_nItemsList.Set( iSlot, -1 );
}

//====================================================================
//====================================================================
void CPlayerInventory::DropAll()
{
	FOR_EACH_ITEM
	{
		DropBySlot( i );
	}
}

//====================================================================
//====================================================================
bool CPlayerInventory::AddItem( CBaseEntity *pItem )
{
	// No puedo obtener objetos
	if ( !GetPlayer()->IsAllowedToPickupWeapons() )
		return false;

	// No puedo obtener este objeto
	if ( !InRules->CanHaveItem( GetPlayer(), this ) )
		return false;

	DevMsg( "[CPlayerInventory::AddItem] Agregando el objeto %s... \n", pItem->GetClassname() );
	AddItem( pItem->entindex() );
}

//====================================================================
//====================================================================
bool CPlayerInventory::AddItem( int iEdict )
{
	// Ya tenemos este objeto en el inventario
	if ( HasItem(iEdict) )
		return false;

	FOR_EACH_ITEM
	{
		CBaseEntity *pItem = GetItem( i );

		// Slot ocupado
		if ( pItem )
			continue;

		// El objeto ahora esta en tu inventario
		m_nItemsList.Set( i, iEdict );
		return true;
	}

	// ¡Tu inventario esta lleno!
	return false;
}

//====================================================================
//====================================================================
void CPlayerInventory::UseItem( int iSlot )
{
	CBaseEntity *pItem = GetItem( iSlot );

	if ( !pItem )
		return;

	CItemObject *pItemObject = ToItemObject( pItem );

	if ( pItemObject )
	{
		pItemObject->UseItem( GetPlayer() );
	}

	{
		DevMsg( "[CPlayerInventory::UseItem] Usando el objeto %s ...", pItem->GetClassname() );
	}

	// TODO
}

//====================================================================
//====================================================================
void CPlayerInventory::UseItem( CBaseEntity *pItem )
{
	// No tenemos el objeto en el inventario
	if ( !HasItem(pItem) )
		return;

	FOR_EACH_ITEM
	{
		if ( pItem->entindex() != GetItemIndex(i) )
			continue;

		UseItem( i );
		return;
	}
}

//====================================================================
//====================================================================
int CPlayerInventory::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//====================================================================
// Crea un sonido por capa
//====================================================================
CPlayerInventory *CreatePlayerInventory( CBasePlayer *pPlayer )
{
	CPlayerInventory *pInventory = (CPlayerInventory *)CreateEntityByName( "player_inventory" );

	DispatchSpawn( pInventory );
	pInventory->Activate();

	pInventory->Prepare( pPlayer );

	return pInventory;
}

#endif