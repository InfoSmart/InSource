//==== InfoSmart. Todos los derechos reservados .===========//
//
// Inteligencia artificial encargada de la creación de enemigos (hijos)
// además de poder controlar la música, el clima y otros aspectos
// del juego.
//
//
// Estados del Director:
//
// SLEEP: El Director entrara en un estado de "sueño" en el que no
// se crearán hijos de ningún tipo. El estado permanecerá hasta que el estres
// de los jugadores baje.
//
// RELAXED: Un estado de relajación en la que solo se creará una
// pequeña cantidad de hijos con proposito de ambientación.
//
// PANIC: Un evento de pánico en el que el Director creará varios
// hijos que atacaran directamente a los jugadores.
//
// POST_PANIC: Se establece después del Pánico. Espera 5 segundos a que
// los hijos sean eliminados para pasar a RELAXED, si esto no ocurre
// se empezará a eliminar a los hijos que no sean visibles (asumiendo
// que se han quedado atascados o "bugeados")
//
// BOSS: Es establecido cuando un Jefe reporta que ha visto a un jugador.
// Durante este estado el Director no crea hijos (a menos que este en Dificil)
// y el Director de Música se encarga de reproducir música del jefe.
//
// CLIMAX: Es establecido cuando el mapa realiza el evento final de escape.
// Durante este estado el Director empezará a crear hijos y jefes infinitamente
// con el fin de capturar/eliminar a los jugadores antes de que puedan escapar.
//
// GAMEOVER: Es estabecido cuando el Director detecta que todos los jugadores
// han muerto o no pueden continuar. Se reproduce una canción para la ocación y
// se  reinicia el mapa.
//
//==========================================================//

#include "cbase.h"
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
#include "in_utils.h"

#include "ai_basenpc.h"

#include "soundent.h"
#include "engine/IEngineSound.h"

#ifndef DIRECTOR_CUSTOM
	CDirector g_Director;
	CDirector* Director = &g_Director;
#endif

//=========================================================
// Comandos de consola
//=========================================================

ConVar director_debug( "director_debug", "0", 0, "Permite imprimir información de depuración del Director" );
ConVar director_max_alive( "director_max_alive", "50", FCVAR_SERVER, "Establece la cantidad máxima de hijos vivos que puede mantener el Director");
ConVar director_gameover_wait( "director_gameover_wait", "10", FCVAR_CHEAT );

ConVar director_min_distance("director_min_distance", "800", FCVAR_SERVER, "Distancia mínima en la que se debe encontrar un hijo de los jugadores");
ConVar director_max_distance("director_max_distance", "2000", FCVAR_SERVER, "Distancia máxima en la que se debe encontrar un hijo de los jugadores");

ConVar director_max_boss_alive("director_max_boss_alive", "2", FCVAR_SERVER, "Cantidad máxima de jefes vivos que puede mantener el Director");
ConVar director_no_boss("director_no_boss", "0", FCVAR_CHEAT, "");

ConVar director_spawn_outview("director_spawn_outview", "1", FCVAR_SERVER | FCVAR_CHEAT, "Establece si se deben crear hijos fuera de la visión de los jugadores");
ConVar director_allow_sleep("director_allow_sleep", "1", FCVAR_SERVER | FCVAR_CHEAT, "Establece si el Director puede dormir");
ConVar director_mode("director_mode", "1", FCVAR_SERVER);

ConVar director_spawn_ambient_interval("director_spawn_ambient_interval", "15", FCVAR_SERVER);
ConVar director_check_unreachable( "director_check_unreachable", "1", FCVAR_SERVER );

ConVar director_gameover( "director_gameover", "1", FCVAR_CHEAT | FCVAR_SERVER );

//=========================================================
// Macros
//=========================================================

#define MAX_ALIVE		director_max_alive.GetInt()
#define GAMEOVER_WAIT	director_gameover_wait.GetInt()

#define MIN_DISTANCE	director_min_distance.GetInt()
#define MAX_DISTANCE	director_max_distance.GetInt()

#define MAX_BOSS_ALIVE	director_max_boss_alive.GetInt()
#define NO_BOSS			director_no_boss.GetBool()

#define SPAWN_OUTVIEW	director_spawn_outview.GetBool()
#define ALLOW_SLEEP		director_allow_sleep.GetBool()

#define CHECK_UNREACHABLE director_check_unreachable.GetBool()
#define ALLOW_GAMEOVER director_gameover.GetBool()

double round( double number )
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

//=========================================================
// Información y Red
//=========================================================
CDirector::CDirector() : CAutoGameSystemPerFrame( "CDirector" )
{
}

