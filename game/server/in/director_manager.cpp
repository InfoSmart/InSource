//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "director_manager.h"

#include "director.h"
#include "in_player.h"
#include "in_utils.h"
#include "players_manager.h"

#include "ai_basenpc.h"
#include "ai_memory.h"

#include "nav.h"
#include "nav_mesh.h"

#include "physobj.h"
#include "collisionutils.h"
#include "movevars_shared.h"

#include "ilagcompensationmanager.h"

#include "tier0/memdbgon.h"

// Nuestro ayudante.
CDirectorManager g_DirectorManager;
CDirectorManager *DirectorManager = &g_DirectorManager;

//====================================================================
// Comandos de consola
//====================================================================

ConVar director_update_nodes( "director_update_nodes", "1.0", 0, "Establece cada cuantos segundos se debera investigar por nodos para la creación de hijos" );
ConVar director_use_navmesh( "director_use_navmesh", "0", 0, "Establece si el Director debe usar el Navigation Mesh para creación de hijos" );
ConVar director_reference_navmesh( "director_reference_navmesh", "1", 0, "Establece si al usar los nodos para la creacion de hijos se tomaran en cuenta las Areas de Navegacion" );
ConVar director_spawn_in_batch( "director_spawn_in_batch", "0", 0, "Establece si el Director creará varios hijos en un solo lugar" );
ConVar director_debug_banareas( "director_debug_banareas", "0" );

#define UPDATE_NODES_INTERVAL	director_update_nodes.GetFloat()
#define USING_NAVMESH			director_use_navmesh.GetBool()
#define REFERENCE_NAVMESH		director_reference_navmesh.GetBool()
#define SPAWN_IN_BATCH			director_spawn_in_batch.GetBool()
#define DEBUG_BANAREAS			director_debug_banareas.GetBool()

//====================================================================
// Inicio
//====================================================================
void CDirectorManager::Init()
{
	DirectorManager = this;

	// TODO
	Director->SetPopulation( "default" );

	// Cargamos la información de la población
	LoadPopulation();
}

//====================================================================
// Un nuevo mapa ha sido cargado
//====================================================================
void CDirectorManager::OnNewMap()
{
	Precache();

	// Limpiamos los baneos
	ClearBans();

	// Limpiamos la lista
	m_hSpawnAreas.Purge();
	m_hSpawnNodes.Purge();

	// Tiempo para actualizar nodos
	m_hCandidateUpdateTimer.Start( 1 );
}

//====================================================================
// Carga la informacion de los hijos que se pueden crear
//====================================================================
void CDirectorManager::LoadPopulation()
{
	// Preparamos las llaves que contendrán la información
	KeyValues *pFile = new KeyValues("Population");
	KeyValues::AutoDelete autoDelete( pFile );

	m_hChilds.Purge();
	m_hBoss.Purge();
	m_hSpecials.Purge();

	// Leemos el archivo
	pFile->LoadFromFile( filesystem, DIRECTOR_POPULATION_FILE, NULL );
	KeyValues *pType;

	// Pasamos por cada sección
	for ( pType = pFile->GetFirstTrueSubKey(); pType; pType = pType->GetNextTrueSubKey() )
	{		
		// Este tipo de población no es la que usa el mapa actual
		if ( !FStrEq(pType->GetName(), STRING(Director->m_hPopulation)) )
			continue;

		KeyValues *pChildType;

		// Pasamos por los tipos de hijos
		for ( pChildType = pType->GetFirstSubKey(); pChildType; pChildType = pChildType->GetNextKey() )
		{
			KeyValues *pData;

			// Pasamos por los hijos a crear
			for ( pData = pChildType->GetFirstSubKey(); pData; pData = pData->GetNextKey() )
			{
				// Lo guardamos en la lista.
				if ( FStrEq(pChildType->GetName(), "Childs") )
					m_hChilds.Insert( pData->GetName(), pChildType->GetInt(pData->GetName(), 0) );

				if ( FStrEq(pChildType->GetName(), "Boss") )
					m_hBoss.Insert( pData->GetName(), pChildType->GetInt(pData->GetName(), 0) );

				if ( FStrEq(pChildType->GetName(), "Specials") )
					m_hSpecials.Insert( pData->GetName(), pChildType->GetInt(pData->GetName(), 0) );

				if ( FStrEq(pChildType->GetName(), "Ambient") )
					m_hAmbient.Insert( pData->GetName(), pChildType->GetInt(pData->GetName(), 0) );
			}
		}
	}
}

