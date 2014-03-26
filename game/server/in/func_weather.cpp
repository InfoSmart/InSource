//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "func_weather.h"
#include "precipitation_shared.h"
#include "players_manager.h"

//=========================================================
// Comandos
//=========================================================

//=========================================================
// Información y Red
//=========================================================

LINK_ENTITY_TO_CLASS( func_weather, CWeather );

BEGIN_DATADESC( CWeather )
END_DATADESC()

//=========================================================
// Creación
//=========================================================
void CWeather::Spawn()
{
	BaseClass::Spawn();

	// Somos invisibles
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING(GetModelName()) );

	// Creamos un func_precipitation justo en esta ubicación
	CBaseEntity *pPrecipitation = CreateEntityByName( "func_precipitation" );
	pPrecipitation->SetAbsOrigin( GetAbsOrigin() );
	pPrecipitation->SetAbsAngles( GetAbsAngles() );
	pPrecipitation->SetModel( STRING(GetModelName()) );
	pPrecipitation->KeyValue("preciptype", PRECIPITATION_TYPE_RAIN);

	DispatchSpawn( pPrecipitation );
	pPrecipitation->Activate();
}

//=========================================================
// 
//=========================================================
void CWeather::Think()
{
	BaseClass::Think();

	PlysManager->ExecCommand( "r_RainSimulate 1" );
	PlysManager->ExecCommand( "r_DrawRain 1" );
}