//=========================================================
//=========================================================
bool CDirector::Init()
{
	NewMap();
	NewCampaign();

	return true;
}

//=========================================================
// Inicia la información para un nuevo mapa
//=========================================================
void CDirector::NewMap()
{
	// Invalidamos los cronometros
	m_hLeft4Boss.Invalidate();
	m_hLeft4Panic.Invalidate();
	m_hLeft4FinishPanic.Invalidate();
	m_hDisabledTime.Invalidate();
	m_hEnd.Invalidate();
	m_nPostPanicLimit.Invalidate();

	// Información
	m_bSpawning			= false;
	m_bBlockAllStatus	= false;
	m_iNextWork			= gpGlobals->curtime + 3.0f;
	m_bMusicEnabled		= true;
	m_pInfoDirector		= NULL;

	m_bSurvivalTimeStarted	= false;
	m_iSurvivalTime			= 0;

	m_bPanicSuspended	= false;

	m_iSpawnQueue		= 0;
	m_iChildsAlive		= 0;
	m_iChildsInDangerZone = 0;

	m_iAmbientAlive		= 0;
	m_iNextAmbientSpawn = gpGlobals->curtime;

	m_bBossPendient = false;
	m_iBossAlive	= 0;
	m_iMaxBossAlive = -1;

	FindInfoDirector();
}

//=========================================================
// Reinicia la información para una nueva campaña
//=========================================================
void CDirector::NewCampaign()
{
	// Información
	m_iAngry			= ANGRY_LOW;
	m_iPlayersDead		= 0;

	m_iPanicChilds		= 0;
	m_iPanicCount		= 0;
	
	m_iLastPanicDuration	= 0;
	m_iLastPanicDeaths		= 0;

	m_iChildsSpawned	= 0;
	m_iAmbientSpawned	= 0;

	m_iBossSpawned	= 0;
	m_iBossKilled	= 0;
}

//=========================================================
// Encuentra al info_director
//=========================================================
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

//=========================================================
// Para el procesamiento del Director
//=========================================================
void CDirector::Stop()
{
	// Matamos a todos los hijos
	DirectorManager->KillChilds();

	// Paramos al Director de Música
	DirectorMusic->Stop();

	// Desactivamos
	m_bDisabled = true;
}

//=========================================================
// Destruye al Director ( Al cerrar el Juego )
//=========================================================
void CDirector::Shutdown()
{
	// Paramos
	Stop();

	// Reiniciamos todo
	Init();

	// Paramos la música
	//DirectorMusic->Shutdown();
}

//=========================================================
// Nuevo Mapa - Antes de cargar las entidades
//=========================================================
void CDirector::LevelInitPreEntity()
{
	// Iniciamos a nuestros ayudantes
	DirectorManager->Init();
	DirectorMusic->Init();
}

//=========================================================
// Nuevo Mapa - Después de cargar las entidades
//=========================================================
void CDirector::LevelInitPostEntity()
{
	DirectorManager->OnNewMap();
	DirectorMusic->OnNewMap();

	// Reiniciamos
	RestartPanic();
	RestartBoss();

	// Iniciamos
	NewMap();
}

//=========================================================
// Cerrando mapa - Antes de descargar las entidades
//=========================================================
void CDirector::LevelShutdownPreEntity()
{
	// Matamos a todos los hijos
	DirectorManager->KillChilds();

	// Paramos la música
	//DirectorMusic->Shutdown();
}

//=========================================================
// Cerrando mapa - Después de descargar las entidades
//=========================================================
void CDirector::LevelShutdownPostEntity()
{
}

//=========================================================
// Pensamiento - Antes de las entidades
//=========================================================
void CDirector::FrameUpdatePreEntityThink()
{
}

//=========================================================
// Pensamiento - Después de las entidades
//=========================================================
void CDirector::FrameUpdatePostEntityThink()
{
	// El Juego no ha empezado
	if ( !InRules )
	{
		// Iniciamos relajados
		Relaxed();

		return;
	}

	DirectorMusic->Think();

	// A trabajar!
	Work();
}

//=========================================================
// Comienza el trabajo del Director
//=========================================================
void CDirector::Work()
{
	// Imprimimos información de depuración
	if ( director_debug.GetBool() )
		PrintDebugInfo();

	// Estamos en modo de edición de mapa o desactivados
	if ( engine->IsInEditMode() || m_bDisabled )
		return;

	// Verificamos si la partida ha terminado
	CheckGameover();

	// La partida ha terminado
	if ( Is(GAMEOVER) )
		return;

	// Aún no toca
	if ( gpGlobals->curtime <= m_iNextWork )
		return;

	// Verificamos los hijos que estan vivos
	ManageChilds();

	// ¿Que haremos ahora?
	ModWork();

	// Calculemos mi nivel de enojo
	UpdateAngry();

	// Próxima actualización en .6s
	m_iNextWork = gpGlobals->curtime + 0.6f;
}