//====================================================================
// Guardado de objetos necesarios en caché
//====================================================================
void CDirectorManager::Precache()
{
	int iChilds		= m_hChilds.Count();
	int iSpecials	= m_hSpecials.Count();
	int iBoss		= m_hBoss.Count();
	int iAmbient	= m_hAmbient.Count();

	for ( int i = 0; i < iChilds; ++i )
		UTIL_PrecacheOther( m_hChilds.GetElementName(i) );

	for ( int i = 0; i < iSpecials; ++i )
		UTIL_PrecacheOther( m_hSpecials.GetElementName(i) );

	for ( int i = 0; i < iBoss; ++i )
		UTIL_PrecacheOther( m_hBoss.GetElementName(i) );

	for ( int i = 0; i < iAmbient; ++i )
		UTIL_PrecacheOther( m_hAmbient.GetElementName(i) );
}

//====================================================================
// Devuelve la cantidad de Areas/Nodos candidatas
//====================================================================
int CDirectorManager::GetCandidateCount()
{
	if ( IsUsingNavMesh() )
		return m_hSpawnAreas.Count();

	return m_hSpawnNodes.Count();
}

//====================================================================
// Devuelve si se esta usando el Nav Mesh para la creación
// de hijos.
//====================================================================
bool CDirectorManager::IsUsingNavMesh()
{
	return USING_NAVMESH;
}

//====================================================================
// Revela a los hijos la ubicación de los jugadores
//====================================================================
void CDirectorManager::Disclose()
{
	CBaseEntity *pChild = NULL;

	do
	{
		pChild = gEntList.FindEntityByName( pChild, "director_*" );

		// No existe o esta muerto
		if ( !pChild || pChild->IsPlayer() || !pChild->IsAlive() )
			continue;

		// Ya tiene a un jugador como enemigo
		if ( pChild->GetEnemy() && pChild->GetEnemy()->IsPlayer() )
			continue;

		// Seleccionamos un jugador al azar
		CIN_Player *pPlayer = PlysManager->GetRandom( TEAM_HUMANS );

		if ( !pPlayer )
			continue;

		// Tu nuevo enemigo
		pChild->MyNPCPointer()->UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin() );

	} while ( pChild );
}

//====================================================================
// Mata a todos los hijos creados
//====================================================================
void CDirectorManager::KillChilds( bool bOnlyNoVisible )
{
	CBaseEntity *pChild = NULL;

	do
	{
		pChild = gEntList.FindEntityByName( pChild, "director_*" );

		// No existe o esta muerto
		if ( pChild == NULL )
			continue;

		if ( pChild->IsPlayer() || !pChild->IsAlive() )
			continue;

		bool canRemove = true;

		// Solo a los que no se esten viendo
		if ( bOnlyNoVisible )
		{
			if ( PlysManager->InVisibleCone(pChild) )
				canRemove = false;
		}


		if ( canRemove )
		{
			//Director->FreeChildSlot( i, true );
			UTIL_Remove( pChild );
		}

	} while ( pChild );
}

//=========================================================
// Devuelve si es posible usar un area de navegación
// para la creación de hijos
//=========================================================
bool CDirectorManager::CanUseNavArea( CNavArea *pArea )
{
	// Es inválido
	if ( !pArea )
		return false;

	// No crear hijos aquí.
	if ( pArea->HasAttributes(NAV_MESH_DONT_SPAWN | NAV_MESH_PLAYER_START) )
		return false;

	#ifdef APOCALYPSE
		// Los infectados no pueden nadar
		if ( pArea->IsUnderwater() )
			return false;
	#endif

	// Alguno de los Jugadores ya ha estado aquí
	if ( Director->IsStatus(ST_NORMAL) )
	{
		if ( PlysManager->InLastKnowAreas(pArea) )
			return false;
	}

	// Ha sido baneada
	if ( IsBanned(pArea) )
		return false;

	// Esta Area ya esta en la lista de candidatas
	if ( m_hSpawnAreas.HasElement(pArea) )
		return false;

	return true;
}

