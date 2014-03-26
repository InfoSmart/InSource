//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"
#include "nodes_generation.h"

#include "nav.h"
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_ladder.h"
#include "nav_node.h"

#include "ai_initutils.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_networkmanager.h"

#include "editor_sendcommand.h"
#include "in_shareddefs.h"

#include "tier0/memdbgon.h"

//=========================================================
// Comandos
//=========================================================

ConVar ai_generate_nodes_pendient("ai_generate_nodes_pendient", "0", FCVAR_SERVER, "");
ConVar ai_generate_nodes_inwater("ai_generate_nodes_inwater", "0", FCVAR_SERVER, "Establece si se permitira generar nodos en el agua");

//=========================================================
//=========================================================

CNodesGeneration g_NodesGeneration;
CNodesGeneration *NodesGen() { return &g_NodesGeneration; }

//=========================================================
// Comienza el proceso para generar nodos
//=========================================================
void CNodesGeneration::Start()
{
	// Se debe estar en modo de edición
	if ( !engine->IsInEditMode() )
	{
		Msg( "============ SOLO PUEDEN GENERARSE NODOS EN EL MODO DE EDICIÓN - map_edit ============ \n" );
		return;
	}

	#ifdef APOCALYPSE

		// Paramos al Director
		engine->ServerCommand("director_stop \n");

	#endif

	if ( !engine->IsDedicatedServer() )
	{
		CBasePlayer *host = UTIL_GetListenServerHost();

		if ( host )
			host->ChangeTeam( TEAM_SPECTATOR );
	}

	// Sacamos a los Bots
	engine->ServerCommand( "bot_kick\n" );

	// Obtenemos el número de areas generadas
	iNavAreaCount = TheNavMesh->GetNavAreaCount();

	// No hay ninguna area
	if ( iNavAreaCount <= 0 )
	{
		Warning("No se ha generado el NavMesh en el mapa. Use el comando nav_generate para generarlo. \n");
		return;
	}

	m_iWalkNodesGenerated = 0;

	Msg("Comenzando el procesamiento de %i areas... \n", iNavAreaCount);

	GenerateWalkableNodes();
	GenerateClimbNodes();
	GenerateHintNodes();

	Msg("Se han generado %i nodos. \n\n", g_pAINetworkManager->GetNetwork()->NumNodes());
}

//=========================================================
// Genera nodos de movimiento
//=========================================================
void CNodesGeneration::GenerateWalkableNodes()
{
	if ( m_iWalkNodesGenerated <= 0 )
		Msg("Generando nodos de tipo transitables... \n");

	int iNodesGenerated = m_iWalkNodesGenerated;

	Vector vecPoint;
	Vector vecUp;

	for ( int i = 0; i <= MAX_NAV_AREAS; ++i )
	{
		// Obtenemos la instancia de esta area
		CNavArea *pArea = TheNavMesh->GetNavAreaByID( i );

		// No existe
		if ( !pArea )
			continue;

		// No debemos usar areas que esten en el agua
		if ( !ai_generate_nodes_inwater.GetBool() && pArea->IsUnderwater() )
			continue;

		for ( int e = 0; e <= MAX_NODES_PER_AREA; ++e )
		{
			// Obtenemos la ubicación del nodo y agregamos altura
			vecPoint = vecUp = pArea->GetRandomPoint();
			vecPoint.z	+= 20;
			vecUp.y		+= 1;

			bool bTooClose = false;

			if ( m_vecWalkLocations.Count() > 0 )
			{
				// Ya hemos generado un nodo aquí
				if ( m_vecWalkLocations.HasElement(vecPoint) )
					continue;

				for ( int k = 0; k <= m_vecWalkLocations.Count(); ++k )
				{
					Vector vecTmpPosition = m_vecWalkLocations[k];

					if ( vecTmpPosition.IsValid() )
					{
						// Esta muy cerca de otro nodo
						if ( vecPoint.DistTo( vecTmpPosition ) <= 70 )
						{
							bTooClose = true;
							break;
						}
					}
				}
			}

			// Esta muy cerca de otro nodo
			if ( bTooClose )
				continue;

			int iStatus = Editor_CreateNode( "info_node", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecPoint.x, vecPoint.y, vecPoint.z, false );

			if ( iStatus == Editor_OK )
			{
				m_vecWalkLocations.AddToTail( vecPoint );

				++m_iWalkNodesGenerated;
			}
		}
	}

	int iMax = (MAX_NODES-500);

	// No se han generado todos los nodos que tenemos disponibles, para generar
	// un movimiento más fluido repetimos el proceso hasta completar o que no haya
	// lugares donde crear más
	if ( m_iWalkNodesGenerated < iMax && iNodesGenerated < m_iWalkNodesGenerated )
	{
		Msg("%i/%i... \n", m_iWalkNodesGenerated, iMax);

		GenerateWalkableNodes();
		return;
	}

	Msg( "Se han creado %i nodos de tipo transitables... \n\n", m_iWalkNodesGenerated );
}