//=========================================================
// Verifica si la partida ha terminado
//=========================================================
void CDirector::CheckGameover()
{
	// Aún hay jugadores vivos
	if ( PlysManager->m_iWithLife > 0 || PlysManager->GetConnected() <= 0 || !ALLOW_GAMEOVER )
	{
		// Evitamos bugs...
		if ( Is(GAMEOVER) )
			Relaxed();

		return;
	}

	// Todos los jugadores han muerto, la partida ha terminado.
	if ( !Is(GAMEOVER) )
	{
		Set( GAMEOVER );

		color32 black = {0, 0, 0, 255};
		UTIL_ScreenFadeAll( black, (GAMEOVER_WAIT-3), 5.0f, FFADE_OUT );

		DirectorMusic->Stop();
		PlysManager->StopMusic();
	}

	DirectorMusic->Update();

	// Todavía no podemos reiniciar la partida
	if ( gpGlobals->curtime <= (m_flStatusTime+GAMEOVER_WAIT) )
		return;

	// Reiniciamos la partida
	DirectorManager->KillChilds();
	engine->ServerCommand("restartgame \n");
	PlysManager->RespawnAll();

	NewMap();
}

//=========================================================
// Calcula el enojo del Director
//=========================================================
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
	if ( m_iChildsSpawned >= 1000 )
	{
		iPoints += 20;
	}
	else
	{
		iPoints += round( ( m_iChildsSpawned / 500 ) * 10 ); // 20%
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
	if ( m_iPanicCount >= 5 )
	{
		iPoints += 10;
	}
	else
	{
		iPoints += round( ( m_iPanicCount / 5 ) * 10 ); // 10%
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
		iPoints -= 20;
	if ( InRules->IsSkillLevel(SKILL_MEDIUM) )
		iPoints -= 10;

	int iResult = round(iPoints / 25);
	++iResult;

	if ( iResult < 1 )
		iResult = 1;
	if ( iResult > 4 )
		iResult = 4;

	//Msg("[CDirector::UpdateAngry] %i - %i \n", iPoints, iResult);

	//
	// Enojo final
	//
	m_iAngry = (DirectorAngry)iResult;
}

//=========================================================
// Devuelve el modo de trabajo del Director
//=========================================================
bool CDirector::IsMode( int iStatus )
{
	int iValue = director_mode.GetInt();

	if ( iValue <= DIRECTOR_MODE_INVALID || iValue >= LAST_DIRECTOR_MODE )
		return DIRECTOR_MODE_NORMAL;

	return ( iStatus == iValue );
}

//=========================================================
// Devuelve el estado del Director en una cadena
//=========================================================
const char *CDirector::GetStatusName( DirectorStatus iStatus )
{
	if ( iStatus == INVALID )
		iStatus = m_iStatus;

	switch ( iStatus )
	{
		case SLEEP:
			return "Dormido";
		break;

		case RELAXED:
			return "Relajado";
		break;

		case PANIC:
			return "Evento de pánico";
		break;

		case POST_PANIC:
			return "Post-Pánico";
		break;

		case BOSS:
			return "Jefe";
		break;

		case CLIMAX:
			return "Climax";
		break;

		case GAMEOVER:
			return "Juego terminado";
		break;

		default:
			return "Desconocido";
		break;
	}
}

//=========================================================
// Devuelve el nivel de enojo del Director en una cadena
//=========================================================
const char *CDirector::GetAngryName( DirectorAngry iStatus )
{
	if ( iStatus == ANGRY_INVALID )
		iStatus = m_iAngry;

	switch ( iStatus )
	{
		case ANGRY_LOW:
			return "¡Feliz!";
		break;

		case ANGRY_MEDIUM:
			return "Normal";
		break;

		case ANGRY_HIGH:
			return "Enojado";
		break;

		case ANGRY_CRAZY:
			return "¡Furioso!";
		break;

		default:
			return "Desconocido";
		break;
	}
}

//=========================================================
// Devuelve el estao general de los jugadores en una cadena
//=========================================================
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

//=========================================================
// Imprime información útil del Director
//=========================================================
void CDirector::PrintDebugInfo()
{
	int iLine = 5;

	int iForPanic	= (int)floor(m_hLeft4Panic.GetRemainingTime() + 0.5);
	int iForFinish	= (int)floor(m_hLeft4FinishPanic.GetRemainingTime() + 0.5);
	int iForBoss	= (int)floor(m_hLeft4Boss.GetRemainingTime() + 0.5);

	Director_StatePrintf(++iLine, "DIRECTOR \n");
	Director_StatePrintf(++iLine, "--------------------------- \n");

	++iLine;

	Director_StatePrintf(++iLine, "Estado: %s \n", GetStatusName() );
	Director_StatePrintf(++iLine, "Poblacion: %i \n", m_iChildsSpawned );
	Director_StatePrintf(++iLine, "Pendientes: %i \n", m_iSpawnQueue );
	Director_StatePrintf(++iLine, "Vivos: %i ( maximo: %i ) \n", m_iChildsAlive, MaxChilds() );
	Director_StatePrintf(++iLine, "Puntos de creacion posibles: %i \n", DirectorManager->m_hSpawnNodes.Count() );
	Director_StatePrintf(++iLine, "Nivel de enojo: %s \n", GetAngryName() );
	Director_StatePrintf(++iLine, "Distancia maxima: %f \n", MaxDistance() );

	++iLine;

	Director_StatePrintf(++iLine, "MUSICA \n");
	Director_StatePrintf(++iLine, "--------------------------- \n");

	++iLine;

	Director_StatePrintf(++iLine, "Cercanos: %i \n", m_iChildsInDangerZone );

	if ( InRules->IsSurvivalTime() )
	{
		++iLine;

		Director_StatePrintf(++iLine, "MODE TIME SURVIVAL \n");
		Director_StatePrintf(++iLine, "--------------------------- \n");

		++iLine;
		
		if ( m_bSurvivalTimeStarted )
			Director_StatePrintf(++iLine, "Estado: HA COMENZADO \n");
		else
			Director_StatePrintf(++iLine, "Estado: NO HA COMENZADO \n");

		Director_StatePrintf(++iLine, "Tiempo: %is \n", m_iSurvivalTime );
	}

	if ( !Is(CLIMAX) )
	{
		++iLine;
		Director_StatePrintf(++iLine, "PANICO/HORDA \n");
		Director_StatePrintf(++iLine, "--------------------------- \n");
		++iLine;

		Director_StatePrintf(++iLine, "Hijos de la ultima: %i \n", m_iPanicChilds );
		Director_StatePrintf(++iLine, "Han pasado: %i eventos \n", m_iPanicCount );
		Director_StatePrintf(++iLine, "Duración de la ultima: %is \n", m_iLastPanicDuration );
		Director_StatePrintf(++iLine, "Muertes de los jugadores de la ultima: %i \n", m_iLastPanicDeaths );

		if ( !IsMode(DIRECTOR_MODE_PASIVE) )
			Director_StatePrintf(++iLine, "Proximo evento en: %is \n", iForPanic );

		if ( Is(PANIC) )
			Director_StatePrintf(++iLine, "Faltan: %i para terminar \n", iForFinish );
	}

	++iLine;
	Director_StatePrintf(++iLine, "AMBIENTE \n");
	Director_StatePrintf(++iLine, "--------------------------- \n");
	++iLine;

	Director_StatePrintf(++iLine, "Vivos: %i \n", m_iAmbientAlive );
	Director_StatePrintf(++iLine, "Cantidad: %i \n", m_iAmbientSpawned );

	++iLine;
	Director_StatePrintf(++iLine, "JEFE \n");
	Director_StatePrintf(++iLine, "--------------------------- \n");
	++iLine;

	Director_StatePrintf(++iLine, "Vivos: %i \n", m_iBossAlive );
	Director_StatePrintf(++iLine, "Cantidad: %i \n", m_iBossSpawned );
	Director_StatePrintf(++iLine, "Posibilidad en: %is \n", iForBoss );

	++iLine;
	Director_StatePrintf(++iLine, "JUGADORES \n");
	Director_StatePrintf(++iLine, "--------------------------- \n");
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

//=========================================================
// Devuelve la distancia minima de creación
//=========================================================
float CDirector::MinDistance()
{
	float flDistance = director_min_distance.GetFloat();

	// En un evento de pánico o Climax disminuimos la distancia
	if ( Is(PANIC) || Is(CLIMAX) )
		flDistance -= 300;

	return flDistance;
}

//=========================================================
// Devuelve la distancia máxima de creación
//=========================================================
float CDirector::MaxDistance()
{
	float flDistance = director_max_distance.GetFloat();

	// Disminumos la distancia en una partida díficil, habrá más hijos juntos
	if ( InRules->IsSkillLevel(SKILL_HARD) )
		flDistance -= 500;

	// En un evento de pánico o Climax disminuimos la distancia
	if ( Is(PANIC) || Is(CLIMAX) )
		flDistance -= 300;

	return flDistance;
}

//=========================================================
// Reinicia la información para un próximo Pánico
//=========================================================
void CDirector::RestartPanic()
{
	// No hay eventos de pánico "automaticos" en el modo pasivo
	if ( IsMode(DIRECTOR_MODE_PASIVE) )
		return;

	int iRand = RandomInt( 300, 400 );

	switch ( InRules->GetSkillLevel() )
	{
		case SKILL_MEDIUM:
			iRand = RandomInt( 200, 300 );
		break;

		case SKILL_HARD:
			iRand = RandomInt( 180, 230 );
		break;
	}

	m_hLeft4Panic.Start( iRand );
}

//=========================================================
// Reinicia la información para un próximo Jefe
//=========================================================
void CDirector::RestartBoss()
{
	// Fácil: De 300 a 400 segundos
	int iRand = RandomInt( 300, 400 );

	switch ( InRules->GetSkillLevel() )
	{
		// Medio: 250 a 400 segundos
		case SKILL_MEDIUM:
			iRand = RandomInt( 250, 400 );
		break;

		// Dificil: 100 a 350 segundos
		case SKILL_HARD:
			iRand = RandomInt( 100, 350 );
		break;
	}

	m_hLeft4Boss.Start( iRand );
}

//=========================================================
// Devuelve si el NPC esta muy lejos de los jugadores
//=========================================================
bool CDirector::IsTooFar( CBaseEntity *pEntity )
{
	float flDistance = MaxDistance();

	// 
	if ( PlysManager->GetWithLife() <= 0 )
		return false;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= PlysManager->Get( i );

		if ( !pPlayer )
			continue;

		// Es del equipo malvado
		if ( pPlayer->GetTeamNumber() == TeamEvil() )
			continue;

		// Obtenemos la distancia del NPC a este jugador
		float flDis	= pEntity->GetAbsOrigin().DistTo( pPlayer->GetAbsOrigin() );

		// Esta cerca
		if ( flDis <= (flDistance * 1.5) )
			return false;
	}

	return true;
}

//=========================================================
// Devuelve si el NPC esta muy cerca de los jugadores
//=========================================================
bool CDirector::IsTooClose( CBaseEntity *pEntity )
{
	float flDistance = MinDistance();

	if ( PlysManager->GetWithLife() <= 0 )
		return false;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer	= PlysManager->Get(i);

		if ( !pPlayer )
			continue;

		// Obtenemos la distancia del NPC a este jugador
		float flDis	= pEntity->GetAbsOrigin().DistTo( pPlayer->GetAbsOrigin() );

		// Esta cerca
		if ( flDis < flDistance )
			return true;
	}

	return false;
}