//=========================================================
// Actualiza los puntos candidatos para crear hijos.
//=========================================================
void CDirectorManager::Update()
{
	// Aún no toca actualizar
	if ( m_hCandidateUpdateTimer.HasStarted() && !m_hCandidateUpdateTimer.IsElapsed() )
		return;

	// Actualizamos de nuevo en...
	m_hCandidateUpdateTimer.Start( UPDATE_NODES_INTERVAL );

	// Limpiamos las listas
	m_hSpawnAreas.Purge();
	m_hSpawnNodes.Purge();
	m_hForceSpawnAreas.Purge();
	m_hAmbientSpawnAreas.Purge();

	// ¡Este mapa no tiene nodos!
	// Lamentablemente los NPC's siguen usando la red de navegación por nodos
	if ( !g_pBigAINet || g_pBigAINet->NumNodes() <= 0 )
	{
		Warning("[CDirectorManager::Update] Este mapa no tiene nodos de movimiento !!! \n");
		return;
	}

	// Veamos en donde podemos crear hijos
	if ( IsUsingNavMesh() )
		UpdateNavPoints();
	else
		UpdateNodes();
}

//=========================================================
// Examina el sistema de navegación y selecciona los mejores
// lugares para crear hijos. ( Con NavMesh )
//=========================================================
void CDirectorManager::UpdateNavPoints()
{
	// Obtenemos todas las areas generadas por el sistema de navegación
	int iNavs = TheNavMesh->GetNavAreaCount();

	// ¡No hay sistema de navegación!
	if ( iNavs <= 0 )
	{
		Warning("[CDirectorManager::UpdateNavPoints] Este mapa no tiene un sistema de navegacion. Ejecuta el comando nav_generate en la consola. \n");
		return;
	}

	for ( int i = 0; i < MAX_NAV_AREAS; ++i )
	{
		// Obtenemos el area
		CNavArea *pArea = TheNavMesh->GetNavAreaByID(i);

		// No podemos usar esta area
		if ( !CanUseNavArea(pArea) )
			continue;

		// Buscamos al jugador más cercano
		float iDistance		= 0;
		Vector vecPos		= pArea->GetCenter();
		CIN_Player *pPlayer = PlysManager->GetNear( vecPos, iDistance );

		Vector vecPos1 = vecPos;
		Vector vecPos2 = vecPos;

		// Agregamos altura para evitar problemas
		vecPos.z += 10;

		// Verificaciones de altura
		vecPos1.z += 40;
		vecPos2.z += 90;

		// No se encontro ningún jugador cercano
		if ( !pPlayer )
			continue;

		// El area esta muy lejos o muy cerca
		if ( iDistance > Director->MaxDistance() || iDistance < Director->MinDistance() )
			continue;

		// No usar puntos que esten a la vista de los jugadores.
		//
		// Cuando el Area esta marcada como "OBSCURED" significa que ese lugar esta bloqueado visiblemente
		// a los jugadores ( detras de un arbol, arbusto, etc... )
		if ( ( !CanMake(vecPos) || !CanMake(vecPos1) || !CanMake(vecPos2) ) && !pArea->HasAttributes(NAV_MESH_OBSCURED) )
			continue;

		// Marcamos al punto afortunado. (Azul)
		if ( director_debug.GetInt() > 1 )
			NDebugOverlay::Box( vecPos, -Vector(5, 5, 5), Vector(5, 5, 5), 32, 32, 128, 10, 4.0f );
		
		// Area disponible para la creación
		m_hSpawnAreas.AddToTail( pArea );

		// Area para hijos de ambiente
		if ( pArea->HasAttributes(NAV_MESH_SPAWN_AMBIENT) )
			m_hAmbientSpawnAreas.AddToTail( pArea );

		// Area especial
		if ( pArea->HasAttributes(NAV_MESH_SPAWN_HERE) )
			m_hForceSpawnAreas.AddToTail( pArea );
	}
}

