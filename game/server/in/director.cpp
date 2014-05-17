//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Inteligencia artificial encargada de la creación de enemigos (hijos)
// además de poder controlar la música, el clima y otros aspectos
// del juego.
//
//=====================================================================//

#include "cbase.h"
#include "director.h"

#include "director_manager.h"
#include "director_music.h"
#include "director_item_manager.h"

// Modos de Juego
#include "director_mode_normal.h"

#include "in_player.h"
#include "players_manager.h"

#include "in_gamerules.h"
#include "in_utils.h"

#include "ai_basenpc.h"
#include "filesystem.h"

#include "soundent.h"
#include "engine/IEngineSound.h"

// Creamos la instancia a este sistema
#ifndef CUSTOM_DIRECTOR
	CDirector g_Director;
	CDirector *Director = &g_Director;
#endif

//====================================================================
// Comandos de consola
//====================================================================

ConVar director_debug( "director_debug", "0", 0, "Permite imprimir información de depuración del Director" );
ConVar director_max_common_alive( "director_max_common_alive", "30", FCVAR_SERVER, "Establece la cantidad máxima de hijos vivos que puede mantener el Director");

ConVar director_min_distance("director_min_distance", "600", FCVAR_SERVER, "Distancia mínima en la que se debe encontrar un hijo de los jugadores");
ConVar director_max_distance("director_max_distance", "2500", FCVAR_SERVER, "Distancia máxima en la que se debe encontrar un hijo de los jugadores");
ConVar director_danger_distance( "director_danger_distance", "700", FCVAR_SERVER, "" );

ConVar director_max_boss_alive("director_max_boss_alive", "2", FCVAR_SERVER, "Cantidad máxima de jefes vivos que puede mantener el Director");
ConVar director_no_boss("director_no_boss", "0", FCVAR_CHEAT, "");

ConVar director_spawn_outview( "director_spawn_outview", "1", FCVAR_SERVER | FCVAR_CHEAT, "Establece si se deben crear hijos fuera de la visión de los jugadores" );
ConVar director_allow_sleep( "director_allow_sleep", "1", FCVAR_SERVER | FCVAR_CHEAT, "Establece si el Director puede dormir" );
ConVar director_mode( "director_mode", "1", FCVAR_SERVER );

ConVar director_spawn_ambient_interval( "director_spawn_ambient_interval", "15", FCVAR_SERVER );
ConVar director_check_unreachable( "director_check_unreachable", "1", FCVAR_SERVER );

ConVar director_gameover( "director_gameover", "1", FCVAR_CHEAT | FCVAR_SERVER );
ConVar director_gameover_wait( "director_gameover_wait", "10", FCVAR_CHEAT );

//====================================================================
// Macros
//====================================================================

double round( double number )
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

//====================================================================
// Información
//====================================================================

BEGIN_SCRIPTDESC_ROOT( CDirector, SCRIPT_SINGLETON "El Director" )

	DEFINE_SCRIPTFUNC( IsStatus, "" )
	DEFINE_SCRIPTFUNC( IsPhase, "" )
	DEFINE_SCRIPTFUNC( IsAngry, "" )
	DEFINE_SCRIPTFUNC( GetTeamGood, "" )
	DEFINE_SCRIPTFUNC( GetTeamEvil, "" )

	DEFINE_SCRIPTFUNC( InDangerZone, "" )
	DEFINE_SCRIPTFUNC( ChildsVisibles, "" )

	DEFINE_SCRIPTFUNC( GetStatusName, "" )
	DEFINE_SCRIPTFUNC( GetPhaseName, "" )
	DEFINE_SCRIPTFUNC( GetAngryName, "" )
	DEFINE_SCRIPTFUNC( GetStatsName, "" )

END_SCRIPTDESC();

//====================================================================
// Constructor
//====================================================================
CDirector::CDirector() : CAutoGameSystemPerFrame( "CDirector" )
{
	
}

//====================================================================
// Inicia el sistema del Director
//====================================================================
bool CDirector::Init()
{
	NewMap();
	NewCampaign();

	SetupGamemodes();
	return true;
}

//====================================================================
// Ajusta las instancias para cada modo de Juego
//====================================================================
void CDirector::SetupGamemodes()
{
	m_pGameModes[ GAME_MODE_NONE ] = new CDR_Normal_GM();
}

//====================================================================
// Inicia la información para un nuevo mapa
//====================================================================
void CDirector::NewMap()
{
	Director->SetStatus( ST_NORMAL );
	Director->SetPhase( PH_RELAX, 10 );

	// Invalidamos los cronometros
	m_hLeft4Boss.Invalidate();
	m_hEndGame.Invalidate();

	// Información
	m_bSpawning			= false;
	m_bBlockAllStatus	= false;
	m_iNextWork			= gpGlobals->curtime + 3.0f;
	m_bMusicEnabled		= true;
	m_pInfoDirector		= NULL;

	m_iCombatChilds			= 0;
	m_iCombatsCount			= 0;
	m_iCombatWaves			= 0;
	m_nLastCombatDuration.Invalidate();
	m_iLastCombatSeconds	= 0;
	m_iLastCombatDeaths		= 0;
	
	m_iCommonAlive			= 0;
	m_iCommonInDangerZone	= 0;

	m_iAmbientAlive		= 0;
	m_iNextAmbientSpawn = gpGlobals->curtime;

	m_bBossPendient = false;
	m_iBossAlive	= 0;
	m_iMaxBossAlive = -1;

	// Necesitamos la entidad "info_director"
	FindInfoDirector();

	// Preparamos la maquina virtual
	PrepareVM();
}

//====================================================================
// Reinicia la información para una nueva campaña
//====================================================================
void CDirector::NewCampaign()
{
	// Información
	m_iAngry			= ANGRY_LOW;
	m_iPlayersDead		= 0;

	m_iCommonSpawned	= 0;
	m_iAmbientSpawned	= 0;

	m_iBossSpawned	= 0;
	m_iBossKilled	= 0;
}