//=========================================================
// Mata a un hijo
//=========================================================
void CDirector::KillChild( CBaseEntity *pEntity )
{
	// Es un Jefe
	if ( pEntity->GetEntityName() == AllocPooledString(DIRECTOR_BOSS_NAME) && !pEntity->IsPlayer() )
	{
		UTIL_Remove( pEntity );
		return;
	}

	CTakeDamageInfo info( pEntity, pEntity, pEntity->GetMaxHealth(), DMG_GENERIC );
	pEntity->TakeDamage( info );
}

//=========================================================
// [Evento] Hemos creado un hijo
//=========================================================
void CDirector::OnSpawnChild( CAI_BaseNPC *pNPC )
{
	++m_iChildsSpawned;

	// Quitamos uno de la cola
	if ( m_iSpawnQueue > 0 )
		--m_iSpawnQueue;

	if ( Is(PANIC) )
		++m_iPanicChilds;

	// Hemos creado un hijo
	m_pInfoDirector->OnSpawnChild.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//=========================================================
// [Evento] Hemos creado un jefe
//=========================================================
void CDirector::OnSpawnBoss( CAI_BaseNPC *pNPC )
{
	// En el modo de Juego "Survival Time" los jefes 
	// deben conocer la ubicación de los jugadores
	if ( InRules->IsGameMode(GAME_MODE_SURVIVAL_TIME) )
	{
		CIN_Player *pPlayer = PlysManager->GetRandom( TEAM_HUMANS );

		if ( pPlayer )
		{
			pNPC->UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin() );
		}
	}

	++m_iBossSpawned;
	m_bBossPendient = false;

	// Hemos creado un jefe
	m_pInfoDirector->OnSpawnBoss.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//=========================================================
// [Evento] Hemos creado un hijo especial
//=========================================================
void CDirector::OnSpawnSpecial( CAI_BaseNPC *pNPC )
{
	++m_iSpecialsSpawned;

	// Hemos creado un especial
	m_pInfoDirector->OnSpawnSpecial.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//=========================================================
// [Evento] Hemos creado un hijo de ambiente
//=========================================================
void CDirector::OnSpawnAmbient( CAI_BaseNPC *pNPC )
{
	++m_iAmbientSpawned;

	// Hemos creado un especial
	m_pInfoDirector->OnSpawnAmbient.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//=========================================================
// [Evento] Un jugador ha sufrido daño
//=========================================================
void CDirector::OnPlayerHurt( CBasePlayer *pPlayer )
{
	m_iLastAttackTime = gpGlobals->curtime;
}

//=========================================================
// [Evento] Un jugador ha muerto
//=========================================================
void CDirector::OnPlayerKilled( CBasePlayer *pPlayer )
{
	// La horda se ha cobrado una vida
	if ( Is(PANIC) || Is(POST_PANIC) )
	{
		++m_iLastPanicDeaths;
	}

	// Nos hemos cobrado una vida más
	++m_iPlayersDead;
}

//=========================================================
// Establece el estado actual
//=========================================================
void CDirector::Set( DirectorStatus iStatus )
{
	Msg("[Director] %s -> %s \n", GetStatusName(), GetStatusName(iStatus) );

	m_iStatus		= iStatus;
	m_flStatusTime	= gpGlobals->curtime;
}

//=========================================================
// Devuelve si el Director quiere forzar una clase de hijo
//=========================================================
const char *CDirector::ForceChildClass()
{
	return NULL;
}

//=========================================================
// Devuelve la cantidad máxima de hijos que se pueden crear
//=========================================================
int CDirector::MaxChilds()
{
	int iMax = MAX_ALIVE;

	// Nos estamos quedando sin espacios ¡debemos parar!
	if ( Utils::RunOutEntityLimit( iMax+DIRECTOR_SECURE_ENTITIES ) )
		return 0;

	// Aumentamos o disminuimos según el nivel de dificultad
	switch ( InRules->GetSkillLevel() )
	{
		case SKILL_EASY:
			if ( Is(RELAXED) )
				iMax -= 20;
		break;

		case SKILL_MEDIUM:
			if ( Is(RELAXED) )
				iMax -= 15;
		break;

		case SKILL_HARD:
			if ( Is(RELAXED) )
				iMax -= 5;
		break;
	}

	//
	// Los Jugadores tienen una mala partida...
	//
	if ( PlysManager->GetStatus() <= STATS_POOR )
		iMax -= 15;

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

	//
	// Algo aún más interesante ha ocurrido
	//
	if ( iMax <= 0 )
		iMax = 30;

	return iMax;
}

//=========================================================
// Devuelve el tipo de Hijo
//=========================================================
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

//=========================================================
// Maneja a los hijos
//=========================================================
void CDirector::ManageChilds()
{
	// Reiniciamos la información
	m_iChildsAlive			= 0;
	m_iChildsInDangerZone	= 0;
	m_iChildsVisibles		= 0;
	m_iBossAlive			= 0;
	m_iAmbientSpawned		= 0;
	m_iAmbientAlive			= 0;

	CBaseEntity *pChild = NULL;

	//
	// NPC
	//
	do
	{
		pChild = (CBaseEntity *)gEntList.FindEntityByName( pChild, "director_*" );

		// El hijo ya no existe o acaba de ser eliminado
		if ( !pChild || !pChild->IsAlive() )
			continue;

		// No es de los mios...
		if ( pChild->GetTeamNumber() != TeamEvil() )
			continue;

		CheckChild( pChild );

	} while ( pChild );
}

//=========================================================
//=========================================================
void CDirector::CheckChild( CBaseEntity *pEntity )
{
	int iType = GetChildType( pEntity );

	//
	// Verificamos si esta muy lejos o no puede atacar
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
			--m_iChildsSpawned;
		}

		return;
	}

	//
	// Te estan viendo
	//
	if ( PlysManager->IsVisible(pEntity) )
	{
		++m_iChildsVisibles;
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
		PlysManager->GetNear( pEntity->GetAbsOrigin(), flDistance, TeamGood() );

		// Tienes a un jugador como enemigo y estas muy cerca de el
		if ( pEntity->GetEnemy() && pEntity->GetEnemy()->IsPlayer() && flDistance <= 800 )
			++m_iChildsInDangerZone;

		++m_iChildsAlive;
	}
}

//=========================================================
// Devuelve si el Hijo puede ser eliminado por estar muy
// lejos de los Jugadores
//=========================================================
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
	PlysManager->GetNear( pEntity->GetAbsOrigin(), flDistance, TeamGood() );
	
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

//=========================================================
// Devuelve si el Hijo puede ser eliminado por no tener
// una ruta de ataque.
//=========================================================
bool CDirector::CheckUnreachable( CBaseEntity *pEntity )
{
	// No eres un NPC
	if ( !pEntity->IsNPC() )
		return false;

	// Te estan viendo
	if ( PlysManager->InVisibleCone(pEntity) )
		return false;

	// Nos piden que no hagamos esta verificación
	if ( !CHECK_UNREACHABLE )
		return false;

	// No tiene un Enemigo o no es un Jugador
	if ( !pEntity->GetEnemy() || !pEntity->GetEnemy()->IsPlayer() )
		return false;

	// ¿Me estas diciendo que no puedes atacar? Dale paso a alguien más
	if ( pEntity->MyNPCPointer()->IsUnreachable( pEntity->GetEnemy() ) )
	{
		return true;
	}

	return false;
}

//=========================================================
// Devuelve si el Director le ha proporcionado información
// acerca de un enemigo al Hijo
//=========================================================
bool CDirector::CheckNewEnemy( CBaseEntity *pEntity )
{
	// No es un NPC
	if ( !pEntity->IsNPC() )
		return false;

	int iType = GetChildType( pEntity );

	// Solo podemos dar un enemigo si estamos en un evento de Pánico o Climax
	if ( !Is(PANIC) && !Is(CLIMAX) )
		return false;

	// Los Jefes no deben alertarse en un evento de Pánico
	// TODO: ¿O si?
	if ( Is(PANIC) && iType == DIRECTOR_BOSS )
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
		pEntity->MyNPCPointer()->SetEnemy( pPlayer );
		pEntity->MyNPCPointer()->UpdateEnemyMemory( pPlayer, pPlayer->GetAbsOrigin() );
	}
	
	return true;
}