//=========================================================
// Genera nodos para escalar
//=========================================================
void CNodesGeneration::GenerateClimbNodes()
{
	DevMsg("Generando nodos de tipo escalables... \n");

	int iNodesInArea = 0;

	Vector vecUp;
	Vector vecDown;
	Vector tmpPoint;

	for ( int i = 0; i <= MAX_NAV_AREAS; ++i )
	{
		// Obtenemos la instancia acerca de esta area
		CNavArea *pArea	= TheNavMesh->GetNavAreaByID( i );

		// No existe
		if ( !pArea )
			continue;

		// No debemos usar areas que esten en el agua
		if ( !ai_generate_nodes_inwater.GetBool() && pArea->IsUnderwater() )
			continue;

		// Obtenemos las "escaleras" del area
		const NavLadderConnectVector *pLadders = pArea->GetLadders( CNavLadder::LADDER_UP );

		// No hay escaleras
		if ( pLadders->Count() <= 0 )
			continue;

		for ( int e = 0; e < pLadders->Count(); ++e )
		{
			// Obtenemos la escalera con esta ID
			CNavLadder *pLadder = pLadders->Element(e).ladder;

			// No existe
			if ( !pLadder )
				continue;

			// Ubicación de donde comienza y donde termina
			vecUp	= pLadder->m_top;
			vecDown = pLadder->m_bottom;

			// Agregamos un poco de altura
			vecUp	+= 5;
			vecDown += 5;

			Editor_CreateNode( "info_node_climb", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecUp.x, vecUp.y, vecUp.z, false );
			Editor_CreateNode( "info_node_climb", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecDown.x, vecDown.y, vecDown.z, false );
			
			// Creación completa
			++iNodesInArea;
			
			// Creación completa
			++iNodesInArea;
		}
	}

	MAX_NODES;

	DevMsg("Se han creado %i nodos de tipo escalables... \n\n", iNodesInArea );
}

