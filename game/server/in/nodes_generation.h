//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef NODES_GENERATION_H
#define NODES_GENERATION_H

#pragma once

class CNodeEnt;

extern ConVar ai_generate_nodes_pendient;

//=========================================================
// >> CNodesGeneration
//=========================================================
class CNodesGeneration
{
public:
	virtual void Start();

	virtual void GenerateWalkableNodes();
	virtual void GenerateClimbNodes();
	virtual void GenerateHintNodes();

	virtual CNodeEnt *CreateNode( const char *pType, const Vector &vecOrigin );

protected:
	int iNavAreaCount;

	int m_iWalkNodesGenerated;
	CUtlVector< Vector > m_vecWalkLocations;
};

CNodesGeneration *NodesGen();

#endif // NODES_GENERATION_H