//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_player.h"

#include "in_gamerules.h"

#ifdef APOCALYPSE
	#include "ap_gamerules.h"
	#include "ap_survival_gamerules.h"
	
	#include "ap_player.h"
	#include "ap_player_infected.h"
#endif

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
//=========================================================
void ClientPutInServer( edict_t *pEdict, const char *pPlayerName )
{
	CIN_Player *pPlayer = CIN_Player::CreatePlayer( "player", pEdict );
	pPlayer->SetPlayerName( pPlayerName );
}

//=========================================================
//=========================================================
void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	CIN_Player *pPlayer = ToInPlayer( CBaseEntity::Instance( pEdict ) );
	
	pPlayer->InitialSpawn();

	if ( !bLoadGame )
		pPlayer->Spawn();

	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof(sName) );

	// Verificamos el nombre del jugador y eliminamos cualquier %
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		if ( *pApersand == '%' )
			*pApersand = ' ';
	}

	// Se ha conectado...
	//UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );
}

//=========================================================
//=========================================================
void ClientFullyConnect( edict_t *pEntity )
{
}

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *GetGameDescription()
{
	if ( g_pGameRules )
		return g_pGameRules->GetGameDescription();
	else
		return "InSource";
}

//=========================================================
//=========================================================
PRECACHE_REGISTER_BEGIN( GLOBAL, ClientGamePrecache )
	PRECACHE( MODEL, "models/player.mdl");
	PRECACHE( KV_DEP_FILE, "resource/ParticleEmitters.txt" )
PRECACHE_REGISTER_END()

//=========================================================
// Objetos del cliente que deben ser guardados
// en caché.
//=========================================================
void ClientGamePrecache()
{
	// Materials used by the client effects
	CBaseEntity::PrecacheModel( "sprites/white.vmt" );
	CBaseEntity::PrecacheModel( "sprites/physbeam.vmt" );
	CBaseEntity::PrecacheModel( "sprites/glow01.vmt" );
}

//=========================================================
// Intenta renacer a un jugador
//
// NOTE: fCopyCorpse es necesario por player.cpp
//=========================================================
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	if ( gpGlobals->coop || gpGlobals->deathmatch )
	{
		// respawn player
		pEdict->Spawn();
	}
	else
	{
		// restart the entire server
		engine->ServerCommand("reload\n");
	}
}

//=========================================================
//=========================================================
void GameStartFrame()
{
	VPROF( "GameStartFrame" );

	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;
}

//=========================================================
// Instala las reglas del juego.
//=========================================================
void InstallGameRules()
{
	/*

	else if ( gpGlobals->teamplay )
		CreateGameRulesObject( "CSurvivalTimeGameRules" );

	else if ( gpGlobals->coop )
		CreateGameRulesObject( "CCoopGameRules" );

	else*/


#ifdef APOCALYPSE
	/*if ( gpGlobals->deathmatch )
		CreateGameRulesObject( "CAP_SurvivalGameRules" );
	else*/
		CreateGameRulesObject( "CAPGameRules" );
#else
	CreateGameRulesObject( "CInGameRules" );
#endif
}