//====================================================================
//====================================================================
void CDirector::PrepareVM()
{
	if ( !g_pScriptVM )
		return;
	
	g_pScriptVM->RegisterInstance( Director, "Director" );

	KeyValues *pOptions = new KeyValues("DirectorOptions");
	pOptions->LoadFromFile( filesystem, "scripts/director_options.txt", NULL );

	CScriptKeyValues *pDirectorOptions = new CScriptKeyValues( pOptions );
	g_pScriptVM->RegisterInstance( pDirectorOptions, "DirectorOptions" );
}

//====================================================================
// Encuentra al info_director
//====================================================================
void CDirector::FindInfoDirector()
{
	// Ya lo tenemos
	if ( m_pInfoDirector )
		return;

	m_pInfoDirector = (CInfoDirector *)gEntList.FindEntityByClassname( NULL, "info_director" );

	// No hay uno en el mapa, lo creamos.
	if ( !m_pInfoDirector )
	{
		m_pInfoDirector = (CInfoDirector *)CBaseEntity::CreateNoSpawn( "info_director", vec3_origin, vec3_angle );
		m_pInfoDirector->SetName( MAKE_STRING("@director") );
			
		DispatchSpawn( m_pInfoDirector );
		m_pInfoDirector->Activate();
	}

	if ( !m_pInfoDirector )
	{
		Warning("[CDirector::FindInfoDirector] Ha ocurrido un problema al crear la entidad info_director !!! \n");
		Stop();
	}
}

//====================================================================
// Para el procesamiento del Director
//====================================================================
void CDirector::Stop()
{
	// Matamos a todos los hijos
	DirectorManager->KillChilds();

	// Paramos al Director de Música
	if ( InRules )
		DirectorMusic->Stop();

	// Desactivamos
	m_bDisabled = true;
}

//====================================================================
// Destruye al Director ( Al cerrar el Juego )
//====================================================================
void CDirector::Shutdown()
{
	// Paramos
	Stop();

	// Reiniciamos todo
	Init();
}

//====================================================================
// Nuevo Mapa - Antes de cargar las entidades
//====================================================================
void CDirector::LevelInitPreEntity()
{
	// Iniciamos a nuestros ayudantes
	DirectorManager->Init();
	DirectorMusic->Init();
}

//====================================================================
// Nuevo Mapa - Después de cargar las entidades
//====================================================================
void CDirector::LevelInitPostEntity()
{
	DirectorManager->OnNewMap();
	DirectorMusic->OnNewMap();

	// Iniciamos
	NewMap();
}

//====================================================================
// Cerrando mapa - Antes de descargar las entidades
//====================================================================
void CDirector::LevelShutdownPreEntity()
{
	// Matamos a todos los hijos
	DirectorManager->KillChilds();
}

//====================================================================
// Cerrando mapa - Después de descargar las entidades
//====================================================================
void CDirector::LevelShutdownPostEntity()
{
}

//====================================================================
// Pensamiento - Antes de las entidades
//====================================================================
void CDirector::FrameUpdatePreEntityThink()
{
}

//====================================================================
// Pensamiento - Después de las entidades
//====================================================================
void CDirector::FrameUpdatePostEntityThink()
{
	// El Juego no ha empezado
	if ( !InRules )
	{
		SetStatus( ST_NORMAL );
		return;
	}

	// Director de Música
	DirectorMusic->Think();

	// ¡A trabajar!
	Work();
}

//====================================================================
// Comienza el trabajo del Director
//====================================================================
void CDirector::Work()
{
	// Imprimimos información de depuración
	if ( director_debug.GetBool() )
		PrintDebugInfo();

	// No podemos
	if ( engine->IsInEditMode() || m_bDisabled )
		return;

	// Aún no toca
	if ( gpGlobals->curtime <= m_iNextWork )
		return;

	// Verificamos si la partida ha terminado
	if ( IsGameover() )
		return;

	// Comprobamos a nuestros hijos
	CheckChilds();

	// Calculemos mi nivel de enojo
	UpdateAngry();

	// No hay un manejador para este modo de Juego
	if ( GameMode() == NULL )
	{
		Warning( "================================================================================== \n");
		Warning( "== [CDirector::Work] No hay un manejador para el Modo de Juego: %i \n", InRules->GameMode() );
		Warning( "== Esto podria ocacionar un Crash !!! \n" );
		Warning( "================================================================================== \n \n");

		return;
	}

	// Hacemos lo que el modo de Juego tenga que hacer
	GameMode()->Work();

	// Música
	if ( m_bMusicEnabled )
		DirectorMusic->Update();

	// Próxima actualización en .6s
	m_iNextWork = gpGlobals->curtime + .6f;
}

//====================================================================
// Hace lo necesario para reiniciar el mapa
//====================================================================
void CDirector::RestartMap()
{
	// Reiniciamos al Director
	NewMap();

	// Matamos a todos nuestros Hijos
	DirectorManager->KillChilds();

	// Reiniciamos el mapa
	InRules->CleanUpMap();

	// Reaparecemos a todos los Jugadores
	PlysManager->RespawnAll();
}

//====================================================================
// Verifica si la partida ha terminado
//====================================================================
bool CDirector::IsGameover()
{
	// Aún hay Jugadores vivos
	// No hay Jugadores conectados
	// No queremos que el Director verifique
	if ( PlysManager->GetWithLife() > 0 || PlysManager->GetConnected() <= 0 || !ALLOW_GAMEOVER )
	{
		// Evitamos bugs...
		if ( IsStatus(ST_GAMEOVER) )
		{
			SetStatus( ST_NORMAL );
		}

		return false;
	}

	// Todos los jugadores han muerto, la partida ha terminado.
	if ( !IsStatus(ST_GAMEOVER) )
	{
		SetStatus( ST_GAMEOVER );

		// Hacemos un Fadeout
		color32 black = {0, 0, 0, 255};
		UTIL_ScreenFadeAll( black, (GAMEOVER_WAIT-3), 5.0f, FFADE_OUT | FFADE_PURGE );

		// Paramos la música
		DirectorMusic->Stop();
		PlysManager->StopMusic();
	}

	// Seguimos actualizando la música
	DirectorMusic->Update();

	// Todavía no podemos reiniciar la partida
	if ( gpGlobals->curtime <= (m_flStatusTime+GAMEOVER_WAIT) )
		return true;

	// Reiniciamos la partida
	RestartMap();
	DirectorMusic->Stop();

	if ( DirectorItemManager )
		DirectorItemManager->LevelInitPostEntity();

	return true;
}

