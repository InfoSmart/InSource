//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_PLAYER_INVENTORY_H
#define IN_PLAYER_INVENTORY_H

#pragma once

#ifdef CLIENT_DLL
	#define CPlayerInventory C_PlayerInventory
#endif

#define MAX_INVENTORY_ITEMS 50

#endif // IN_PLAYER_INVENTORY_H

//====================================================================
// CPlayerInventory
//
// >> Administra el inventario de cada Jugador
//
// Iván: Por ahora lo haremos así, pero que sea una entidad será muy
// costosa para el motor. 32 = Jugadores y 32 = Inventarios
//====================================================================
class CPlayerInventory : public CBaseEntity
{
public:
	DECLARE_CLASS( CPlayerInventory, CBaseEntity );
	DECLARE_NETWORKCLASS();

	virtual void Spawn();
	virtual void Prepare( CBasePlayer *pPlayer );

	virtual CBaseEntity *GetItem( int iSlot );
	virtual int GetItemIndex( int iSlot );

	virtual bool HasItem( CBaseEntity *pItem );
	virtual bool HasItem( int iEdict );

	virtual void Clear();

	virtual CBasePlayer *GetPlayer() { return m_nPlayer.Get(); }

#ifndef CLIENT_DLL
	virtual void Drop( CBaseEntity *pItem );
	virtual void Drop( int iEdict );

	virtual void DropBySlot( int iSlot );
	virtual void DropAll();

	virtual bool AddItem( CBaseEntity *pItem );
	virtual bool AddItem( int iEdict );

	virtual void UseItem( int iSlot );
	virtual void UseItem( CBaseEntity *pItem );

	virtual int UpdateTransmitState();
#endif

protected:
	CNetworkHandle( CBasePlayer, m_nPlayer );
	CNetworkArray( int, m_nItemsList, MAX_INVENTORY_ITEMS );

};

#ifndef CLIENT_DLL
	CPlayerInventory *CreatePlayerInventory( CBasePlayer *pPlayer );
#endif