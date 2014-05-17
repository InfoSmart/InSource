//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"
#include "players_manager.h"

#include "in_player.h"
#include "weapon_inbase.h"

#include "ai_basenpc.h"
#include "ai_pathfinder.h"

#include "in_utils.h"

#include "tier0/memdbgon.h"

PlayersManager g_PlayersManager;
PlayersManager *PlysManager = &g_PlayersManager;

//====================================================================
// Comandos de consola
//====================================================================

extern double round( double number );

//====================================================================
// Constructor
//====================================================================
PlayersManager::PlayersManager() : CAutoGameSystemPerFrame( "PlayersManager" )
{
}

//====================================================================
// Inicia el sistema
//====================================================================
bool PlayersManager::Init()
{
	m_iTotal		= 0;
	m_iWithLife		= 0;
	m_iConnected	= 0;

	m_iHealth		= 0;
	m_flSanity		= 0;

	m_bInCombat		= false;
	m_iWithWeapons	= 0;
	m_iAmmoStatus	= STATS_POOR;

	m_iStatus		= STATS_POOR;
	m_iNextScan		= gpGlobals->curtime;

	return true;
}

//====================================================================
// Apaga el sistema
//====================================================================
void PlayersManager::Shutdown()
{
	Init();
}

//====================================================================
//====================================================================
void PlayersManager::LevelInitPreEntity()
{
	Init();
}

//====================================================================
//====================================================================
void PlayersManager::LevelInitPostEntity()
{
}

//====================================================================
//====================================================================
void PlayersManager::FrameUpdatePreEntityThink()
{
}

//====================================================================
//====================================================================
void PlayersManager::FrameUpdatePostEntityThink()
{
	if ( gpGlobals->curtime <= m_iNextScan )
		return;

	Scan( DefaultTeam() );
	m_iNextScan = gpGlobals->curtime + 1.0f;
}

//====================================================================
// Devuelve el equipo de Jugadores a escanear
//====================================================================
int PlayersManager::DefaultTeam()
{
	#ifdef APOCALYPSE
		return TEAM_HUMANS;
	#endif

	return TEAM_ANY;
}

//====================================================================
//====================================================================
void PlayersManager::ExecCommand( const char *pName )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get( i );

		if ( !pPlayer )
			continue;
		
		pPlayer->ExecCommand( pName );
	}
}

//====================================================================
// Devuelve al jugador con la ID especificada
//====================================================================
CIN_Player *PlayersManager::Get( int iIndex )
{
	// Esta ID no es válida.
	if ( iIndex <= 0 || iIndex > gpGlobals->maxClients )
		return NULL;

	CIN_Player *pPlayer		= NULL;
	edict_t *pPlayerEdict	= INDEXENT( iIndex );

	// Esta ID esta siendo ocupada por una entidad.
	if ( pPlayerEdict && !pPlayerEdict->IsFree() )
	{
		pPlayer = (CIN_Player *)GetContainingEntity(pPlayerEdict);
	}

	return pPlayer;
}

//====================================================================
// Devuelve el jugador que ha creado la partida
//====================================================================
CIN_Player *PlayersManager::GetLocal()
{
	return Get( 1 );
}

//====================================================================
// Devuelve un jugador al azar.
//====================================================================
CIN_Player *PlayersManager::GetRandom( int iTeam )
{
	int iPlayers[50]	= {};
	int iKey			= 0;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		if ( !pPlayer )
			continue;

		// No esta vivo
		if ( !pPlayer->IsAlive() )
			continue;

		// Solo queremos un jugador de un determinado equipo
		if ( iTeam != TEAM_ANY )
		{
			// Este jugador no es del equipo que queremos
			if ( pPlayer->GetTeamNumber() != iTeam )
				continue;
		}

		// Lo agregamos a los posibles
		iPlayers[ iKey ] = i;
		++iKey;
	}

	// Al parecer no hay jugadores
	if ( iKey == 0 )
		return GetLocal();

	// Seleccionamos un jugador de la lista
	int iRandomKey		= RandomInt(0, iKey);
	CIN_Player *pPlayer = Get( iRandomKey );

	// ¿El jugador no es válido?
	if ( !pPlayer )
		pPlayer = GetLocal();

	return pPlayer;
}

