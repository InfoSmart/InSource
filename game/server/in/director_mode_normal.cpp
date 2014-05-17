//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Modo de Juego: Normal
//
// 
//
//==========================================================================================//

#include "cbase.h"
#include "director_mode_normal.h"

#include "director_manager.h"

#include "in_player.h"
#include "players_manager.h"

#include "director_spawn.h"

//====================================================================
// Constantes y Macros
//====================================================================

#define MAX_COMBAT_TIME			150
#define CORPSE_BUILD_INTERVAL	RandomInt( 20, 40 )

//====================================================================
//====================================================================
CDR_Normal_GM::CDR_Normal_GM()
{
}

//====================================================================
// Procesa el funcionamiento del Modo de Juego
//====================================================================
void CDR_Normal_GM::Work()
{
	// Iniciamos el cronometro
	if ( !m_pCorpseBuild.HasStarted() )
		m_pCorpseBuild.Start( CORPSE_BUILD_INTERVAL );

	//
	// Estado: EN COMBATE
	//
	if ( Director->IsStatus(ST_COMBAT) )
	{
		// Duración del evento de Combate
		Director->m_iLastCombatSeconds = Director->m_nLastCombatDuration.GetElapsedTime();

		// Combate no infinito
		if ( !Director->IsPhase(PH_CRUEL_BUILD_UP) )
		{
			// Hemos creado todas las hordas o el tiempo máximo ha sido pasado
			if ( Director->m_iCombatWaves <= -1 || gpGlobals->curtime >= (Director->m_flStatusTime+MAX_COMBAT_TIME) )
			{
				Director->m_nLastCombatDuration.Invalidate();

				Director->SetStatus( ST_NORMAL );
				StartRelax();

				return;
			}
		}
	}

	//
	// Fase: Creación cruel
	// En esta fase no hay descansos, por lo que debemos eliminar a los hijos que creemos que no lograron ir a su objetivo
	//
	if ( Director->IsPhase(PH_CRUEL_BUILD_UP) )
	{
		if ( m_nCruelKill.IsElapsed() )
		{
			DirectorManager->KillChilds( true );
			m_nCruelKill.Start( RandomInt(10, 20) );
		}
	}

	//
	// Fase: RELAJADO
	//
	if ( Director->IsPhase(PH_RELAX) )
	{
		if ( Director->IsStatus(ST_FINALE) )
		{
			// Si hay menos de 3 hijos en Climax, debemos parar
			if ( Director->m_iCommonAlive > 3 )
			{
				// Aún no ha terminado
				if ( gpGlobals->curtime <= (Director->m_flPhaseTime + Director->m_iPhaseValue) )
					return;
			}
		}
		else
		{
			// Aún no ha terminado
			if ( gpGlobals->curtime <= (Director->m_flPhaseTime + Director->m_iPhaseValue) )
				return;
		}

		// Ha terminado, empezamos a crear Hijos
		Director->SetPhase( PH_BUILD_UP );
		return;
	}

	//
	// Fase: DESVANECIMIENTO DE CORDURA
	//
	if ( Director->IsPhase(PH_SANITY_FADE) )
	{
		// Los Jugadores tienen poca cordura
		if ( PlysManager->GetAllSanity() < 20 )
			return;

		// Pasamos a Relajado
		StartRelax();
		return;
	}

	//
	// Fase: DESVANECIMIENTO DE HIJOS
	//
	if ( Director->IsPhase(PH_POPULATION_FADE) )
	{
		// Aún hay más de 10 hijos
		if ( Director->m_iCommonAlive > 10 )
			return;

		// Pasamos a Relajado
		StartRelax();
		return;
	}

#ifdef APOCALYPSE

	//
	// Fase: DECORACIÓN CON CADAVERES
	//
	if ( m_pCorpseBuild.IsElapsed() && !Director->IsStatus(ST_COMBAT) && !Director->IsStatus(ST_FINALE) )
	{
		Director->SetPhase( PH_CORPSE_BUILD );
		
		for ( int i = 0; i <= 5; ++i )
		{
			DirectorManager->SpawnChild( DIRECTOR_CHILD );
		}

		Director->SetPhase( PH_BUILD_UP );


		// Reiniciamos
		m_pCorpseBuild.Start( CORPSE_BUILD_INTERVAL );
	}

#endif

	// Creación de Hijos
	Director->HandleAll();

	// Verificamos
	Check();
}

//====================================================================
// Verifica el estado actual de la partida
//====================================================================
void CDR_Normal_GM::Check()
{
	// Los Jugadores tienen muy poca cordura
	if ( PlysManager->GetAllSanity() < 20 && !Director->IsStatus(ST_FINALE) )
	{
		Director->SetPhase( PH_SANITY_FADE );
		return;
	}

	// Combate o Climax
	if ( Director->IsStatus(ST_COMBAT) || Director->IsStatus(ST_FINALE) )
	{
		if ( Director->m_iCommonAlive >= (Director->MaxChilds()-2) && !Director->IsPhase(PH_POPULATION_FADE) && !Director->IsPhase(PH_CRUEL_BUILD_UP) )
		{
			// Esto califica como una horda
			if ( Director->m_iCombatWaves > -1 )
				--Director->m_iCombatWaves;

			Director->SetPhase( PH_POPULATION_FADE );
			return;
		}
	}
}

//====================================================================
// Inicia la fase "RELAX" y calcula la mejor duración
//====================================================================
void CDR_Normal_GM::StartRelax()
{
	// Duración de RELAX
	int iSeconds = 3;

	// El Director no esta tan enojado
	if ( Director->m_iAngry <= ANGRY_MEDIUM )
		iSeconds += 3;

	// No tiene una partida buena
	if ( PlysManager->GetStatus() <= STATS_MED )
		iSeconds += 3;

	// Aumentamos el tiempo si hay Jugadores abatidos
	int iDejected	= PlysManager->GetAllDejected();
	iSeconds		+= (2 * iDejected);

	if ( iSeconds > 15 )
		iSeconds = 15;

	// Establecemos RELAX
	Director->SetPhase( PH_RELAX, iSeconds );
}

//====================================================================
//====================================================================
void CDR_Normal_GM::OnSetPhase( DirectorPhase iPhase, int iValue )
{
	if ( iPhase == PH_CRUEL_BUILD_UP )
	{
		m_nCruelKill.Start( 20 );
	}
}

//====================================================================
// [Evento] El Director ha creado un hijo
//====================================================================
void CDR_Normal_GM::OnSpawnChild( CBaseEntity *pEntity )
{
	// Solo eres de decoración, te matamos
	if ( Director->IsPhase(PH_CORPSE_BUILD) )
	{
		Director->KillChild( pEntity );
	}
}