//====================================================================
// Calcula el enojo del Director
//====================================================================
void CDirector::UpdateAngry()
{
	int iPoints			= 0;
	int iPlayersStatus	= PlysManager->GetStatus();

	//
	// Estado de los Jugadores
	//
	iPoints += round( ( iPlayersStatus / 2 ) * 10 ); // 20%

	//
	//  Número de Hijos que he creado
	//
	if ( m_iCommonSpawned >= 1000 )
	{
		iPoints += 20;
	}
	else
	{
		iPoints += round( ( m_iCommonSpawned / 500 ) * 10 ); // 20%
	}

	//
	// Número de Jefes que he creado
	//
	if ( m_iBossSpawned >= 5 )
	{
		iPoints += 20;
	}
	else
	{
		iPoints += round( ( m_iBossSpawned / 5 ) * 20 ); // 20%
	}

	//
	// Su nivel de cordura es alta
	//
	iPoints += round( ( PlysManager->GetAllSanity() / PlysManager->GetTotal() ) / 100 ) * ( 10 * PlysManager->GetTotal() );

	//
	// Ya ha habido eventos de pánico
	//
	if ( m_iCombatsCount >= 5 )
	{
		iPoints += 10;
	}
	else
	{
		iPoints += round( ( m_iCombatsCount / 5 ) * 10 ); // 10%
	}

	//
	// Número de Muertes
	//
	if ( m_iPlayersDead <= 0 )
	{
		iPoints += 20;
	}
	else
	{
		iPoints += round( ( 5 / m_iPlayersDead ) * ( 10 / 5 ) );
	}

	//
	// Dificultad
	//
	if ( InRules->IsSkillLevel(SKILL_EASY) )
		iPoints -= 10;
	if ( InRules->IsSkillLevel(SKILL_MEDIUM) )
		iPoints -= 5;

	int iResult = round(iPoints / 25);
	++iResult;

	if ( iResult < 1 )
		iResult = 1;
	if ( iResult > 4 )
		iResult = 4;

	//
	// Enojo final
	//
	m_iAngry = (DirectorAngry)iResult;
}

//====================================================================
// Devuelve el modo de trabajo del Director
//====================================================================
bool CDirector::IsMode( int iStatus )
{
	int iValue = director_mode.GetInt();

	if ( iValue <= DIRECTOR_MODE_INVALID || iValue >= LAST_DIRECTOR_MODE )
		return DIRECTOR_MODE_NORMAL;

	return ( iStatus == iValue );
}

//====================================================================
// Devuelve el estado del Director en una cadena
//====================================================================
const char *CDirector::GetStatusName( int iStatus )
{
	if ( iStatus == ST_INVALID )
		iStatus = m_iStatus;

	switch ( iStatus )
	{
		case ST_NORMAL:
			return "NORMAL";
		break;

		case ST_COMBAT:
			return "COMBAT";
		break;

		case ST_FINALE:
			return "FINALE / CLIMAX";
		break;

		case ST_GAMEOVER:
			return "GAMEOVER";
		break;

		case ST_BOSS:
			return "JEFE";
		break;
	}

	return "DESCONOCIDO";
}

//====================================================================
// Devuelve la fase del Director en una cadena
//====================================================================
const char *CDirector::GetPhaseName( int iStatus )
{
	if ( iStatus == PH_INVALID )
		iStatus = m_iPhase;

	switch ( iStatus )
	{
		case PH_RELAX:
			return "RELAX";
		break;

		case PH_BUILD_UP:
			return "BUILD_UP";
		break;

		case PH_SANITY_FADE:
			return "SANITY_FADE";
		break;

		case PH_POPULATION_FADE:
			return "POPULATION_FADE";
		break;

		case PH_EVENT:
			return "EVENT";
		break;
	}

	return "DESCONOCIDO";
}

//====================================================================
// Devuelve el nivel de enojo del Director en una cadena
//====================================================================
const char *CDirector::GetAngryName( int iStatus )
{
	if ( iStatus == ANGRY_INVALID )
		iStatus = m_iAngry;

	switch ( iStatus )
	{
		case ANGRY_LOW:
			return "Feliz";
		break;

		case ANGRY_MEDIUM:
			return "Normal";
		break;

		case ANGRY_HIGH:
			return "Enojado";
		break;

		case ANGRY_CRAZY:
			return "Furioso";
		break;
	}

	return "Desconocido";
}

//====================================================================
// Devuelve el estao general de los jugadores en una cadena
//====================================================================
const char *CDirector::GetStatsName( int iStatus )
{
	if ( iStatus == 0 )
		iStatus = PlysManager->GetStatus();

	switch ( iStatus )
	{
		case STATS_POOR:
			return "Pobre (Mala partida)";
		break;

		case STATS_MED:
			return "Normal";
		break;

		case STATS_GOOD:
			return "Buena";
		break;

		case STATS_PERFECT:
			return "Partida perfecta";
		break;

		default:
			return "Desconocido";
		break;
	}
}