//=========================================================
// Devuelve si es posible crear más hijos
//=========================================================
bool CDirector::CanSpawnChilds()
{
	int iMax = MaxChilds();

	// Nos estamos quedando sin espacios ¡debemos parar!
	if ( Utils::RunOutEntityLimit( MAX_ALIVE+DIRECTOR_SECURE_ENTITIES ) )
		return false;

	// En Post-Pánico, queremos eliminar, no crear...
	if ( Is(POST_PANIC) )
		return false;

	// Hijos al limite, no podemos con más
	if ( m_iChildsAlive >= iMax )
		return false;

	// Estamos en proceso de crear más
	if ( m_bSpawning )
		return false;

	// Estamos en un evento de Climax o Pánico ¡CLARO!
	if ( Is(CLIMAX) || Is(PANIC) )
		return true;

	// Si estamos con un Jefe solo crear hijos si estamos en Dificil
	if ( Is(BOSS) && InRules->IsSkillLevel(SKILL_HARD) )
		return true;

	// No tienen la partida muy fácil
	if ( PlysManager->GetStatus() <= STATS_POOR )
	{
		// Suertudos
		if ( RandomInt(1, 100) > 20 )
			return false;
	}

	return true;
}

//=========================================================
// Procesa a todos los tipos de hijos
//=========================================================
void CDirector::HandleAll()
{
	HandleChilds();
	HandleAmbient();
	HandleBoss();
	HandleSpecials();
}