//=========================================================
// Examina los nodos en el mapa y selecciona los mejores
// para crear hijos. ( Con info_node )
//=========================================================
void CDirectorManager::UpdateNodes()
{
	// Obtenemos todos los nodos del mapa
	int iNumNodes = g_pBigAINet->NumNodes();

	// ¡No hay sistema de navegación!
	if ( REFERENCE_NAVMESH && TheNavMesh->GetNavAreaCount() <= 0 )
	{
		Warning("[CDirectorManager::UpdateNodes] Este mapa no tiene un sistema de navegacion. Ejecuta el comando nav_generate en la consola o desactiva el comando director_reference_navmesh. \n");
		return;
	}

	// Revisamos cada nodo
	for ( int i = 0; i < iNumNodes; ++i )
	{
		// Obtenemos el nodo
		CAI_Node *pNode = g_pBigAINet->GetNode(i);
		CNavArea *pArea = NULL;

		// El nodo es inválido o no esta en el suelo
		if ( !pNode || pNode->GetType() != NODE_GROUND )
			continue;

		// Este nodo ya esta en la lista.
		if ( m_hSpawnNodes.HasElement(pNode) )
			continue;

		// Buscamos al jugador más cercano
		float flDistance	= 0;
		bool bCheckVisible	= true;
		Vector vecOrigin	= pNode->GetPosition( HULL_HUMAN );
		CIN_Player *pPlayer = PlysManager->GetNear( vecOrigin, flDistance );

		if ( REFERENCE_NAVMESH )
		{
			// Obtenemos el area en donde se encuentra el nodo
			pArea = TheNavMesh->GetNearestNavArea( vecOrigin );

			// No podemos usar esta area
			if ( !CanUseNavArea(pArea) )
				continue;

			// El area esta marcado como oculto, no revisar si alguien lo esta viendo
			if ( pArea->HasAttributes(NAV_MESH_OBSCURED) )
				bCheckVisible = false;
		}
		
		Vector vecOrigin1	= vecOrigin;
		Vector vecOrigin2	= vecOrigin;

		// Agregamos altura para evitar problemas
		vecOrigin.z += 10;

		// Verificaciones de altura
		vecOrigin1 += 30;
		vecOrigin2 += 80;

		// ¡Ninguno!
		if ( !pPlayer )
			continue;

		// Este nodo esta muy lejos o muy cerca.
		if ( flDistance > Director->MaxDistance() || flDistance < Director->MinDistance() )
			continue;

		// No usar nodos que esten a la vista de los jugadores
		if ( bCheckVisible )
		{
			if ( !CanMake(vecOrigin) || !CanMake(vecOrigin1) || !CanMake(vecOrigin2) )
				continue;
		}

		// Marcamos al nodo afortunado (Azul)
		if ( director_debug.GetInt() > 1 )
			NDebugOverlay::Box( vecOrigin, -Vector(5, 5, 5), Vector(5, 5, 5), 32, 32, 128, 10, 4.0f );

		if ( REFERENCE_NAVMESH )
		{
			// TODO: Hijos de ambiente y areas forzadas
		}

		// Lo agregamos a la lista
		m_hSpawnNodes.AddToTail( pNode );
	}
}

//=========================================================
// Devuelve un lugar para la creación de hijos
//=========================================================
bool CDirectorManager::GetSpawnLocation( Vector *vecPosition, int iType )
{
	bool bCheckVisible	= true;
	int i				= 0;

	//
	// NavMesh
	// Obtenemos una area al azar y de ahí un punto al azar.
	//
	if ( IsUsingNavMesh() )
	{
		// No hay Areas donde podamos crear
		if ( m_hSpawnAreas.Count() <= 0 )
			return false;

		CNavArea *pArea = NULL;

		// Hijos de ambiente
		if ( iType == DIRECTOR_AMBIENT_CHILD )
		{
			// No hay Areas donde crear Hijos de ambiente
			if ( m_hAmbientSpawnAreas.Count() <= 0 )
				return false;

			i		= RandomInt( 0, m_hAmbientSpawnAreas.Count() - 1 );
			pArea	= m_hAmbientSpawnAreas[ i ];
		}

		// Hijos normales
		else
		{
			// Hay Areas especiales donde solo ahi debemos crear Hijos
			if ( m_bSpawnInForceAreas && m_hForceSpawnAreas.Count() > 0 )
			{
				i = RandomInt( 0, m_hForceSpawnAreas.Count() - 1 );
				pArea = m_hForceSpawnAreas[i];
			}

			// Usar cualquier area
			else
			{
				i = RandomInt( 0, m_hSpawnAreas.Count() - 1 );
				pArea = m_hSpawnAreas[i];
			}
		}

		// Es una Area baneada
		if ( IsBanned(pArea) )
			return false;

		*vecPosition = pArea->GetRandomPoint();

		// El area seleccionada esta marcada como "Oculto"
		if ( pArea->HasAttributes(NAV_MESH_OBSCURED) )
			bCheckVisible = false;
	}

	//
	// Nodos de movimiento.
	//
	else
	{
		// No hay nodos candidatos
		if ( m_hSpawnNodes.Count() == 0 )
			return false;

		i				= RandomInt(0, m_hSpawnNodes.Count() - 1);
		*vecPosition	= m_hSpawnNodes[i]->GetPosition( HULL_HUMAN );
	}

	// No usar puntos que esten a la vista de los jugadores
	if ( bCheckVisible && !CanMake(*vecPosition) )
		return false;

	return true;
}

