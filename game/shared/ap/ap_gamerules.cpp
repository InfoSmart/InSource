//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ap_gamerules.h"

#ifndef CLIENT_DLL
	#include "ap_player.h"
	#include "ap_player_infected.h"

	extern void ClientActive( edict_t *pEdict, bool bLoadGame );
#endif

//=========================================================
// Comandos
//=========================================================

//=========================================================
// Información y Red
//=========================================================

REGISTER_GAMERULES_CLASS( CAPGameRules );

// DT_InGameRules
BEGIN_NETWORK_TABLE_NOBASE( CAPGameRules, DT_APGameRules )
END_NETWORK_TABLE()

//=========================================================
// Constructor
//=========================================================
CAPGameRules::CAPGameRules() : BaseClass()
{
	InRules = this;
}

#ifndef CLIENT_DLL

//=========================================================
// Convierte un Jugador en Infectado
//=========================================================
void CAPGameRules::ConvertToInfected( CBasePlayer *pPlayer )
{
	// Destruimos
	ClientDestroy( pPlayer );

	// Seguia vivo
	if ( pPlayer->IsAlive() )
		pPlayer->CommitSuicide();

	edict_t *pEdict			= pPlayer->edict();
	const char *pPlayerName = pPlayer->GetPlayerName();

	//UTIL_RemoveImmediate( pPlayer );

	// Transformamos
	CAP_PlayerInfected *pNewPlayer = CAP_PlayerInfected::CreateInfectedPlayer( "player_infected", pEdict );
	pNewPlayer->SetPlayerName( pPlayerName );

	// Spawn
	ClientActive( pEdict, false );
}

//=========================================================
// Convierte un Jugador en Humano
//=========================================================
void CAPGameRules::ConvertToHuman( CBasePlayer *pPlayer )
{
	// Destruimos
	ClientDestroy( pPlayer );

	// Seguia vivo
	if ( pPlayer->IsAlive() )
		pPlayer->CommitSuicide();

	edict_t *pEdict			= pPlayer->edict();
	const char *pPlayerName = pPlayer->GetPlayerName();

	//UTIL_RemoveImmediate( pPlayer );

	// Transformamos
	CIN_Player *pNewPlayer = CIN_Player::CreatePlayer( "player", pEdict, pPlayerName );

	// Spawn
	ClientActive( pEdict, false );
}

//=========================================================
// Deuelve si el Jugador puede ser abatido
//=========================================================
bool CAPGameRules::FPlayerCanDejected( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	// No hay incapacitación en Apocalypse
	return false;
}

#endif // CLIENT_DLL