//=========================================================
// Procesa la creación de hijos normales
//=========================================================
void CDirector::HandleChilds()
{
	// No podemos crear más hijos ahora mismo
	if ( !CanSpawnChilds() )
		return;

	// Hijos que faltan para llegar al limite
	m_iSpawnQueue = MaxChilds() - m_iChildsAlive;

	// Comenzamos la creación
	DirectorManager->StartSpawn( DIRECTOR_CHILD );
}

//=========================================================
// Devuelve si es posible crear hijos de ambiente
//=========================================================
bool CDirector::CanSpawnAmbient()
{
	// No debe haber más de 8 hijos de ambiente
	if ( m_iAmbientAlive >= 8 )
		return false;

	// Estamos en proceso de crear más
	if ( m_bSpawning )
		return false;

	// Aún no tica
	if ( gpGlobals->curtime < m_iNextAmbientSpawn )
		return false;

	// Nos estamos quedando sin espacios ¡debemos parar!
	if ( Utils::RunOutEntityLimit( MAX_ALIVE+DIRECTOR_SECURE_ENTITIES ) )
		return false;

	// No hay lugares donde crear
	if ( DirectorManager->m_hAmbientSpawnAreas.Count() <= 0 )
		return false;

	return true;
}

//=========================================================
// Procesa la creación de hijos de ambiente
//=========================================================
void CDirector::HandleAmbient()
{
	// No podemos crear más hijos ahora mismo
	if ( !CanSpawnAmbient() )
		return;

	DirectorManager->StartSpawn( DIRECTOR_AMBIENT_CHILD );

	m_iNextAmbientSpawn = gpGlobals->curtime + director_spawn_ambient_interval.GetInt();
}