//====================================================================
//====================================================================
void CDirectorManager::UnBan( CNavArea *pArea )
{
	m_iBannedAreas[ pArea->GetID() ] = NULL;
}

//====================================================================
//====================================================================
void CDirectorManager::UnBan( const Vector vecPosition )
{
	CNavArea *pArea = TheNavMesh->GetNearestNavArea( vecPosition );

	if ( pArea )
	{
		UnBan( pArea );
	}
}

//====================================================================
// Banea un Area por la cantidad de segundos indicado
//====================================================================
void CDirectorManager::AddBan( CNavArea *pArea, float flDuration )
{
	// Ya ha sido baneada
	if ( IsBanned(pArea) )
		return;

	m_iBannedAreas[ pArea->GetID() ] = gpGlobals->curtime + flDuration;
}

//====================================================================
// Banea una ubicación y su Nav Area por la cantidad de segundos indicado
//====================================================================
void CDirectorManager::AddBan( const Vector vecPosition, float flDuration )
{
	CNavArea *pArea = TheNavMesh->GetNearestNavArea( vecPosition );

	if ( pArea )
	{
		AddBan( pArea, flDuration );
	}
}

//====================================================================
//====================================================================
void CDirectorManager::ClearBans()
{
	for ( int i = 0; i < MAX_NAV_AREAS; ++i )
	{
		m_iBannedAreas[i] = NULL;
	}
}

//====================================================================
// Devuelve si un Nav Area ha sido baneado por el Director
//====================================================================
bool CDirectorManager::IsBanned( CNavArea *pArea )
{
	// Area inválida
	if ( !pArea )
		return true;

	float flExpire = m_iBannedAreas[ pArea->GetID() ];

	// No ha sido baneada
	if ( flExpire == NULL )
		return false;

	// El tiempo de baneo ha expirado
	if ( gpGlobals->curtime >= flExpire )
	{
		m_iBannedAreas[ pArea->GetID() ] = NULL;
		return false;
	}

	if ( DEBUG_BANAREAS )
		NDebugOverlay::Box( pArea->GetCenter(), -Vector(15, 15, 15), Vector(15, 15, 15), 32, 32, 128, 10, 1.0f );

	// Esta baneada
	return true;
}

//====================================================================
// Devuelve si es posible crear un hijo en esta ubicación
//====================================================================
bool CDirectorManager::CanMake( const Vector &vecPosition )
{
	// No usar la ubicación si esta a la vista de los jugadores.
	if ( director_spawn_outview.GetBool() )
	{
		if ( PlysManager->IsVisible(vecPosition) )
			return false;
	}

	return true;
}