//====================================================================
// Devuelve el jugador más cercano a la posición indicada
//====================================================================
CIN_Player *PlayersManager::GetNear( const Vector &vPosition, float &fDistance, int iTeam, CBasePlayer *pExcept )
{
	CIN_Player *pNear	= NULL;
	fDistance			= 0.0f;
	float fNearDistance = 0.0f;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido
		if ( !pPlayer )
			continue;

		// No esta vivo
		if ( !pPlayer->IsAlive() )
			continue;

		if ( pExcept )
		{
			if ( pPlayer == pExcept )
				continue;

			if ( pExcept->IsBot() && pPlayer->IsBot() )
				continue;
		}

		// Solo queremos un jugador de un determinado equipo
		if ( iTeam != TEAM_ANY )
		{
			// Este jugador no es del equipo
			if ( pPlayer->GetTeamNumber() != iTeam )
				continue;
		}

		fNearDistance = pPlayer->GetAbsOrigin().DistTo( vPosition );

		// Es el primer jugador o esta más cerca que el anterior
		if ( fDistance == 0.0f || fNearDistance < fDistance )
		{
			fDistance	= fNearDistance;
			pNear		= pPlayer;
		}
	}

	return pNear;
}

//====================================================================
// Devuelve el jugador más cercano a la posición indicada
//====================================================================
CIN_Player *PlayersManager::GetNear( const Vector &vPosition, int iTeam )
{
	float fDistance;
	return GetNear( vPosition, fDistance, iTeam );
}

//====================================================================
// Devuelve si el NPC tiene una ruta válida hacia el jugador
//====================================================================
bool PlayersManager::HasRoute( CIN_Player *pPlayer, CAI_BaseNPC *pNPC, int iTolerance, Navigation_t pNavType )
{
	AI_Waypoint_t *pRoute = pNPC->GetPathfinder()->BuildRoute( pNPC->GetAbsOrigin(), pPlayer->GetAbsOrigin(), pPlayer, iTolerance, pNavType );

	// No se encontro una ruta válida
	if ( !pRoute )
		return false;

	return true;
}

//====================================================================
// Devuelve si el NPC tiene una ruta válida hacia todos
// los jugadores
//====================================================================
bool PlayersManager::HasRouteToAny( CAI_BaseNPC *pNPC, int iTolerance, Navigation_t pNavType )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido.
		if ( !pPlayer )
			continue;

		// Sin una ruta válida.
		if ( !PlayersManager::HasRoute(pPlayer, pNPC, iTolerance, pNavType) )
			return false;
	}

	return true;
}

//====================================================================
//====================================================================
void PlayersManager::RespawnAll()
{
	// Volvemos a crear a todos los jugadores.
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		if ( !pPlayer )
			continue;

		pPlayer->StopMusic();
		pPlayer->Spawn();
	}
}

//====================================================================
// Devuelve si hay jugadores vivos
//====================================================================
bool PlayersManager::AreLive()
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido
		if ( !pPlayer )
			continue;

		// ¡Este jugador esta vivo!
		if ( pPlayer->IsAlive() )
			return true;
	}

	return false;
}

//====================================================================
// Ejecuta la función "StopMusic" de todos los jugadores.
//====================================================================
void PlayersManager::StopMusic()
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido.
		if ( !pPlayer )
			continue;

		// Paramos la música
		pPlayer->StopMusic();
	}
}

//====================================================================
// Devuelve si el Area de Navegación ha sido visitada por
// los Jugadores
//====================================================================
bool PlayersManager::InLastKnowAreas( CNavArea *pArea )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido.
		if ( !pPlayer )
			continue;

		if ( pPlayer->InLastKnowAreas(pArea) )
			return true;
	}

	return false;
}