//=========================================================
// Devuelve si es posible crear un jefe
//=========================================================
bool CDirector::CanSpawnBoss()
{
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
	if ( Is(CLIMAX) )
		return true;

	// No hace falta
	if ( m_iAngry <= ANGRY_LOW )
		return false;

	// No ha pasado el tiempo necesario para un jefe
	if ( !m_hLeft4Boss.IsElapsed() )
		return false;

	// Un poco de suerte
	if ( RandomInt(1, 100) > 90 )
		return false;

	return true;
}

//=========================================================
// Procesa la creación de jefes
//=========================================================
void CDirector::HandleBoss()
{
	// Han eliminado al Jefe
	if ( Is(BOSS) && m_iBossAlive <= 0 )
	{
		// Fácil: Pasamos a Relajado
		if ( InRules->IsSkillLevel(SKILL_EASY) )
		{
			Relaxed();
		}

		// Pasamos a Pánico
		else
		{
			Panic();
		}
	}

	// Es hora de crear un Jefe
	if ( CanSpawnBoss() )
	{
		DirectorManager->StartSpawn( DIRECTOR_BOSS );
		RestartBoss();
	}
}

//=========================================================
// Pone pendiente a un jefe
//=========================================================
void CDirector::TryBoss()
{
	if ( m_iBossSpawned > 0 )
		--m_iBossSpawned;

	m_bBossPendient = true;
}

