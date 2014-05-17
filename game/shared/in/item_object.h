//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef ITEM_OBJECT_H
#define ITEM_OBJECT_H

#pragma once

#ifdef CLIENT_DLL
	#include "c_in_player.h"
	#define CItemObject C_ItemObject
#else
	#include "in_player.h"
#endif

//====================================================================
// CItemObject
//
// Define un objeto que debe ser tocado para guardarse en el inventario
// del Jugador
//====================================================================
class CItemObject : public CBaseAnimating
{
public:
	DECLARE_CLASS( CItemObject, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_WCEDIT_POSITION; };

#ifndef CLIENT_DLL
	virtual void Spawn();
	virtual void Precache();

	virtual const char *GetItemModelName() { return NULL; }

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void UseItem( CBasePlayer *pPlayer ) { };

	virtual void Equip( CIN_Player *pPlayer );
	virtual void OnDrop();
#endif // CLIENT_DLL
};

inline CItemObject *ToItemObject( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	return dynamic_cast<CItemObject *>( pEntity );
}

//====================================================================
// Macro para la creación de objetos
//====================================================================

#ifndef CLIENT_DLL

#define CREATE_ITEM_OBJECT( _className, _entityName, _modelName ) \
	class C##_className : public CItemObject \
	{ \
		public: \
			DECLARE_CLASS( CItemBandage, CItemObject ); \
			virtual const char *GetItemModelName() { return _modelName; } \
			virtual void UseItem( CBasePlayer *pPlayer ); \
	};\
	LINK_ENTITY_TO_CLASS( _entityName, C##_className );

#else

#define CREATE_ITEM_OBJECT( _className, _entityName, _modelName ) \
	class C##_className : public CItemObject \
	{ \
		public: \
			DECLARE_CLASS( CItemBandage, CItemObject ); \
	};\
	LINK_ENTITY_TO_CLASS( _entityName, C##_className );

#endif // CLIENT_DLL

#endif