//=========================================================
// Genera nodos de ayuda
//=========================================================
void CNodesGeneration::GenerateHintNodes()
{
	// Obtenemos los lugares con buen escondite
	int iHidingSpots	= TheHidingSpots.Count();
	int iNodes			= 0;

	CNavArea *pArea;
	Vector vecPosition;

	int iEditorStatus;

	// No se encontraron lugares
	if ( iHidingSpots <= 0 )
	{
		Msg("[CNodesGeneration] No se han generado puntos de resguardo para generar nodos de ayuda. \n");
		return;
	}

	for ( int i = 0; i <= iHidingSpots; ++i )
	{
		// Obtenemos información acerca de este punto.
		HidingSpot *pSpot = TheHidingSpots.Element( i );

		// No existe
		if ( !pSpot )
			continue;

		// No existe
		if ( TheHidingSpots.Find(pSpot) <= -1 )
			continue;

		// Obtenemos la ubicación del punto
		vecPosition = pSpot->GetPosition();

		// Obtenemos el area que se encuentra en la ubicación
		pArea = TheNavMesh->GetNavArea( vecPosition );

		// No existe
		if ( !pArea )
			continue;

		// No debemos usar areas que esten en el agua
		if ( !ai_generate_nodes_inwater.GetBool() && pArea->IsUnderwater() )
			continue;

		// Agregamos un poco de altura.
		vecPosition.z += 10;

		//
		// Buena cobertura
		// Los NPC lo usarán para cubrise de ataques enemigos
		//
		if ( pSpot->HasGoodCover() )
		{
			iEditorStatus = Editor_CreateNode( "info_node_hint", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecPosition.x, vecPosition.y, vecPosition.z, false );

			if ( iEditorStatus == Editor_OK )
			{
				Editor_SetKeyValue( "info_node_hint", vecPosition.x, vecPosition.y, vecPosition.z, "hinttype", "100" );
				++iNodes;
			}
		}

		//
		// Buena posición francotirador
		// Los NPC lo usarán para mirar de vez en cuando a esta posición
		//
		if ( pSpot->IsIdealSniperSpot() || pSpot->IsGoodSniperSpot() )
		{
			iEditorStatus = Editor_CreateNode( "info_hint", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecPosition.x, vecPosition.y, vecPosition.z, false );

			if ( iEditorStatus == Editor_OK )
			{
				Editor_SetKeyValue( "info_hint", vecPosition.x, vecPosition.y, vecPosition.z, "hinttype", "13" );
				++iNodes;
			}
		}
	}

	DevMsg("Se han creado %i nodos de ayuda... \n", iNodes);
}

//=========================================================
// Crea un nodo
//=========================================================
CNodeEnt *CNodesGeneration::CreateNode( const char *pType, const Vector &vecOrigin )
{
	return NULL;

	CNodeEnt *pNode = (CNodeEnt *)CBaseEntity::CreateNoSpawn( pType, vecOrigin, vec3_angle );
	pNode->m_NodeData.nWCNodeID	= g_pAINetworkManager->GetEditOps()->m_nNextWCIndex;
	pNode->m_debugOverlays		|= OVERLAY_WC_CHANGE_ENTITY;
	pNode->Spawn();

	DevMsg(2, "[CNodesGeneration] Nodo creado en <%f, %f, %f> \n", vecOrigin.x, vecOrigin.y, vecOrigin.z);

	return pNode;
}

//=========================================================
//=========================================================
void C_RebuildNetwork()
{
	g_pAINetworkManager->m_bNeedGraphRebuild = true;
	g_pAINetworkManager->RebuildNetworkGraph();
}

//=========================================================
//=========================================================
void C_SaveNodes()
{
	DevMsg("Guardando %i Nodos... \n", g_pAINetworkManager->GetNetwork()->NumNodes() );

	// HACK
	g_pAINetworkManager->m_bDontSaveGraph		= false;
	

	// Reconstruimos la red y guardamos el archivo de nodos
	g_pAINetworkManager->SaveNetworkGraph();

	// HACK
	g_pAINetworkManager->m_bDontSaveGraph		= true;
	g_pAINetworkManager->m_bNeedGraphRebuild	= false;
	g_pAINetworkManager->gm_fNetworksLoaded		= true;

	DevMsg("Guardado completado. \n");
	engine->ServerCommand("reload \n");
}

//=========================================================
//=========================================================
void C_GenerateNodes()
{
	NodesGen()->Start();
}

ConCommand ai_generate_nodes("ai_generate_nodes", C_GenerateNodes, "Genera nodos de movimiento a partir de la red de navegacion");
//ConCommand ai_regenerate_nodes("ai_regenerate_nodes", C_RebuildNetwork, "");
//ConCommand ai_save_nodes("ai_save_nodes", C_SaveNodes, "");