//=========================================================
// Devuelve si es posible crear un hijo especial
//=========================================================
bool CDirector::CanSpawnSpecial()
{
	// TODO
	return false;
}

//=========================================================
// Procesa la creación de jefes
//=========================================================
void CDirector::HandleSpecials()
{
	// No podemos
	if ( !CanSpawnSpecial() )
		return;

	// TODO
}

//=========================================================
//=========================================================
//=========================================================

void CC_ForceRelax()
{
	if ( !Director )
		return;

	Director->Relaxed();
}

void CC_ForcePanic()
{
	if ( !Director )
		return;

	Director->Panic( UTIL_GetCommandClient(), 60 );
}

void CC_ForceBoss()
{
	if ( !Director )
		return;

	Director->TryBoss();
}

void CC_ForceClimax()
{
	if ( !Director )
		return;

	Director->Climax();
}

void CC_ZSpawn()
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	if ( !pPlayer )
		return;

	trace_t tr;
	Vector forward;

	AngleVectors( pPlayer->EyeAngles(), &forward );
	UTIL_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * 300.0f, MASK_SOLID, 
		pPlayer, COLLISION_GROUP_NONE, &tr );

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
}

void CC_ZSpawnBatch()
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	if ( !pPlayer )
		return;

	trace_t tr;
	Vector forward;

	AngleVectors( pPlayer->EyeAngles(), &forward );
	UTIL_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * 300.0f, MASK_SOLID, 
		pPlayer, COLLISION_GROUP_NONE, &tr );

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
}

void CC_ZSpawnRandom()
{
	DirectorManager->SpawnChild( DIRECTOR_CHILD );
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

static ConCommand director_force_relax("director_force_relax", CC_ForceRelax);
static ConCommand director_force_panic("director_force_panic", CC_ForcePanic);
static ConCommand director_force_boss("director_force_boss", CC_ForceBoss);
static ConCommand director_force_climax("director_force_climax", CC_ForceClimax);
static ConCommand director_update("director_update", CC_UpdateNodes, "Actualiza los nodos candidatos para crear hijos manualmente.");

static ConCommand z_spawn("z_spawn", CC_ZSpawn);
static ConCommand z_spawn_batch("z_spawn_batch", CC_ZSpawnBatch);
static ConCommand z_spawn_random("z_spawn_random", CC_ZSpawnRandom);

static ConCommand director_stop("director_stop", CC_DirectorStop);
static ConCommand director_start("director_start", CC_DirectorStart);
static ConCommand director_kill_childs("director_kill_childs", CC_DirectorKill);