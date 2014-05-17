//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "director_item_manager.h"

#include "director.h"

#include "in_gamerules.h"
#include "directordefs.h"
#include "players_manager.h"

#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"

// Creamos la instancia al sistema
CDirectorItemManager g_DirectorItemManager;
CDirectorItemManager *DirectorItemManager = &g_DirectorItemManager;

//====================================================================
// Comandos
//====================================================================

ConVar director_items_min_distance( "director_spawn_min_distance", "500" );

#define MIN_DISTANCE director_items_min_distance.GetFloat()

//====================================================================
//====================================================================
bool CDirectorItemManager::Init()
{
	return true;
}

//====================================================================
// Nuevo Mapa - Después de cargar las entidades
//====================================================================
void CDirectorItemManager::LevelInitPostEntity()
{
	FindSpawns();
}

//====================================================================
// Pensamiento - Después de las entidades
//====================================================================
void CDirectorItemManager::FrameUpdatePostEntityThink()
{
	// El Juego no ha empezado
	if ( !InRules )
		return;

	Update();
}

//====================================================================
// Encuentra todos los puntos de creación "director_spawn" para crear
// objetos cuando los Jugadores se encuentren cerca
//====================================================================
void CDirectorItemManager::FindSpawns()
{
	CDirectorSpawn *pSpawn = NULL;

	// Limpiamos la lista
	m_nSpawnEntities.Purge();

	do
	{
		pSpawn = (CDirectorSpawn *)gEntList.FindEntityByClassname( pSpawn, "director_spawn" );

		if ( !pSpawn )
			continue;

		m_nSpawnEntities.AddToTail( pSpawn );
	}
	while ( pSpawn );
}

//====================================================================
//====================================================================
void CDirectorItemManager::Update()
{
	// No hay entidades para crear en este mapa
	if ( m_nSpawnEntities.Count() <= 0 || Director->IsStatus(ST_GAMEOVER) )
		return;

	for ( int i = 0; i < m_nSpawnEntities.Count(); ++i )
	{
		CDirectorSpawn *pSpawn = m_nSpawnEntities.Element( i );

		// Spawn inválido. Esto no debería pasar, pero por si acaso...
		if ( !pSpawn )
			continue;

		// No puede crear un objeto ahora mismo
		if ( !pSpawn->CanSpawnItem() )
			continue;

		// Establecemos la distancia al Jugador más cercano
		float flDistance = 0.0f;
		PlysManager->GetNear( pSpawn->GetAbsOrigin(), flDistance );

		// Esta muy lejos
		if ( flDistance <= 0 || flDistance > MIN_DISTANCE )
			continue;

		// ¡Creamos!
		pSpawn->SpawnItem();
	}
}