//====================================================================
// Devuelve si la posición esta en la visibilidad
// de algún jugador
//====================================================================
bool PlayersManager::IsVisible( const Vector &vPosition )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido.
		if ( !pPlayer )
			continue;

		#ifdef APOCALYPSE
			if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
				continue;
		#endif

		// Este punto esta visible al jugador.
		if ( pPlayer->FVisible(vPosition, MASK_BLOCKLOS) )
			return true;
	}

	return false;
}

//====================================================================
// Devuelve si la entidad esta en la visibilidad
// de algún jugador
//====================================================================
bool PlayersManager::IsVisible( CBaseEntity *pEntity )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido
		if ( !pPlayer )
			continue;

		#ifdef APOCALYPSE
			if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
				continue;
		#endif

		// Este punto esta visible al jugador
		if ( pPlayer->FVisible(pEntity, MASK_BLOCKLOS) )
			return true;
	}

	return false;
}

//====================================================================
// Devuelve si la posición esta en el cono de mira
// de algún jugador
//====================================================================
bool PlayersManager::InViewcone( const Vector &vPosition )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido
		if ( !pPlayer )
			continue;

		#ifdef APOCALYPSE
			if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
				continue;
		#endif

		// Este punto esta visible al jugador
		if ( pPlayer->FInViewCone(vPosition) )
			return true;
	}

	return false;
}

//====================================================================
// Devuelve si la entidad esta en el cono de mira
// de algún jugador
//====================================================================
bool PlayersManager::InViewcone(CBaseEntity *pEntity)
{
	return InViewcone( pEntity->WorldSpaceCenter() );
}

//====================================================================
// Devuelve si la ubicación se encuentra a la vista o en el
// cono de visibilidad de algún jugador
//====================================================================
bool PlayersManager::InVisibleCone(const Vector &vPosition)
{
	return ( IsVisible(vPosition) && InViewcone(vPosition) );
}

//====================================================================
// Devuelve si la entidad se encuentra a la vista o en el
// cono de visibilidad de algún jugador
//====================================================================
bool PlayersManager::InVisibleCone(CBaseEntity *pEntity)
{
	return ( IsVisible(pEntity) && InViewcone(pEntity) );
}

//====================================================================
//====================================================================
void PlayersManager::SendLesson( const char *pLesson, CIN_Player *pPlayer, bool bOnce, CBaseEntity *pSubject )
{
	// Solo debemos enviarlo una vez por vida
	if ( bOnce )
	{
		if ( pPlayer->m_nLessonsList.HasElement(pLesson) )
			return;
	}

	IGameEvent *pEvent = Utils::CreateLesson( pLesson, pSubject );

	if ( pEvent )
	{
		// Solo una vez
		if ( bOnce )
		{
			pPlayer->m_nLessonsList.AddToTail( pLesson );
		}

		pEvent->SetInt( "userid", pPlayer->GetUserID() );
		gameeventmanager->FireEvent( pEvent );
	}
}

//====================================================================
//====================================================================
void PlayersManager::SendAllLesson( const char *pLesson, bool bOnce, CBaseEntity *pSubject )
{
	if ( bOnce )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CIN_Player *pPlayer = Get(i);

			if ( !pPlayer )
				continue;

			SendLesson( pLesson, pPlayer, bOnce, pSubject );
		}

		return;
	}

	IGameEvent *pEvent = Utils::CreateLesson( pLesson, pSubject );

	if ( pEvent )
	{
		gameeventmanager->FireEvent( pEvent );
	}
}

//====================================================================
// Devuelve del 1 al 100 el porcentaje de salud de todos
// los jugadores
//====================================================================
int PlayersManager::GetHealth()
{
	int iHealth		= 0;
	int iPlayers	= 0;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido
		if ( !pPlayer )
			continue;

		// 
		if ( !pPlayer->IsAlive() || pPlayer->IsObserver() )
			continue;

		// Incluimos la salud de este jugador
		++iPlayers;
		iHealth += pPlayer->GetHealth();
	}

	// Dividimos la salud de todos los por jugadores.
	if ( iHealth > 0 )
		iHealth = iHealth / iPlayers;

	return iHealth;
}

