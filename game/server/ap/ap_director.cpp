//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Inteligencia artificial encargada de la creación de enemigos (hijos)
// además de poder controlar la música, el clima y otros aspectos
// del juego.
//
//==========================================================//

#include "cbase.h"
#include "ap_director.h"

#include "director.h"
#include "director_manager.h"
#include "ap_director_music.h"

#include "in_player.h"
#include "players_manager.h"

#include "in_gamerules.h"
#include "in_utils.h"

#include "ai_basenpc.h"

CAP_Director g_Director;
CAP_Director *Director = &g_Director;

//=========================================================
// Comandos de consola
//=========================================================

ConVar ap_director_cruel( "ap_director_cruel", "0", FCVAR_SERVER );

//=========================================================
// Macros
//=========================================================

#define IS_CRUEL ap_director_cruel.GetBool()

BEGIN_SCRIPTDESC( CAP_Director, CDirector, SCRIPT_SINGLETON "El Director de Apocalypse" )

	

END_SCRIPTDESC();

//=========================================================
// Constructor
//=========================================================
void CAP_Director::LevelInitPreEntity()
{
	BaseClass::LevelInitPreEntity();
}

//=========================================================
// Devuelve la distancia máxima de creación
//=========================================================
float CAP_Director::MaxDistance()
{
	float flDistance = director_max_distance.GetFloat();

	// Disminumos la distancia
	if ( InRules->IsSkillLevel(SKILL_HARD) )
		flDistance -= 300;

	// Apocalypse: Aumentamos la distancia
	//if ( IsStatus(ST_COMBAT) || IsStatus(ST_FINALE) )
		//flDistance += 1500;

	return flDistance;
}

//=========================================================
// Revela la última posición de los Jugadores al Hijo
//=========================================================
void CAP_Director::Disclose( CBaseEntity *pEntity )
{
	// No es un NPC
	if ( !pEntity->IsNPC() )
		return;

	CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();

	// Ya tiene un enemigo
	if ( pNPC->GetEnemy() )
	{
		pNPC->UpdateEnemyMemory( pNPC->GetEnemy(), pEntity->GetEnemy()->GetAbsOrigin() );
		return;
	}
	
	// Obtenemos un jugador al azar
	CIN_Player *pPlayer = PlysManager->GetRandom();

	// Este es perfecto
	if ( pPlayer )
	{
		pNPC->UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin() );
	}
}

//=========================================================
// Devuelve el modo de trabajo del Director
//=========================================================
bool CAP_Director::IsMode( int iStatus )
{
	// Estamos jugando en "Cruel"
	if ( IS_CRUEL && iStatus == DIRECTOR_MODE_CRUEL )
		return true;

	if ( iStatus == DIRECTOR_MODE_PASIVE )
		return true;

	return false;
}

//=========================================================
// Establece el estado actual
//=========================================================
void CAP_Director::SetStatus( DirectorStatus iStatus )
{
	// Estamos saliendo del combate
	if ( IsStatus(ST_COMBAT) && iStatus != ST_FINALE && iStatus != ST_COMBAT )
	{
		EmitPlayerSound( PL_HURRAH );
	}

	// Estamos entrando en combate
	else if ( iStatus == ST_COMBAT )
	{
		EmitPlayerSound( PL_INCOMING );
	}

	BaseClass::SetStatus( iStatus );
}

//=========================================================
// Devuelve la cantidad máxima de hijos que se pueden crear
//=========================================================
int CAP_Director::MaxChilds()
{
	return BaseClass::MaxChilds();
}

//=========================================================
// Verifica los hijos que siguen vivos
//=========================================================
void CAP_Director::CheckChilds()
{
	BaseClass::CheckChilds();

	// Jugador: Atras, atras...
	//if ( m_iCommonInDangerZone > 4 && m_iCommonVisibles > 4 )
		//EmitPlayerSound( PL_BACKUP );
}

//=========================================================
// Devuelve si el Director le ha proporcionado información
// acerca de un enemigo al Hijo
//=========================================================
bool CAP_Director::CheckNewEnemy( CBaseEntity *pEntity )
{
	// En Climax usamos el código original
	if ( IsStatus(ST_FINALE) )
		return BaseClass::CheckNewEnemy( pEntity );

	return false;
}

//=========================================================
// [Evento] Hemos creado un hijo
//=========================================================
void CAP_Director::OnSpawnChild( CBaseEntity *pEntity )
{
	BaseClass::OnSpawnChild( pEntity );

	// Solo podemos dar un enemigo si estamos en un evento de Pánico
	if ( IsStatus(ST_COMBAT) )
	{
		Disclose( pEntity );
	}
}

//=========================================================
// Activa el estado de Pánico
//=========================================================
void CAP_Director::StartCombat( CBaseEntity *pActivator, int iWaves, bool bInfinite )
{
	BaseClass::StartCombat( pActivator, iWaves, bInfinite );

	CBaseEntity *pChild = NULL;

	// Revelamos la posición a los Hijos vivos
	do
	{
		pChild = gEntList.FindEntityByName( pChild, "director_*" );

		// No existe
		if ( pChild == NULL )
			continue;

		// Esta muerto, no nos sirve
		if ( !pChild->IsAlive() )
			continue;

		Disclose( pChild );

	} while ( pChild );
}

//=========================================================
// Selecciona un Jugador al azar para reproducir una oración
//=========================================================
void CAP_Director::EmitPlayerSound( PlayerSound pSound )
{
	if ( !InRules->IsMultiplayer() )
		return;

	int iLastTime = m_iLastPlayerSound[ pSound ];

	// Ya hemos reproducido esta oración
	if ( iLastTime > 0 )
	{
		// Hace menos de 10s
		if ( gpGlobals->curtime <= iLastTime + 10 )
			return;
	}

	CIN_Player *pPlayers[ 38 ];
	int iPlayers = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = PlysManager->Get( i );

		if ( !pPlayer )
			continue;
		
		// No esta vivo
		if ( !pPlayer->IsAlive() )
			continue;

		// Esta incapacitado
		if ( pPlayer->IsIncap() )
			continue;

		// Agregamos el Jugador a la lista de los posibles
		pPlayers[ iPlayers ] = pPlayer;
		++iPlayers;
	}

	// No hubo nadie...
	if ( iPlayers <= 0 )
		return;

	CIN_Player *pSoundPlayer;

	// Seleccionamos un Jugador al azar
	if ( iPlayers == 1 )
		pSoundPlayer = pPlayers[ 0 ];
	else
		pSoundPlayer = pPlayers[ RandomInt(0, iPlayers-1) ];

	// Reproducimos
	pSoundPlayer->EmitPlayerSound( m_nPlayersSounds[pSound] );

	// La última vez que he reproducido este sonido
	m_iLastPlayerSound[ pSound ] = gpGlobals->curtime;
}