//====================================================================
// Verificaciones después de crear al NPC.
//====================================================================
bool CDirectorManager::PostSpawn( CBaseEntity *pEntity, Hull_t pHull )
{
	//bool isStuck = true;

	//
	// Mientras estemos atorados en algo...
	//
	while ( true )
	{
		trace_t tr;
		UTIL_TraceHull( pEntity->WorldSpaceCenter(), pEntity->WorldSpaceCenter(), pEntity->WorldAlignMins(), pEntity->WorldAlignMaxs(), MASK_NPCSOLID, pEntity, COLLISION_GROUP_NONE, &tr );

		// Estamos libres
		if ( tr.fraction == 1.0 )
			break;

		if ( tr.m_pEnt )
		{
			//
			// Atorado en un objeto con físicas
			//
			if ( FClassnameIs(tr.m_pEnt, "prop_physics") )
			{
				// Hacemos que el objeto no sea solido
				tr.m_pEnt->AddSolidFlags( FSOLID_NOT_SOLID );

				// Notificamos
				ConColorMsg( Color(174, 180, 4), "[Director] El objeto <prop_physics: %s> esta bloqueando un hijo. Eliminado. \n", STRING(tr.m_pEnt->GetModelName()) );

				// Eliminamos el objeto
				UTIL_Remove( tr.m_pEnt );
				continue;
			}

			//
			// Atorado en algo que se puede romper
			//
			if ( Utils::IsBreakable(tr.m_pEnt) )
			{
				CBreakableProp *pBreakableProp	= dynamic_cast<CBreakableProp *>( tr.m_pEnt );
				CBreakable *pBreakable			= dynamic_cast<CBreakable *>( tr.m_pEnt );

				// Lo rompemos
				if ( pBreakableProp )
					pBreakableProp->Break( pEntity, CTakeDamageInfo(pEntity, pEntity, 100, DMG_GENERIC) );
				if ( pBreakable )
					pBreakable->Break( pEntity );

				continue;
			}

			if ( director_debug.GetInt() > 1 )
				ConColorMsg( Color(174, 180, 4), "Un hijo se ha quedado atorado con %s. Eliminado. \n", tr.m_pEnt->GetClassname() );

			// Atorado...
			return false;
		}

		// ¡No estamos atorados!
		break;
	}

	/*bool bHaveRoute = false;

	// Verificamos si tienes una ruta a al menos un Jugador
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = PlysManager->Get( i );

		if ( !pPlayer )
			continue;

		AI_NavGoal_t goal;
		goal.type		= GOALTYPE_LOCATION;
		goal.dest		= pPlayer->WorldSpaceCenter();
		goal.pTarget	= pPlayer;

		bool bResult = pNPC->GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );

		// Solo era una prueba ¡no te muevas!
		pNPC->GetNavigator()->ClearGoal();
		pNPC->GetNavigator()->StopMoving();

		// Tiene una ruta segura hacia este Jugador
		if ( bResult )
		{
			bHaveRoute = true;
			break;
		}
	}

	// No tienes ninguna ruta
	if ( !bHaveRoute )
	{
		// Baneamos el Nav Area donde se encuentre
		AddBan( pNPC->GetAbsOrigin(), BAN_DURATION_UNREACHABLE );
		return false;
	}
	else
	{
		// En caso de que el Area estuviera baneado lo desbaneamos, de esta manera eliminamos los baneos
		// de Areas que no tuvieron una ruta a los Jugadores solo por un momento
		UnBan( pNPC->GetAbsOrigin() );
	}*/

	return true;
}

//====================================================================
// Devuelve el nombre de un hijo según su tipo
//====================================================================
const char *CDirectorManager::GetChildName( int iType )
{
	switch ( iType )
	{
		case DIRECTOR_CHILD:
		default:
			return DIRECTOR_CHILD_NAME;

		case DIRECTOR_SPECIAL_CHILD:
			return DIRECTOR_SPECIAL_NAME;

		case DIRECTOR_BOSS:
			return DIRECTOR_BOSS_NAME;

		case DIRECTOR_AMBIENT_CHILD:
			return DIRECTOR_AMBIENT_NAME;

		case DIRECTOR_CUSTOMCHILD:
			return DIRECTOR_CUSTOM_NAME;
	}
}