//====================================================================
// Escanea a los recursos de los jugadores
//====================================================================
void PlayersManager::Scan( int iTeam )
{
	// Reinciamos las variables
	m_iTotal		= 0;
	m_iWithLife		= 0;
	m_iConnected	= 0;
	m_iHealth		= 0;
	m_bInCombat		= false;
	m_iWithWeapons	= 0;

	int iAmmoPoints		= 0;
	int iHealthTotal	= 0;
	float flSanityTotal	= 0;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = Get(i);

		// Jugador inválido
		if ( !pPlayer )
			continue;

		// Jugador conectado
		++m_iConnected;

		// Solo contamos a los de un equipo
		if ( iTeam != TEAM_ANY )
		{
			if ( pPlayer->GetTeamNumber() != iTeam )
				continue;
		}

		++m_iTotal;

		// No esta vivo
		if ( !pPlayer->IsAlive() || pPlayer->IsObserver() )
			continue;

		++m_iWithLife;
		
		// Salud
		iHealthTotal += pPlayer->GetHealth();

		// Cordura
		flSanityTotal += pPlayer->GetSanity();

		// Incapacitado
		if ( pPlayer->IsIncap() )
			++m_iDejected;

		// Esta en combate
		if ( pPlayer->InCombat() )
			m_bInCombat = true;

		CBaseInWeapon *pWeapon = pPlayer->GetActiveInWeapon();

		// Tiene un arma
		if ( pWeapon )
		{
			++m_iWithWeapons;

			// Utiliza munición primaria
			if ( pWeapon->UsesClipsForAmmo1() )
			{
				int iClip		= pWeapon->Clip1();
				int iMaxClip	= pWeapon->GetMaxClip1();
				int iAmmo		= pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );

				//
				// Clip actua del arma
				//

				// Todavía tiene balas
				if ( iClip >= (iMaxClip-10) )
					iAmmoPoints += 1;

				//
				// Munición total
				//

				if ( iAmmo >= 150 )
					iAmmoPoints += 3;

				if ( iAmmo >= 100 )
					iAmmoPoints += 2;

				if ( iAmmo >= 50 )
					iAmmoPoints += 1;
			}
		}
	}

	if ( m_iWithLife > 0 )
	{
		m_iHealth		= round( iHealthTotal / m_iWithLife );
		m_flSanity		= round( flSanityTotal / m_iWithLife );
		m_iAmmoStatus	= ( m_iWithWeapons > 0 ) ? round( iAmmoPoints / m_iWithWeapons ) : STATS_POOR;

		// Decidimos el estado de los jugadores
		Decide();
	}
}

//====================================================================
// Calcula el estado general de los jugadores según lo que
// tengan en recursos
//
// TODO: Algo mejor
//====================================================================
void PlayersManager::Decide()
{
	int iPoints = 0;

	// Hay dos o más jugadores
	if ( m_iTotal >= 2 )
	{
		// Jugadores vivos
		iPoints += ( m_iWithLife / m_iTotal ) * 20;
	}
	else
	{
		if ( !m_bInCombat )
			iPoints += 10;

		iPoints += 10;
	}
	
	// No estan en combate
	if ( !m_bInCombat )
		iPoints += 10;

	// Jugadores con armas
	iPoints += ( m_iWithWeapons / m_iTotal ) * 10;

	// Estado de la munición
	iPoints += ( m_iAmmoStatus / 2 ) * 5; // 10 pts

	// Estado de la salud
	iPoints += ( ( m_iHealth / m_iTotal ) / 100 ) * ( 20 * m_iTotal );

	// Estado de la cordura
	iPoints += ( ( m_flSanity / m_iTotal ) / 100 ) * ( 10 * m_iTotal );

	// Jugadores incapacitados
	if ( m_iDejected > 0 )
	{
		// TODO: Algo mejor
		iPoints += ( m_iTotal / m_iDejected ) * ( 8 / m_iTotal );
	}
	else
	{
		iPoints += 10;
	}

	// TODO: 10%

	// Estado final
	m_iStatus = ( iPoints / 25 );
}