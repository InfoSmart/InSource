//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef DIRECTOR_SPAWN
#define DIRECTOR_SPAWN

//====================================================================
// >> El Director creará un recurso de esta entidad
//====================================================================
class CDirectorSpawn : public CLogicalEntity
{
public:
	DECLARE_CLASS( CDirectorSpawn, CLogicalEntity );
	DECLARE_DATADESC();

	virtual void Spawn();
	virtual void LoadItemsList();

	virtual void Precache();
	virtual void Think();

	virtual void SpawnItem();
	virtual bool CanSpawnItem();

	virtual const char *GetItem( int iKey );
	virtual const char *GetRandomItem();

	virtual void InputForceSpawn( inputdata_t &inputdata );

protected:
	string_t m_nItemsList;
	CUtlDict<const char *> m_nItems;

	bool m_bForceSpawn;
	bool m_bCanSpawn;
	int m_iSpawnCount;

	CountdownTimer m_nNextSpawn;
};

#endif