//====================================================================
// Imprime información útil del Director
//====================================================================
void CDirector::PrintDebugInfo()
{
	int iLine		= 5;
	int iForBoss	= (int)floor(m_hLeft4Boss.GetRemainingTime() + 0.5);

	Director_StatePrintf(++iLine, "DIRECTOR \n");
	Director_StatePrintf(++iLine, "------------------------------------------ \n");

	++iLine;

	Director_StatePrintf(++iLine, "Estado: %s \n", GetStatusName() );

	if ( IsPhase(PH_RELAX) )
	{
		Director_StatePrintf(++iLine, "Fase: %s ( %is ) \n", GetPhaseName(), m_iPhaseValue );
	}
	else if ( IsPhase(PH_SANITY_FADE) )
	{
		Director_StatePrintf(++iLine, "Fase: %s ( %.2f < 20.0 ) \n", GetPhaseName(), PlysManager->GetAllSanity() );
	}
	else if ( IsPhase(PH_POPULATION_FADE) )
	{
		Director_StatePrintf(++iLine, "Fase: %s ( %i > 10 ) \n", GetPhaseName(), m_iCommonAlive );
	}
	else
	{
		Director_StatePrintf(++iLine, "Fase: %s \n", GetPhaseName() );
	}

	Director_StatePrintf(++iLine, "Poblacion: %i \n", m_iCommonSpawned );
	Director_StatePrintf(++iLine, "Vivos: %i ( maximo: %i ) \n", m_iCommonAlive, MaxChilds() );
	Director_StatePrintf(++iLine, "Visibles: %i \n", m_iCommonVisibles );

	Director_StatePrintf(++iLine, "Puntos de creacion posibles: %i \n", DirectorManager->GetCandidateCount() );
	Director_StatePrintf(++iLine, "Nivel de enojo: %s \n", GetAngryName() );

	++iLine;

	Director_StatePrintf(++iLine, "MUSICA \n");
	Director_StatePrintf(++iLine, "------------------------------------------ \n");

	++iLine;

	Director_StatePrintf(++iLine, "Cercanos: %i \n", m_iCommonInDangerZone );

	if ( !IsStatus(ST_FINALE) )
	{
		++iLine;
		Director_StatePrintf(++iLine, "COMBATE \n");
		Director_StatePrintf(++iLine, "------------------------------------------ \n");
		++iLine;

		Director_StatePrintf(++iLine, "Hijos de la ultima: %i \n", m_iCombatChilds );
		Director_StatePrintf(++iLine, "Han pasado: %i combates \n", m_iCombatsCount );
		Director_StatePrintf(++iLine, "Duración de la ultima: %is \n", m_iLastCombatSeconds );
		Director_StatePrintf(++iLine, "Muertes de la ultima: %i \n", m_iLastCombatDeaths );

		if ( IsStatus(ST_COMBAT) )
			Director_StatePrintf(++iLine, "Faltan: %i hordas \n", m_iCombatWaves );
	}

	++iLine;
	Director_StatePrintf(++iLine, "HIJOS DE AMBIENTE \n");
	Director_StatePrintf(++iLine, "------------------------------------------ \n");
	++iLine;

	Director_StatePrintf(++iLine, "Vivos: %i \n", m_iAmbientAlive );
	Director_StatePrintf(++iLine, "Cantidad: %i \n", m_iAmbientSpawned );

	++iLine;
	Director_StatePrintf(++iLine, "JEFES \n");
	Director_StatePrintf(++iLine, "------------------------------------------ \n");
	++iLine;

	Director_StatePrintf(++iLine, "Vivos: %i \n", m_iBossAlive );
	Director_StatePrintf(++iLine, "Cantidad: %i \n", m_iBossSpawned );
	Director_StatePrintf(++iLine, "Posibilidad en: %is \n", iForBoss );

	++iLine;
	Director_StatePrintf(++iLine, "JUGADORES \n");
	Director_StatePrintf(++iLine, "------------------------------------------ \n");
	++iLine;

	Director_StatePrintf(++iLine, "Vivos: %i \n", PlysManager->GetWithLife() );
	Director_StatePrintf(++iLine, "Salud: %i \n", PlysManager->GetAllHealth() );
	Director_StatePrintf(++iLine, "Cordura: %.2f \n", PlysManager->GetAllSanity() );
	Director_StatePrintf(++iLine, "Estado: %s \n", GetStatsName() );

	if ( PlysManager->InCombat() )
		Director_StatePrintf(++iLine, "EN COMBATE \n");
	else
		Director_StatePrintf(++iLine, "NO EN COMBATE \n");
}

//====================================================================
// Devuelve la instancia del Modo de Juego
//====================================================================
CDR_Gamemode *CDirector::GameMode()
{
	if ( !InRules )
		return NULL;

	return m_pGameModes[ InRules->GameMode() ];
}

//====================================================================
// Devuelve la distancia minima de creación
//====================================================================
float CDirector::MinDistance()
{
	float flDistance = director_min_distance.GetFloat();

	// Disminuimos la distancia
	if ( IsStatus(ST_COMBAT) || IsStatus(ST_FINALE) )
		flDistance -= 300;

	return flDistance;
}

//====================================================================
// Devuelve la distancia máxima de creación
//====================================================================
float CDirector::MaxDistance()
{
	float flDistance = director_max_distance.GetFloat();

	// Disminuimos la distancia
	if ( InRules->IsSkillLevel(SKILL_HARD) )
		flDistance -= 300;

	// Disminuimos la distancia
	if ( IsStatus(ST_COMBAT) || IsStatus(ST_FINALE) )
		flDistance -= 300;

	return flDistance;
}

//====================================================================
// Devuelve si el Hijo esta muy lejos de los jugadores
//====================================================================
bool CDirector::IsTooFar( CBaseEntity *pEntity )
{
	float flMaxDistance = ( MaxDistance() + 1000 );

	// No hay nadie con vida
	if ( PlysManager->GetWithLife() <= 0 )
		return false;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= PlysManager->Get( i );

		if ( !pPlayer )
			continue;

		// Es del equipo malvado
		if ( pPlayer->GetTeamNumber() == GetTeamEvil() )
			continue;

		// Obtenemos la distancia
		float flDist = pEntity->GetAbsOrigin().DistTo( pPlayer->GetAbsOrigin() );

		// Esta cerca
		if ( flDist <= flMaxDistance )
			return false;
	}

	return true;
}

//====================================================================
// Devuelve si el NPC esta muy cerca de los jugadores
//====================================================================
bool CDirector::IsTooClose( CBaseEntity *pEntity )
{
	float flDistance = MinDistance();

	// No hay nadie con vida
	if ( PlysManager->GetWithLife() <= 0 )
		return false;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= PlysManager->Get(i);

		if ( !pPlayer )
			continue;

		// Es del equipo malvado
		if ( pPlayer->GetTeamNumber() == GetTeamEvil() )
			continue;

		// Obtenemos la distancia
		float flDist = pEntity->GetAbsOrigin().DistTo( pPlayer->GetAbsOrigin() );

		// Esta cerca
		if ( flDist < flDistance )
			return true;
	}

	return false;
}

