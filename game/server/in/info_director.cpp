//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"
#include "info_director.h"

#include "director.h"
#include "director_manager.h"
#include "director_music.h"

#ifdef APOCALYPSE
	#include "ap_director.h"
	#include "ap_director_music.h"
#endif

#include "in_player.h"
#include "players_manager.h"
#include "in_gamerules.h"


//=========================================================
// Información y Red
//=========================================================

LINK_ENTITY_TO_CLASS( info_director, CInfoDirector );

BEGIN_DATADESC( CInfoDirector )

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "HowAngry", InputHowAngry ),
	DEFINE_INPUTFUNC( FIELD_VOID, "WhatsYourStatus", InputWhatsYourStatus ),

	DEFINE_INPUTFUNC( FIELD_VOID, "HowAliveChilds", InputHowAliveChilds ),
	DEFINE_INPUTFUNC( FIELD_VOID, "HowAliveSpecials", InputHowAliveSpecials ),
	DEFINE_INPUTFUNC( FIELD_VOID, "HowAliveBoss", InputHowAliveBoss ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ForceRelax", InputForceRelax ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ForcePanic", InputForcePanic ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceInfinitePanic", InputForceInfinitePanic ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceBoss", InputForceBoss ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceClimax", InputForceClimax ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetPopulation", InputSetPopulation ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisclosePlayers", InputDisclosePlayers ),

	DEFINE_INPUTFUNC( FIELD_VOID, "KillChilds", InputKillChilds ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KillNoVisibleChilds", InputKillNoVisibleChilds ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SetSpawnInForceAreas", InputSetSpawnInForceAreas ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetSpawnInNormalAreas", InputSetSpawnInNormalAreas ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartSurvivalTime", InputStartSurvivalTime ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DisableMusic", InputDisableMusic ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableMusic", InputEnableMusic ),
	
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxAliveBoss", InputSetMaxAliveBoss ),

	// Outputs
	DEFINE_OUTPUT( OnResponseAngry, "OnResponseAngry" ),
	DEFINE_OUTPUT( OnResponseStatus, "OnResponseStatus" ),

	DEFINE_OUTPUT( OnResponseAliveChilds, "OnResponseAliveChilds" ),
	DEFINE_OUTPUT( OnResponseAliveSpecials, "OnResponseAliveSpecials" ),
	DEFINE_OUTPUT( OnResponseAliveBoss, "OnResponseAliveBoss" ),

	DEFINE_OUTPUT( OnSpawnChild,	"OnSpawnChild" ),
	DEFINE_OUTPUT( OnSpawnBoss,		"OnSpawnBoss" ),
	DEFINE_OUTPUT( OnSpawnSpecial,	"OnSpawnSpecial" ),
	DEFINE_OUTPUT( OnSpawnAmbient,	"OnSpawnAmbient" ),
	DEFINE_OUTPUT( OnSleep,			"OnSleep" ),
	DEFINE_OUTPUT( OnRelaxed,		"OnRelaxed" ),
	DEFINE_OUTPUT( OnPanic,			"OnPanic" ),
	DEFINE_OUTPUT( OnBoss,			"OnBoss" ),
	DEFINE_OUTPUT( OnClimax,		"OnClimax" ),

END_DATADESC()

//=========================================================
// Creación en el mapa
//=========================================================

//=========================================================
// Director ¿Que tan enojado estas?
//=========================================================
void CInfoDirector::InputHowAngry( inputdata_t &inputdata )
{
	OnResponseAngry.Set( Director->m_iAngry, inputdata.pActivator, inputdata.pCaller );
}

//=========================================================
// Director ¿Cual es tu estado?
//=========================================================
void CInfoDirector::InputWhatsYourStatus( inputdata_t &inputdata )
{
	OnResponseStatus.Set( Director->m_iStatus, inputdata.pActivator, inputdata.pCaller );
}

//=========================================================
// Director ¿Cuantos hijos estan vivos?
//=========================================================
void CInfoDirector::InputHowAliveChilds( inputdata_t &inputdata )
{
	OnResponseAliveChilds.Set( Director->m_iChildsAlive, inputdata.pActivator, inputdata.pCaller );
}

//=========================================================
// Director ¿Cuantos hijos especiales estan vivos?
//=========================================================
void CInfoDirector::InputHowAliveSpecials( inputdata_t &inputdata )
{
	OnResponseAliveSpecials.Set( Director->m_iSpecialsAlive, inputdata.pActivator, inputdata.pCaller );
}

//=========================================================
// Director ¿Cuantos jefes estan vivos?
//=========================================================
void CInfoDirector::InputHowAliveBoss( inputdata_t &inputdata )
{
	OnResponseAliveBoss.Set( Director->m_iBossAlive, inputdata.pActivator, inputdata.pCaller );
}

void CInfoDirector::InputForceRelax( inputdata_t &inputdata )
{
	Director->Relaxed();
}

void CInfoDirector::InputForcePanic( inputdata_t &inputdata )
{
	CBasePlayer *pActivator = NULL;

	if ( inputdata.pActivator->IsPlayer() )
	{
		pActivator = ToBasePlayer( inputdata.pActivator );
	}

	Director->Panic( pActivator, inputdata.value.Int() );
}

void CInfoDirector::InputForceInfinitePanic( inputdata_t &inputdata )
{
	CBasePlayer *pActivator = NULL;

	if ( inputdata.pActivator->IsPlayer() )
	{
		pActivator = ToBasePlayer( inputdata.pActivator );
	}

	Director->Panic( pActivator, 0, true );
}

void CInfoDirector::InputForceBoss( inputdata_t &inputdata )
{
	Director->m_bBossPendient = true;
}

void CInfoDirector::InputForceClimax( inputdata_t &inputdata )
{
	Director->Climax();
}

void CInfoDirector::InputSetPopulation( inputdata_t &inputdata )
{
	Director->SetPopulation( inputdata.value.String() );
	DirectorManager->LoadPopulation();
}

void CInfoDirector::InputDisclosePlayers( inputdata_t &inputdata )
{
	DirectorManager->Disclose();
}

void CInfoDirector::InputKillChilds( inputdata_t &inputdata )
{
	DirectorManager->KillChilds();
}

void CInfoDirector::InputKillNoVisibleChilds(inputdata_t &inputdata)
{
	DirectorManager->KillChilds( true );
}

void CInfoDirector::InputSetSpawnInForceAreas(inputdata_t &inputdata)
{
	DirectorManager->m_bSpawnInForceAreas = true;
}

void CInfoDirector::InputSetSpawnInNormalAreas(inputdata_t &inputdata)
{
	DirectorManager->m_bSpawnInForceAreas = false;
}

void CInfoDirector::InputStartSurvivalTime( inputdata_t &inputdata )
{
	Director->m_bSurvivalTimeStarted = true;
}

void CInfoDirector::InputDisableMusic( inputdata_t &inputdata )
{
	DirectorMusic->Stop();
	Director->m_bMusicEnabled = false;
}

void CInfoDirector::InputEnableMusic( inputdata_t &inputdata )
{
	Director->m_bMusicEnabled = true;
}

void CInfoDirector::InputSetMaxAliveBoss( inputdata_t &inputdata )
{
	Director->m_iMaxBossAlive = inputdata.value.Int();
}