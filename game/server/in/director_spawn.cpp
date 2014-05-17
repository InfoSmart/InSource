//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "director.h"
#include "director_spawn.h"

#include "players_manager.h"
#include "director_item_manager.h"

#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"

//====================================================================
// Comandos
//====================================================================

ConVar director_items_min_interval( "director_items_min_interval", "10", FCVAR_CHEAT );
ConVar director_items_max_interval( "director_items_max_interval", "30", FCVAR_CHEAT );

#define MIN_INTERVAL	director_items_min_interval.GetInt()
#define MAX_INTERVAL	director_items_max_interval.GetInt()
#define MAX_ITEMS		30

//====================================================================
// Información y Red
//====================================================================

LINK_ENTITY_TO_CLASS( director_spawn, CDirectorSpawn );

BEGIN_DATADESC( CDirectorSpawn )

	DEFINE_KEYFIELD( m_nItemsList, FIELD_STRING, "ItemsList" ),
	DEFINE_KEYFIELD( m_bForceSpawn, FIELD_BOOLEAN, "SpawnNow" ),
	DEFINE_KEYFIELD( m_iSpawnCount, FIELD_INTEGER, "Count" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ForceSpawn", InputForceSpawn ),

END_DATADESC()

//====================================================================
// Creación en el mundo
//====================================================================
void CDirectorSpawn::Spawn()
{
	BaseClass::Spawn();

	// Cargamos la lista de objetos
	LoadItemsList();

	// Guardamos objetos necesarios en caché
	Precache();

	// Podemos crear un objeto
	m_bCanSpawn = true;
	m_nNextSpawn.Invalidate();

	// Forzamos la creación de un objeto
	if ( m_bForceSpawn )
	{
		SpawnItem();
	}
}

//====================================================================
// Carga los objetos de la lista especificada
//====================================================================
void CDirectorSpawn::LoadItemsList()
{
	// ¿No hay nada aquí?
	if ( m_nItemsList == NULL_STRING )
	{
		Warning( "[CDirectorSpawn::LoadItemsList] [%i] No tiene una lista de objetos a crear \n", entindex() );
		m_bCanSpawn = false;

		return;
	}

	KeyValues *pFile = new KeyValues("DirectorItems");
	KeyValues::AutoDelete autoDelete( pFile );

	// Leemos el archivo
	pFile->LoadFromFile( filesystem, DIRECTOR_ITEMS_FILE, NULL );
	KeyValues *pType;

	// Pasamos por cada sección
	for ( pType = pFile->GetFirstTrueSubKey(); pType; pType = pType->GetNextTrueSubKey() )
	{
		// Esta lista no corresponde a la que la entidad quiere
		if ( !FStrEq(pType->GetName(), STRING(m_nItemsList)) )
			continue;

		KeyValues *pChildType;

		// Pasamos por cada objeto
		for ( pChildType = pType->GetFirstValue(); pChildType; pChildType = pChildType->GetNextKey() )
		{
			m_nItems.Insert( pChildType->GetName(), pType->GetString( pChildType->GetName(), "any" ) );
		}
	}

	// No hay objetos en la lista
	if ( m_nItems.Count() <= 0 )
	{
		Warning( "[CDirectorSpawn::LoadItemsList] [%i] No se han encontrado objetos en la lista. \n", entindex() );
		m_bCanSpawn = false;
	}
}

//====================================================================
// Guarda objetos necesarios en caché
//====================================================================
void CDirectorSpawn::Precache()
{
	int iObjects = m_nItems.Count();

	for ( int i = 0; i < iObjects; ++i )
		UTIL_PrecacheOther( m_nItems.GetElementName(i) );
}

//====================================================================
// Bucle de ejecución de tareas
//====================================================================
void CDirectorSpawn::Think()
{
	BaseClass::Think();

	// Estamos esperando haber cuando podemos recrear un objeto
	if ( !m_bCanSpawn && m_nNextSpawn.HasStarted() && m_nNextSpawn.IsElapsed() )
	{
		// El tiempo de espera a terminado, podemos recrear
		m_bCanSpawn = true;
		m_nNextSpawn.Invalidate();
	}
}

//====================================================================
// Devuelve si el objeto puede crearse
//====================================================================
bool CDirectorSpawn::CanSpawnItem()
{
	// No podemos crear aún
	if ( !m_bCanSpawn )
		return false;

	// Alguien te esta viendo
	if ( PlysManager->InVisibleCone(this) )
		return false;

	return true;
}

//====================================================================
// Crea el objeto
//====================================================================
void CDirectorSpawn::SpawnItem()
{
	// No podemos crear un objeto
	//if ( !CanSpawnItem() )
		//return;

	const char *pItemName = GetRandomItem();

	if ( pItemName == NULL )
	{
		Warning( "[DirectorSpawn::SpawnItem] Se ha intentado crear un objeto, pero no se ha devuelto objeto. \n" );
		return;
	}

	Warning( "[DirectorSpawn::SpawnItem] Creando %s ... \n", pItemName );
	CBaseEntity *pItem = CreateEntityByName( pItemName );

	// No se ha podido crear el objeto
	if ( !pItem )
	{
		Warning( "[DirectorSpawn::SpawnItem] El objeto %s no se ha podido crear. \n", pItemName );
		return;
	}

	// Nombre para los objetos
	pItem->SetName( AllocPooledString("director_item") );

	// Aquí mismo
	pItem->SetAbsOrigin( GetAbsOrigin() );
	pItem->SetAbsAngles( GetAbsAngles() );

	// ¡Spawn!
	DispatchSpawn( pItem );
	pItem->Activate();

	// Una creación menos
	if ( m_iSpawnCount >= 1 )
		--m_iSpawnCount;

	m_bCanSpawn = false;

	// Aún podemos recrearnos
	if ( m_iSpawnCount >= 1 )
	{
		m_nNextSpawn.Start( RandomInt(MIN_INTERVAL, MAX_INTERVAL) );
	}
}

//====================================================================
// Devuelve que objeto esta en el slot.
//====================================================================
const char *CDirectorSpawn::GetItem( int iKey )
{
	return m_nItems.GetElementName( iKey );
}

//====================================================================
// Devuelve el nombre de un objeto a crear al azar
//====================================================================
const char *CDirectorSpawn::GetRandomItem()
{
	int iFinish		= -1;
	int iObjects	= m_nItems.Count();

	for ( int i = 0; i < iObjects; ++i )
	{
		const char *pItem = GetItem(i);

		if ( pItem == NULL )
			continue;

		++iFinish;
	}

	if ( iFinish < 0 )
		return NULL;

	int iKey = RandomInt( 0, iFinish );

	if ( iFinish == 0 )
		iKey = 0;

	const char *pItem = GetItem( iKey );
	return pItem;
}

//====================================================================
// Input: 
//====================================================================
void CDirectorSpawn::InputForceSpawn( inputdata_t &inputdata )
{
	SpawnItem();
}