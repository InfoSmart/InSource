//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "roundrestart.h"

#include <igameevents.h>
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Creamos la instancia a este sistema
CRoundRestart g_roundRestart;
CRoundRestart *roundRestart = &g_roundRestart;

//=========================================================
// Inicia el sistema
//=========================================================
bool CRoundRestart::Init()
{
	// Entidades que no deben ser eliminadas al reiniciar la ronda
	PreserveEntity( "ai_network" );
	PreserveEntity( "ai_hint" );
	PreserveEntity( "ambient_generic" );

	PreserveEntity( "commentary_auto" );

	PreserveEntity( "viewmodel" );
	
	PreserveEntity( "env_soundscape" );
	PreserveEntity( "env_soundscape_proxy" );
	PreserveEntity( "env_soundscape_triggerable" );
	PreserveEntity( "env_sprite" );
	PreserveEntity( "env_sun" );
	PreserveEntity( "env_wind" );
	PreserveEntity( "env_fog_controller" );

	PreserveEntity( "func_brush" );
	PreserveEntity( "func_wall" );
	PreserveEntity( "func_illusionary" );
	PreserveEntity( "func_rotating" );

	PreserveEntity( "info_node" );
	PreserveEntity( "info_target" );
	PreserveEntity( "info_node_hint" );
	PreserveEntity( "info_player_start" );
	PreserveEntity( "infodecal" );
	PreserveEntity( "info_projecteddecal" );
	PreserveEntity( "info_spectator" );
	PreserveEntity( "info_map_parameters" );
	PreserveEntity( "info_ladder" );
	PreserveEntity( "info_director" );
	PreserveEntity( "in_team_manager" );

	PreserveEntity( "keyframe_rope" );
	PreserveEntity( "move_rope" );

	PreserveEntity( "player" );
	PreserveEntity( "in_player" );
	PreserveEntity( "player_infected" );

	PreserveEntity( "player_manager" );
	PreserveEntity( "point_commentary_node" );
	PreserveEntity( "point_viewcontrol" );
	PreserveEntity( "point_viewcontrol_multiplayer" );
	PreserveEntity( "point_commentary_viewpoint" );
	PreserveEntity( "predicted_viewmodel" );
	PreserveEntity( "point_devshot_camera" );
	PreserveEntity( "postprocess_controller" );

	PreserveEntity( "func_precipitation" );
	PreserveEntity( "func_team_wall" );

	PreserveEntity( "shadow_control" );
	PreserveEntity( "sky_camera" );
	PreserveEntity( "scene_manager" );
	PreserveEntity( "soundent" );

	PreserveEntity( "trigger_soundscape" );

	PreserveEntity( "layer_sound" );

	PreserveEntity( "in_gamerules" );
	PreserveEntity( "worldspawn" );

	return true;
}

//=========================================================
// Limpia el mapa y vuelve a crear todas las entidades
//=========================================================
void CRoundRestart::CleanUpMap()
{
	// Notificación para desarrolladores
	DevMsg( "[CRoundRestart::CleanUpMap] ============= \n" );
	DevMsg( " Entities: %d (%d edicts)\n", gEntList.NumberOfEntities(), gEntList.NumberOfEdicts() );

	// Creamos una lista de todas las entidades en el mapa
	CBaseEntity *pCur = gEntList.FirstEnt();

	while ( pCur )
	{
		// Debemos recrear esta entidad
		if ( ShouldCreateEntity(pCur) )
		{
			// La eliminamos
			DevMsg( " > Eliminando: %s \n", pCur->GetClassname() );
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Limpiamos
	g_EventQueue.Clear();

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();
	engine->AllowImmediateEdictReuse();

	// Creamos el filtro anterior
	CMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// Recreamos todas las entidades del mapa
	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}

//=========================================================
// Devuelve si la entidad debe recrearse
//=========================================================
bool CRoundRestart::ShouldCreateEntity( const char *pEntName )
{
	return ( IsPreserving(pEntName) > -1 ) ? false : true;
}

//=========================================================
// Devuelve si la entidad debe recrearse
//=========================================================
bool CRoundRestart::ShouldCreateEntity( CBaseEntity *pEntity )
{
	return ShouldCreateEntity( pEntity->GetClassname() );
}

//=========================================================
// Agrega una entidad a la lista de "no recrearse"
//=========================================================
void CRoundRestart::PreserveEntity( const char *pEntName )
{
	// Ya esta en la lista
	if ( IsPreserving(pEntName) > -1 )
		return;

	// Verificamos en cada Slot
	for ( int i = 0; i < MAX_PRESERVE_ENTS; ++i )
	{
		// Slot ocupado
		if ( m_pPreserveEnts[i] != NULL )
			continue;

		m_pPreserveEnts[i] = pEntName;
		break;
	}
}

//=========================================================
// Elimina una entidad de la lista de "no recrearse"
//=========================================================
void CRoundRestart::DontPreserveEntity( const char *pEntName )
{
	int i = IsPreserving( pEntName );

	// No esta en la lista
	if ( i == -1 )
		return;

	m_pPreserveEnts[i] = NULL;
}

//=========================================================
// Devuelve el Slot de la entidad en la lista de "no recrearse"
//=========================================================
int CRoundRestart::IsPreserving( const char *pEntName )
{
	// Verificamos en cada Slot
	for ( int i = 0; i < MAX_PRESERVE_ENTS; ++i )
	{
		if ( m_pPreserveEnts[i] == NULL )
			continue;

		if ( Q_stricmp( m_pPreserveEnts[i], pEntName ) == 0 )
			return i;
	}

	return -1;
}