//====================================================================
// Devuelve una clase de hijo para crear
//====================================================================
const char *CDirectorManager::GetRandomClass( int iType )
{
	const char *pClass;
	int iPopulation;

	switch ( iType )
	{
		case DIRECTOR_CHILD:
		default:
			pClass		= m_hChilds.GetElementName(0);
			iPopulation	= m_hChilds.Count();
		break;

		case DIRECTOR_BOSS:
			pClass		= m_hBoss.GetElementName(0);
			iPopulation	= m_hBoss.Count();
		break;

		case DIRECTOR_SPECIAL_CHILD:
			pClass		= m_hSpecials.GetElementName(0);
			iPopulation	= m_hSpecials.Count();
		break;

		case DIRECTOR_AMBIENT_CHILD:
			pClass		= m_hAmbient.GetElementName(0);
			iPopulation	= m_hAmbient.Count();
		break;
	}

	// Numero al azar del 1% al 100%
	int iRandom	= RandomInt(1, 100);		

	// Pasamos por todos los posibles hijos que pueden aparecer ahora mismo. ( Tipo de población )
	for ( int i = 0; i < iPopulation; ++i )
	{
		switch ( iType )
		{
			case DIRECTOR_CHILD:
			default:
				if ( m_hChilds[i] > iRandom )
					pClass = m_hChilds.GetElementName(i);
			break;

			case DIRECTOR_BOSS:
				if ( m_hBoss[i] > iRandom )
					pClass = m_hBoss.GetElementName(i);
			break;

			case DIRECTOR_SPECIAL_CHILD:
				if ( m_hSpecials[i] > iRandom )
					pClass = m_hSpecials.GetElementName(i);
			break;

			case DIRECTOR_AMBIENT_CHILD:
				if ( m_hAmbient[i] > iRandom )
					pClass = m_hAmbient.GetElementName(i);
			break;
		}
	}

	return pClass;
}

//====================================================================
// Permite ajustar cierta configuración especial para
// algunos hijos que se creen con el Director
//=========================================================
// Ideal para las modificaciones del Director
//====================================================================
void CDirectorManager::SetupChild( CAI_BaseNPC *pChild )
{
	if ( RandomInt(1, 100) > 90 )
		pChild->SetCollisionGroup( COLLISION_GROUP_NOT_BETWEEN_THEM );
}

//====================================================================
// Comienza la creación de hijos
//====================================================================
void CDirectorManager::StartSpawn( int iType )
{
	// Le notificamos al Director que hemos empezado
	Director->m_bSpawning = true;

	// Actualizamos la lista de ubicaciones donde podemos crear
	Update();

	int iSpawnQueue = Director->MaxChilds() - Director->m_iCommonAlive;

	//
	// Hijos normales
	//
	if ( iType == DIRECTOR_CHILD )
	{
		// ¿Como llegue hasta aquí?
		if ( iSpawnQueue <= 0 )
			return;

		// Varios hijos en una ubicación
		if ( SPAWN_IN_BATCH || Director->IsStatus(ST_COMBAT) || Director->IsStatus(ST_FINALE) )
		{
			SpawnBatch( iType );
		}

		// Cada hijo en una ubicación diferente
		else
		{
			for ( int i = 0; i <= iSpawnQueue; ++i )
			{
				SpawnChild( iType );
			}
		}
	}

	//
	// Hijos especiales o jefes
	//
	else
	{
		SpawnChild( iType );
	}

	Director->m_bSpawning = false;
}

//====================================================================
// Crea un hijo en la ubicación indicada
//====================================================================
bool CDirectorManager::SpawnChild( int iType, const Vector &vecPosition )
{
	// Intentamos crear al hijo
	bool bSpawn	= MakeSpawn( GetRandomClass(iType), iType, vecPosition );
	return bSpawn;
}

//====================================================================
// Crea un hijo en un nodo candidato
//====================================================================
void CDirectorManager::SpawnChild( int iType )
{
	for ( int i = 0; i < DIRECTOR_MAX_SPAWN_TRYS; ++i )
	{
		Vector vecPosition;

		// Seleccionamos una ubicación para la creación
		bool bResult = GetSpawnLocation( &vecPosition, iType );

		// ¡No hay donde crear! Volvemos a intentarlo
		if ( !bResult )
			continue;

		// El hijo no se ha podido crear, volvemos a intentarlo
		if ( !SpawnChild(iType, vecPosition) )
			continue;

		break;
	}
}

//====================================================================
// Crea un grupo de hijos en la ubicación indicada
//====================================================================
void CDirectorManager::SpawnBatch( int iType, const Vector &vecPosition )
{
	// Hijos a crear
	int iToSpawn = RandomInt( 3, 4 );

	// Creamos los hijos en esta ubicación
	for ( int e = 0; e < iToSpawn; ++e )
	{
		// Intentamos crear al hijo
		MakeSpawn( GetRandomClass(iType), iType, vecPosition );
	}
}

