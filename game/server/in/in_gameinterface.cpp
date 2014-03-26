//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"

#include "in_gamerules.h"
#include "director.h"
#include "fmtstr.h"

#include "nodes_generation.h"

bool g_bOfflineGame = false;
extern const char *COM_GetModDirectory( void );
extern ConVar sv_force_transmit_ents;

//==============================================
// Limite de jugadores
//==============================================
void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	minplayers			= 1;
	maxplayers			= MAX_PLAYERS;
	defaultMaxPlayers	= 2;
}

//==============================================
// Inicio del mapa.
//==============================================
void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
	// Las reglas del juego nos indican que debe haber un Director
	/*if ( InRules->HasDirector() )
	{
		CDirector *pDirector = (CDirector *)gEntList.FindEntityByClassname( NULL, "info_director" );

		// No hay un Director en el mapa
		if ( !pDirector )
		{
			// Creamos al Director
			pDirector = (CDirector *)CBaseEntity::CreateNoSpawn( "info_director", vec3_origin, vec3_angle );
			pDirector->SetName( MAKE_STRING("@director") );
			
			DispatchSpawn( pDirector );
			pDirector->Activate();
		}
	}*/

	// Tenemos pendiente la creaci�n de nodos
	if ( ai_generate_nodes_pendient.GetBool() )
	{
		engine->ServerCommand("ai_generate_nodes\n");
		ai_generate_nodes_pendient.SetValue( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called to apply lobby settings to a dedicated server
//-----------------------------------------------------------------------------
void CServerGameDLL::ApplyGameSettings( KeyValues *pKV )
{
	if ( !pKV )
		return;

	// Fix the game settings request when a generic request for
	// map launch comes in (it might be nested under reservation
	// processing)
	bool bAlreadyLoadingMap = false;
	char const *szBspName = NULL;
	const char *pGameDir = COM_GetModDirectory();
	if ( !Q_stricmp( pKV->GetName(), "::ExecGameTypeCfg" ) )
	{
		if ( !engine )
			return;

		char const *szNewMap = pKV->GetString( "map/mapname", "" );
		if ( !szNewMap || !*szNewMap )
			return;

		KeyValues *pLaunchOptions = engine->GetLaunchOptions();
		if ( !pLaunchOptions )
			return;

		if ( FindLaunchOptionByValue( pLaunchOptions, "changelevel" ) ||
			FindLaunchOptionByValue( pLaunchOptions, "changelevel2" ) )
			return;

		if ( FindLaunchOptionByValue( pLaunchOptions, "map_background" ) )
		{

			return;
		}

		bAlreadyLoadingMap = true;

		if ( FindLaunchOptionByValue( pLaunchOptions, "reserved" ) )
		{
			// We are already reserved, but we still need to let the engine
			// baseserver know how many human slots to allocate
			pKV->SetInt( "members/numSlots", g_bOfflineGame ? 1 : 4 );
			return;
		}

		pKV->SetName( pGameDir );
	}

	if ( Q_stricmp( pKV->GetName(), pGameDir ) || bAlreadyLoadingMap )
		return;

	//g_bOfflineGame = pKV->GetString( "map/offline", NULL ) != NULL;
	g_bOfflineGame = !Q_stricmp( pKV->GetString( "system/network", "LIVE" ), "offline" );

	//Msg( "GameInterface reservation payload:\n" );
	//KeyValuesDumpAsDevMsg( pKV );

	// Vitaliy: Disable cheats as part of reservation in case they were enabled (unless we are on Steam Beta)
	if ( sv_force_transmit_ents.IsFlagSet( FCVAR_DEVELOPMENTONLY ) &&	// any convar with FCVAR_DEVELOPMENTONLY flag will be sufficient here
		sv_cheats && sv_cheats->GetBool() )
	{
		sv_cheats->SetValue( 0 );
	}

	// Between here and the last part: apply settings...
	char const *szMapCommand = pKV->GetString( "map/mapcommand", "map" );

	const char *szMode = pKV->GetString( "game/mode", "sdk" );

#if 0
	char const *szGameMode = pKV->GetString( "game/mode", "" );
	if ( szGameMode && *szGameMode )
	{
		extern ConVar mp_gamemode;
		mp_gamemode.SetValue( szGameMode );
	}
#endif // 0

	if ( !Q_stricmp( szMode, "sdk" ) )
	{
		szBspName = pKV->GetString( "game/mission", NULL );
		if ( !szBspName )
		{
			Warning( "ApplyGameSettings: no map specified in sdk mode!\n" );
			return;
		}

		engine->ServerCommand( CFmtStr( "%s %s reserved\n",
			szMapCommand,
			szBspName ) );
	}
	else
	{
		Warning( "ApplyGameSettings: Unknown game mode!\n" );
	}
}