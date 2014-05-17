//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef DIRECTOR_MANAGER_H
#define DIRECTOR_MANAGER_H

#pragma once

#include "ai_basenpc.h"
#include "ai_network.h"
#include "ai_node.h"

#include "in_shareddefs.h"
#include "directordefs.h"

#define BAN_DURATION_DEFAULT		20
#define BAN_DURATION_UNREACHABLE	60

//=========================================================
// >> CDirectorManager
// Procesa la creación de hijos
//=========================================================
class CDirectorManager
{
public:
	DECLARE_CLASS_NOBASE( CDirectorManager );

	// Principales
	virtual void Init();
	virtual void OnNewMap();

	virtual void LoadPopulation();
	virtual void Precache();

	// Utilidades
	virtual int GetCandidateCount();
	virtual bool IsUsingNavMesh();

	virtual void Disclose();
	virtual void KillChilds( bool bOnlyNoVisible = false );
	virtual bool CanUseNavArea( CNavArea *pArea );

	// Ubicación
	virtual void Update();
	virtual void UpdateNavPoints();
	virtual void UpdateNodes();

	virtual bool GetSpawnLocation( Vector *vecPosition, int iType = DIRECTOR_CHILD );

	// Baneo
	virtual void UnBan( CNavArea *pArea );
	virtual void UnBan( const Vector vecPosition );

	virtual void AddBan( CNavArea *pArea, float flDuration = BAN_DURATION_DEFAULT );
	virtual void AddBan( const Vector vecPosition, float flDuration = BAN_DURATION_DEFAULT );

	virtual void ClearBans();
	virtual bool IsBanned( CNavArea *pArea );

	// Verificación
	virtual bool CanMake( const Vector &vecPosition );
	virtual bool PostSpawn( CBaseEntity *pEntity, Hull_t pHull = HULL_HUMAN );

	// Creación
	virtual const char *GetChildName( int iType );
	virtual const char *GetRandomClass( int iType );
	virtual void SetupChild( CAI_BaseNPC *pChild );

	virtual void StartSpawn( int iType = DIRECTOR_CHILD );

	virtual bool SpawnChild( int iType, const Vector &vecPosition );
	virtual void SpawnChild( int iType );

	virtual void SpawnBatch( int iType, const Vector &vecPosition );
	virtual void SpawnBatch( int iType, int iNumber );
	virtual void SpawnBatch( int iType );

	virtual bool MakeSpawn( const char *pClass, int iType, const Vector &vecPosition );

protected:
	// Hijos
	CUtlDict<int> m_hChilds;
	CUtlDict<int> m_hBoss;
	CUtlDict<int> m_hSpecials;
	CUtlDict<int> m_hAmbient;

	// Nodos
	CUtlVector<CNavArea *> m_hSpawnAreas;
	CUtlVector<CAI_Node *> m_hSpawnNodes;

	CUtlVector<CNavArea *> m_hForceSpawnAreas;
	CUtlVector<CNavArea *> m_hAmbientSpawnAreas;

	bool m_bSpawnInForceAreas;
	int m_iBannedAreas[ MAX_NAV_AREAS ];

	// Cronometros
	CountdownTimer m_hCandidateUpdateTimer;

	friend class CDirector;
	friend class CInfoDirector;
};

extern CDirectorManager *DirectorManager;

#endif // DIRECTOR_MANAGER_H