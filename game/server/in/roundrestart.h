#ifndef ROUNDRESTART_H
#define ROUNDRESTART_H

#pragma once

#include "mapentities.h"
#include "gameinterface.h"

#define MAX_PRESERVE_ENTS 100

class CRoundRestart : public CMemZeroOnNew, public CAutoGameSystemPerFrame
{
public:
	virtual bool Init();
	virtual void CleanUpMap();

	virtual bool ShouldCreateEntity( const char *pEntName );
	virtual bool ShouldCreateEntity( CBaseEntity *pEntity );

	virtual void PreserveEntity( const char *pEntName );
	virtual void DontPreserveEntity( const char *pEntName );
	virtual int IsPreserving( const char *pEntName );

protected:
	const char *m_pPreserveEnts[MAX_PRESERVE_ENTS];
};

extern CRoundRestart *roundRestart;

// Establecemos la clase que tendrá el filtro de que tipo de entidades crear
class CMapEntityFilter : public IMapEntityFilter
{
	public:
		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Debemos crear esta entidad
			if ( roundRestart->ShouldCreateEntity( pClassname ) )
				return true;

			// Increment our iterator since it's not going to call CreateNextEntity for this ent.
			if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
			{
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );
			}

			return false;
		}

		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CTeamplayMapEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || INDEXENT( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator;
};

#endif