//====================================================================
// Mata a un Hijo
//====================================================================
void CDirector::KillChild( CBaseEntity *pEntity )
{
	if ( pEntity->IsPlayer() )
		return;

	UTIL_Remove( pEntity );

	// Es un Jefe
	/*if ( pEntity->GetEntityName() == AllocPooledString(DIRECTOR_BOSS_NAME) && !pEntity->IsPlayer() )
	{
		UTIL_Remove( pEntity );
		return;
	}

	CTakeDamageInfo info( pEntity, pEntity, pEntity->GetMaxHealth(), DMG_GENERIC );
	pEntity->TakeDamage( info );*/
}

//====================================================================
// [Evento] Hemos creado un hijo
//====================================================================
void CDirector::OnSpawnChild( CBaseEntity *pEntity )
{
	AddChildSlot( pEntity );
	++m_iCommonSpawned;

	// Un Hijo de Combate
	if ( IsStatus(ST_COMBAT) )
		++m_iCombatChilds;

	// Hemos creado un hijo
	GameMode()->OnSpawnChild( pEntity );
	m_pInfoDirector->OnSpawnChild.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//====================================================================
// [Evento] Hemos creado un jefe
//====================================================================
void CDirector::OnSpawnBoss( CBaseEntity *pEntity )
{
	++m_iBossSpawned;
	m_bBossPendient = false;

	AddChildSlot( pEntity );

	// Hemos creado un jefe
	GameMode()->OnSpawnBoss( pEntity );
	m_pInfoDirector->OnSpawnBoss.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//====================================================================
// [Evento] Hemos creado un hijo especial
//====================================================================
void CDirector::OnSpawnSpecial( CBaseEntity *pEntity )
{
	++m_iSpecialsSpawned;

	AddChildSlot( pEntity );

	// Hemos creado un especial
	GameMode()->OnSpawnSpecial( pEntity );
	m_pInfoDirector->OnSpawnSpecial.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//====================================================================
// [Evento] Hemos creado un hijo de ambiente
//====================================================================
void CDirector::OnSpawnAmbient( CBaseEntity *pEntity )
{
	++m_iAmbientSpawned;

	AddChildSlot( pEntity );

	// Hemos creado un especial
	GameMode()->OnSpawnAmbient( pEntity );
	m_pInfoDirector->OnSpawnAmbient.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//====================================================================
// [Evento] Un jugador ha sufrido daño
//====================================================================
void CDirector::OnPlayerHurt( CBasePlayer *pPlayer )
{
	m_iLastAttackTime = gpGlobals->curtime;
	GameMode()->OnPlayerHurt( pPlayer );
}

//====================================================================
// [Evento] Un jugador ha muerto
//====================================================================
void CDirector::OnPlayerKilled( CBasePlayer *pPlayer )
{
	// La horda se ha cobrado una vida
	if ( IsStatus(ST_COMBAT) )
	{
		++m_iLastCombatDeaths;
	}

	// Nos hemos cobrado una vida más
	++m_iPlayersDead;

	GameMode()->OnPlayerKilled( pPlayer );
}

//====================================================================
// Devuelve si se puede establecer el estado
//====================================================================
bool CDirector::CanSetStatus( DirectorStatus iNewStatus )
{
	// El Final no puede cambiarse
	//if ( IsStatus(ST_FINALE) )
		//return false;

	// Estamos bloqueando todos los estados
	if ( m_bBlockAllStatus )
		return false;

	return true;
}

//====================================================================
// Devuelve si se puede establecer una fase
//====================================================================
bool CDirector::CanSetPhase( DirectorPhase iNewPhase )
{
	return true;
}

//====================================================================
// Establece el estado actual
//====================================================================
void CDirector::SetStatus( DirectorStatus iStatus )
{
	// No podemos cambiar
	if ( !CanSetStatus(iStatus) )
		return;

	m_iStatus		= iStatus;
	m_flStatusTime	= gpGlobals->curtime;

	if ( GameMode() )
		GameMode()->OnSetStatus( iStatus );
}

//====================================================================
// Establece la fase actual
//====================================================================
void CDirector::SetPhase( DirectorPhase iPhase, int iValue )
{
	// No podemos cambiar
	if ( !CanSetPhase(iPhase) )
		return;

	m_iPhase		= iPhase;
	m_iPhaseValue	= iValue;
	m_flPhaseTime	= gpGlobals->curtime;

	if ( GameMode() )
		GameMode()->OnSetPhase( iPhase, iValue );
}

//====================================================================
// Comienza un Combate
//====================================================================
void CDirector::StartCombat( CBaseEntity *pActivator, int iWaves, bool bInfinite )
{
	// Reiniciamos estadisticas
	m_iCombatChilds			= 0;
	m_iCombatWaves			= iWaves;
	m_nLastCombatDuration.Start();
	m_iLastCombatSeconds	= 0;
	m_iLastCombatDeaths		= 0;

	++m_iCombatsCount;

	SetStatus( ST_COMBAT );

	if ( bInfinite )
		SetPhase( PH_CRUEL_BUILD_UP );
}

//====================================================================
// Devuelve la cantidad máxima de hijos que se pueden crear
//====================================================================
int CDirector::MaxChilds()
{
	int iMax = MAX_ALIVE;

	//
	// No queremos crear más Hijos
	//
	if ( IsStatus(ST_GAMEOVER) || IsPhase(PH_RELAX) || IsPhase(PH_SANITY_FADE) || IsPhase(PH_POPULATION_FADE) )
		return 0;

	//
	// Nos estamos quedando sin espacios ¡debemos parar!
	//
	if ( Utils::RunOutEntityLimit( iMax+DIRECTOR_SECURE_ENTITIES ) )
		return 0;

	//
	// Entre más fácil, menos hijos
	//
	switch ( InRules->GetSkillLevel() )
	{
		case SKILL_EASY:
			iMax -= 10;
		break;

		case SKILL_MEDIUM:
			iMax -= 5;
		break;
	}

	//
	// Los Jugadores tienen una mala partida...
	//
	if ( PlysManager->GetStatus() <= STATS_POOR )
		iMax -= 5;

	//
	// No podemos sobrepasar el limite indicado
	//
	if ( iMax > MAX_ALIVE )
		iMax = MAX_ALIVE;

	//
	// Algo interesante ha ocurrido
	//
	if ( iMax <= 0 )
		iMax = MAX_ALIVE;

	return iMax;
}

//====================================================================
// Devuelve el tipo de Hijo
//====================================================================
int CDirector::GetChildType( CBaseEntity *pEntity )
{
	bool bIsBoss	= FStrEq( STRING(pEntity->GetEntityName()), DIRECTOR_BOSS_NAME );
	bool bIsSpecial = FStrEq( STRING(pEntity->GetEntityName()), DIRECTOR_SPECIAL_NAME );
	bool bIsAmbient	= FStrEq( STRING(pEntity->GetEntityName()), DIRECTOR_AMBIENT_NAME );

	if ( bIsBoss )
		return DIRECTOR_BOSS;

	if ( bIsSpecial )
		return DIRECTOR_SPECIAL_CHILD;

	if ( bIsAmbient )
		return DIRECTOR_AMBIENT_CHILD;

	return DIRECTOR_CHILD;
}

//====================================================================
// Libera el Slot del Hijo
//====================================================================
void CDirector::FreeChildSlot( CBaseEntity *pEntity )
{
	/*for ( int i = 0; i < DIRECTOR_MAX_CHILDS; ++i )
	{
		CBaseEntity *pChild = m_pChilds[i];

		if ( !pChild )
			continue;

		if ( pChild != pEntity )
			continue;

		FreeChildSlot( i, true );
		break;
	}*/
}

//====================================================================
// Libera el Slot del Hijo
//====================================================================
void CDirector::FreeChildSlot( int iSlot, bool bRemove )
{
	/*CBaseEntity *pChild = m_pChilds[iSlot];

	if ( bRemove && pChild )
	{
		UTIL_Remove( pChild );
	}

	m_pChilds[ iSlot ] = NULL;*/
}

//====================================================================
// Agrega un Hijo a la lista y devuelve su ID
//====================================================================
int CDirector::AddChildSlot( CBaseEntity *pEntity )
{
	/*for ( int i = 0; i < DIRECTOR_MAX_CHILDS; ++i )
	{
		CBaseEntity *pChild = m_pChilds[i];

		// Slot ocupado
		if ( pChild )
			continue;

		m_pChilds[i] = pEntity;
		return i;
	}*/

	return -1;
}

//====================================================================
// Verifica los hijos que siguen vivos
//====================================================================
void CDirector::CheckChilds()
{
	// Reiniciamos las estadisticas
	m_iCommonAlive			= 0;
	m_iCommonInDangerZone	= 0;
	m_iCommonVisibles		= 0;
	m_iBossAlive			= 0;
	m_iAmbientSpawned		= 0;
	m_iAmbientAlive			= 0;

	CBaseEntity *pChild = NULL;

	do
	{
		pChild = gEntList.FindEntityByName( pChild, "director_*" );

		// No existe
		if ( pChild == NULL )
			continue;

		// Esta muerto, no nos sirve
		if ( !pChild->IsAlive() )
			continue;

		CheckChild( pChild );

	} while ( pChild );

	/*for ( int i = 0; i < DIRECTOR_MAX_CHILDS; ++i )
	{
		CBaseEntity *pChild = m_pChilds[i];

		// No existe
		if ( pChild == NULL )
			continue;

		// Esta muerto, no nos sirve
		if ( !pChild->IsAlive() )
		{
			FreeChildSlot( i );
			continue;
		}

		CheckChild ( pChild );
	}*/
}

//====================================================================
// Comprueba si el Hijo aún sirve
//====================================================================
void CDirector::CheckChild( CBaseEntity *pEntity )
{
	int iType = GetChildType( pEntity );

	//
	// Esta muy lejos de los Jugadores o no puede alcanzarlos
	//
	if ( CheckDistance(pEntity) || CheckUnreachable(pEntity)  )
	{
		KillChild( pEntity );

		// No lo contamos como válido
		if ( iType == DIRECTOR_SPECIAL_CHILD )
		{
			--m_iSpecialsSpawned;
		}
		else if ( iType == DIRECTOR_BOSS )
		{
			--m_iBossSpawned;
		}
		else if ( iType != DIRECTOR_AMBIENT_CHILD )
		{
			--m_iCommonSpawned;
		}

		return;
	}

	//
	// Te estan viendo
	//
	if ( PlysManager->IsVisible(pEntity) )
	{
		++m_iCommonVisibles;
	}

	//
	// Los hijos de ambiente no necesitan enemigos
	//
	if ( iType == DIRECTOR_AMBIENT_CHILD )
	{
		++m_iAmbientAlive;
		return;
	}

	// ¿Podemos darle un enemigo?
	CheckNewEnemy( pEntity );

	if ( iType == DIRECTOR_SPECIAL_CHILD )
	{
		++m_iSpecialsAlive;
	}
	else if ( iType == DIRECTOR_BOSS )
	{
		++m_iBossAlive;
	}
	else
	{
		// Obtenemos la distancia a la que se encuentra del jugador más cercano
		float flDistance;
		PlysManager->GetNear( pEntity->GetAbsOrigin(), flDistance, GetTeamGood() );

		// Esta cerca
		if ( flDistance <= DANGER_DISTANCE )
			++m_iCommonInDangerZone;

		++m_iCommonAlive;
	}
}

//====================================================================
// Devuelve si el Hijo puede ser eliminado por estar muy
// lejos de los Jugadores
//====================================================================
bool CDirector::CheckDistance( CBaseEntity *pEntity )
{
	// Te estan viendo
	if ( PlysManager->InVisibleCone(pEntity) )
		return false;

	// No estas muy lejos
	if ( !IsTooFar(pEntity) )
		return false;

	int iType = GetChildType( pEntity );

	// Obtenemos la distancia a la que se encuentra del jugador más cercano
	float flDistance;
	PlysManager->GetNear( pEntity->GetAbsOrigin(), flDistance, GetTeamGood() );
	
	// Solo podemos eliminar a un Jefe si esta a una gran distancia.
	if ( iType == DIRECTOR_BOSS )
	{
		if ( flDistance > 1000 )
		{
			return true;
		}
	}
	else
	{
		return true;
	}

	return false;
}

//====================================================================
// Devuelve si el Hijo puede ser eliminado por no tener
// una ruta de ataque.
//====================================================================
bool CDirector::CheckUnreachable( CBaseEntity *pEntity )
{
	// No eres un NPC
	if ( !pEntity->IsNPC() )
		return false;

	CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();

	// Te estan viendo
	if ( PlysManager->InVisibleCone(pEntity) )
		return false;

	// Estas muy cerca de los Jugadores
	if ( IsTooClose(pEntity) )
		return false;

	// Nos piden que no hagamos esta verificación
	if ( !CHECK_UNREACHABLE )
		return false;

	// No tiene un Enemigo o no es un Jugador
	if ( !pEntity->GetEnemy() || !pEntity->GetEnemy()->IsPlayer() )
		return false;

	// ¿Me estas diciendo que no puedes atacar? Dale paso a alguien más
	if ( pNPC->IsUnreachable(pEntity->GetEnemy()) )
	{
		DirectorManager->AddBan( pNPC->GetAbsOrigin(), BAN_DURATION_UNREACHABLE );
		return true;
	}

	return false;
}

//====================================================================
// Devuelve si el Director le ha proporcionado información
// acerca de un enemigo al Hijo
//====================================================================
bool CDirector::CheckNewEnemy( CBaseEntity *pEntity )
{
	// No es un NPC
	if ( !pEntity->IsNPC() )
		return false;

	int iType = GetChildType( pEntity );

	// Solo podemos dar un enemigo si estamos en un evento de Pánico o Climax
	if ( !IsStatus(ST_COMBAT) && !IsStatus(ST_FINALE) )
		return false;

	// Ya tiene un enemigo
	if ( pEntity->GetEnemy() )
	{
		pEntity->MyNPCPointer()->UpdateEnemyMemory( pEntity->GetEnemy(), pEntity->GetEnemy()->GetAbsOrigin() );
		return true;
	}
	
	// Obtenemos un jugador al azar
	CIN_Player *pPlayer = PlysManager->GetRandom();

	// Este es perfecto
	if ( pPlayer )
	{
		pEntity->MyNPCPointer()->UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin() );
	}
	
	return true;
}

//====================================================================
// Devuelve si es posible crear más hijos
//====================================================================
bool CDirector::CanSpawnChilds()
{
	int iMax = MaxChilds();

	// Nos estamos quedando sin espacios ¡debemos parar!
	if ( Utils::RunOutEntityLimit( MAX_ALIVE+DIRECTOR_SECURE_ENTITIES ) )
		return false;

	// El Modo de Juego dice que no
	if ( !GameMode()->CanSpawnChilds() )
		return false;

	// No hay que crear en estas fases
	if ( IsPhase(PH_RELAX) || IsPhase(PH_SANITY_FADE) || IsPhase(PH_POPULATION_FADE) )
		return false;

	// Hijos al limite, no podemos con más
	if ( m_iCommonAlive >= iMax )
		return false;

	// Estamos en proceso de crear más
	if ( m_bSpawning )
		return false;

	return true;
}

//====================================================================
// Procesa a todos los tipos de hijos
//====================================================================
void CDirector::HandleAll()
{
	HandleChilds();
	HandleAmbient();
	HandleBoss();
	HandleSpecials();
}

//====================================================================
// Procesa la creación de hijos normales
//====================================================================
void CDirector::HandleChilds()
{
	// No podemos crear más hijos ahora mismo
	if ( !CanSpawnChilds() )
		return;

	// Comenzamos la creación
	DirectorManager->StartSpawn( DIRECTOR_CHILD );
}

//====================================================================
// Devuelve si es posible crear hijos de ambiente
//====================================================================
bool CDirector::CanSpawnAmbient()
{
	// Nos estamos quedando sin espacios ¡debemos parar!
	if ( Utils::RunOutEntityLimit( MAX_ALIVE+DIRECTOR_SECURE_ENTITIES ) )
		return false;

	// No debe haber más de 8 hijos de ambiente
	if ( m_iAmbientAlive >= 8 )
		return false;

	// Estamos en proceso de crear más
	if ( m_bSpawning )
		return false;

	// Aún no toca
	if ( gpGlobals->curtime < m_iNextAmbientSpawn )
		return false;	

	// No hay lugares donde crear
	if ( DirectorManager->m_hAmbientSpawnAreas.Count() <= 0 )
		return false;

	return true;
}

//====================================================================
// Procesa la creación de hijos de ambiente
//====================================================================
void CDirector::HandleAmbient()
{
	// No podemos crear más hijos ahora mismo
	if ( !CanSpawnAmbient() )
		return;

	DirectorManager->StartSpawn( DIRECTOR_AMBIENT_CHILD );
	m_iNextAmbientSpawn = gpGlobals->curtime + SPAWN_AMBIENT_INTERVAL;
}

//====================================================================
// Reinicia el contador para un próximo Jefe
//====================================================================
void CDirector::RestartCountdownBoss()
{
	// Fácil: De 300 a 400 segundos
	int i = RandomInt( 300, 400 );

	switch ( InRules->GetSkillLevel() )
	{
		// Medio: 250 a 400 segundos
		case SKILL_MEDIUM:
			i = RandomInt( 250, 400 );
		break;

		// Dificil: 100 a 350 segundos
		case SKILL_HARD:
			i = RandomInt( 100, 350 );
		break;
	}

	m_hLeft4Boss.Start( i );
}

//====================================================================
// Devuelve si es posible crear un jefe
//====================================================================
bool CDirector::CanSpawnBoss()
{
	// Nos estamos quedando sin espacios ¡debemos parar!
	if ( Utils::RunOutEntityLimit( MAX_ALIVE+DIRECTOR_SECURE_ENTITIES ) )
		return false;

	// El modo de Juego dice que no
	if ( !GameMode()->CanSpawnBoss() )
		return false;

	// Limite de Jefes en el mapa
	int iMaxBoss = ( m_iMaxBossAlive >= 0 ) ? m_iMaxBossAlive : MAX_BOSS_ALIVE;

	// Muchos jefes en el escenario
	if ( m_iBossAlive >= iMaxBoss )
		return false;

	// No podemos
	if ( NO_BOSS )
		return false;

	// Estamos en proceso de crear más
	if ( m_bSpawning )
		return false;

	// Esta pendiente la creación de un jefe
	if ( m_bBossPendient )
		return true;

	// Evento de Climax ¡Quiero a los jugadores en pedacitos!
	if ( IsStatus(ST_FINALE) )
		return true;

	// No ha pasado el tiempo necesario para un jefe
	if ( !m_hLeft4Boss.IsElapsed() || !m_hLeft4Boss.HasStarted() )
		return false;

	// No hace falta
	// Ivan: El RandomInt es para que haya solo una pequeña probabilidad de que si haya. 1 / 101
	if ( m_iAngry <= ANGRY_LOW && RandomInt(0, 100) != 1 )
		return false;

	// Un poco de suerte
	if ( RandomInt(0, 100) > 50 )
		return false;

	return true;
}

//====================================================================
// Procesa la creación de jefes
//====================================================================
void CDirector::HandleBoss()
{
	// Han eliminado al Jefe
	if ( IsStatus(ST_BOSS) && m_iBossAlive <= 0 )
	{
		// Fácil: Pasamos a Relajado
		if ( InRules->IsSkillLevel(SKILL_EASY) )
		{
			SetStatus( ST_NORMAL );
		}

		// Pasamos a Pánico
		else
		{
			SetStatus( ST_COMBAT );
		}
	}

	// Es hora de crear un Jefe
	if ( CanSpawnBoss() )
	{
		RestartCountdownBoss();
		DirectorManager->StartSpawn( DIRECTOR_BOSS );
	}
}

//====================================================================
// Pone pendiente a un jefe
//====================================================================
void CDirector::TryBoss()
{
	if ( m_iBossSpawned > 0 )
		--m_iBossSpawned;

	m_bBossPendient = true;
}

//====================================================================
// Devuelve si es posible crear un hijo especial
//====================================================================
bool CDirector::CanSpawnSpecial()
{
	// TODO
	return false;
}

//====================================================================
// Procesa la creación de jefes
//====================================================================
void CDirector::HandleSpecials()
{
	// No podemos
	if ( !CanSpawnSpecial() )
		return;

	// TODO
}

//====================================================================
//====================================================================
//====================================================================

void CC_ForceNormal()
{
	if ( !Director )
		return;

	Director->SetStatus( ST_NORMAL );
}

void CC_ForceBoss()
{
	if ( !Director )
		return;

	Director->m_bBossPendient = true;
}

void CC_ForceCombat()
{
	if ( !Director )
		return;

	Director->StartCombat( UTIL_GetCommandClient() );
}

void CC_ForceFinale()
{
	if ( !Director )
		return;

	Director->SetStatus( ST_FINALE );
}

void CC_ZSpawn()
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	if ( !pPlayer )
		return;

	director_spawn_outview.SetValue( 0 );

	trace_t tr;
	Vector forward;

	AngleVectors( pPlayer->EyeAngles(), &forward );
	UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * 300.0f, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 0.0 )
	{
		// trace to the floor from this spot
		Vector vecSrc = tr.endpos;
		tr.endpos.z += 12;

		UTIL_TraceLine( vecSrc + Vector(0, 0, 12),
			vecSrc - Vector( 0, 0, 512 ) ,MASK_SOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		
		DirectorManager->SpawnChild( DIRECTOR_CHILD, tr.endpos );
	}

	director_spawn_outview.SetValue( 1 );
}

void CC_ZSpawnBatch()
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	if ( !pPlayer )
		return;

	director_spawn_outview.SetValue( 0 );

	trace_t tr;
	Vector forward;

	AngleVectors( pPlayer->EyeAngles(), &forward );
	UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * 300.0f, MASK_SOLID,  pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 0.0 )
	{
		// trace to the floor from this spot
		Vector vecSrc = tr.endpos;
		tr.endpos.z += 12;

		UTIL_TraceLine( vecSrc + Vector(0, 0, 12),
			vecSrc - Vector( 0, 0, 512 ) ,MASK_SOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		
		DirectorManager->SpawnBatch( DIRECTOR_CHILD, tr.endpos );
	}

	director_spawn_outview.SetValue( 1 );
}

void CC_UpdateNodes()
{
	DirectorManager->Update();
}

void CC_DirectorStop()
{
	Director->Stop();
}

void CC_DirectorStart()
{
	Director->m_bDisabled = false;
}

void CC_DirectorKill()
{
	DirectorManager->KillChilds();
}

static ConCommand director_force_normal( "director_force_normal", CC_ForceNormal );
static ConCommand director_force_boss( "director_force_boss", CC_ForceBoss );
static ConCommand director_force_combat( "director_force_combat", CC_ForceCombat );
static ConCommand director_force_finale( "director_force_finale", CC_ForceFinale );

static ConCommand director_update("director_update", CC_UpdateNodes, "Actualiza los nodos candidatos para crear hijos manualmente.");

static ConCommand z_spawn("z_spawn", CC_ZSpawn);
static ConCommand z_spawn_batch("z_spawn_batch", CC_ZSpawnBatch);

static ConCommand director_stop("director_stop", CC_DirectorStop);
static ConCommand director_start("director_start", CC_DirectorStart);
static ConCommand director_kill_childs("director_kill_childs", CC_DirectorKill);