//====================================================================
// Crea un grupo de hijos en la ubicación indicada
//====================================================================
void CDirectorManager::SpawnBatch( int iType, int iNumber )
{
	Vector vecPosition;

	// Seleccionamos una ubicación para la creación
	bool bResult = GetSpawnLocation( &vecPosition, iType );

	// ¡No hay donde crear!
	if ( !bResult )
		return;

	// Creamos los hijos en esta ubicación
	for ( int e = 0; e < iNumber; ++e )
	{
		// Intentamos crear al hijo
		MakeSpawn( GetRandomClass(iType), iType, vecPosition );
	}
}

//====================================================================
// Crea un grupo de hijos en un nodo candidato
//====================================================================
void CDirectorManager::SpawnBatch( int iType )
{
	// Hijos que debemos crear
	int spawnQueue = Director->MaxChilds() - Director->m_iCommonAlive;

	while ( spawnQueue > 0 )
	{
		// Hijos a crear
		int toSpawn = RandomInt( 3, 4 );

		// Creamos el "grupo"
		SpawnBatch( iType, toSpawn );

		// Disminuimos de la cola
		spawnQueue -= toSpawn;
	}
}

//====================================================================
// Crea un tipo de hijo en la ubicación especificada
//====================================================================
bool CDirectorManager::MakeSpawn( const char *pClass, int iType, const Vector &vecPosition )
{
	CAI_BaseNPC *pChild	= (CAI_BaseNPC *)CBaseEntity::CreateNoSpawn( pClass, vecPosition, vec3_angle );

	if ( !pChild )
	{
		Warning( "[Director] Ha ocurrido un problema al crear un %s \n", pClass );
		return false;
	}

	// ¿Como se llamara?
	string_t pName = AllocPooledString( GetChildName(iType) );
	Vector vecPositionRadius;

	// Angulos de creación
	QAngle angles = RandomAngle(0, 360);
	angles.x = 0.0;
	angles.z = 0.0;	

	// Su cadaver debe desaparecer
	pChild->AddSpawnFlags( SF_NPC_FADE_CORPSE );

	// Debe caer al suelo.
	pChild->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	// Intentamos crearlo en un radio de 80 unidades
	if ( CAI_BaseNPC::FindSpotForNPCInRadius(&vecPositionRadius, vecPosition, pChild, 200) )
	{
		// Evitamos la creación por debajo del suelo
		vecPositionRadius.z	+= 10;
		pChild->SetAbsOrigin( vecPositionRadius );
		
		// No podemos crearlo aquí
		if ( !CanMake(vecPositionRadius) )
		{
			UTIL_Remove( pChild );
			return false;
		}
	}

	// Lo creamos
	DispatchSpawn( pChild );
	pChild->Activate();

	// El Hijo debe tener compensación de Lag
	lagcompensation->AddAdditionalEntity( pChild );

	// ¡¡NO CAMBIAR!!
	pChild->SetName( pName );

	// Definimos el equipo de este hijo
	pChild->ChangeTeam( Director->GetTeamEvil() );

	// Configuramos
	SetupChild( pChild );

	// Al parecer se atoro en una pared
	if ( !PostSpawn(pChild) )
	{
		// Marcamos al nodo desafortunado. (Negro)
		if ( director_debug.GetInt() > 1 )
			NDebugOverlay::Box( pChild->GetAbsOrigin(), -Vector(10, 10, 10), Vector(10, 10, 10), 0, 0, 0, 10, 15.0f );

		UTIL_Remove( pChild );
		return false;
	}

	// Marcamos al nodo afortunado. (Rojo)
	if ( director_debug.GetBool() )
		NDebugOverlay::Box( pChild->GetAbsOrigin(), -Vector(10, 10, 10), Vector(10, 10, 10), 223, 1, 1, 10, 6.0f );

	// Un hijo más
	switch ( iType )
	{
		case DIRECTOR_CHILD:
		default:
			Director->OnSpawnChild( pChild );
		break;

		case DIRECTOR_BOSS:
			Director->OnSpawnBoss( pChild );
		break;

		case DIRECTOR_SPECIAL_CHILD:
			Director->OnSpawnSpecial( pChild );
		break;

		case DIRECTOR_AMBIENT_CHILD:
			Director->OnSpawnAmbient( pChild );
		break;
	}

	return true;
}