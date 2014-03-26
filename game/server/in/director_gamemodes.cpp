//==== InfoSmart. Todos los derechos reservados .===========//
//
// Inteligencia artificial encargada de la creaci�n de enemigos (hijos)
// adem�s de poder controlar la m�sica, el clima y otros aspectos
// del juego.
//==========================================================//

#include "cbase.h"
#include "director.h"

#include "in_gamerules.h"

#include "director_music.h"
#include "director_manager.h"

#ifdef APOCALYPSE
	#include "ap_director_music.h"
#endif

#include "players_manager.h"

//=========================================================
// Bucle de ejecuci�n de tareas para el Mod
//=========================================================
void CDirector::ModWork()
{
	// Depende del modo de juego
	switch ( InRules->GameMode() )
	{
		case GAME_MODE_NONE:
		case GAME_MODE_COOP:
		default:
			NormalUpdate();
		break;

		case GAME_MODE_SURVIVAL:
			SurvivalUpdate();
		break;

		case GAME_MODE_SURVIVAL_TIME:
			SurvivalTimeUpdate();
		break;
	}

	// M�sica
	if ( m_bMusicEnabled )
		DirectorMusic->Update();

	// Mientras no estemos dormidos podemos crear hijos
	if ( !Is(SLEEP) )
		HandleAll();
}

//=========================================================
// Pensamiento para el modo de juego "Normal"
//=========================================================
void CDirector::NormalUpdate()
{
	// Estamos en un evento de p�nico
	if ( Is(PANIC) )
	{
		// Un evento de p�nico no infinito
		if ( !IsInfinitePanic() )
		{
			// El p�nico ha finalizado, pasamos al Post
			if ( m_hLeft4FinishPanic.IsElapsed() )
			{
				Set( POST_PANIC );
				m_nPostPanicLimit.Start( 5 );
			}
		}
	}

	// Post-P�nico, esperamos a que la batalla termine...
	if ( Is(POST_PANIC) )
	{
		// Empezamos a eliminar hijos
		if ( m_nPostPanicLimit.IsElapsed() && m_nPostPanicLimit.HasStarted() )
			DirectorManager->KillChilds( true );

		// El evento de p�nico ha finalizado
		if ( m_iChildsAlive <= 5 )
		{
			Sleep();
			m_nPostPanicLimit.Invalidate();
		}
	}

	// Estamos en p�nico o post-p�nico.
	if ( Is(PANIC) || Is(POST_PANIC) )
	{
		m_nLastPanicDuration.Start();
	}

	// Estamos relajados o durmiendo
	if ( Is(RELAXED) || Is(SLEEP) )
	{
		// No estamos en modo de Pasivo
		if ( !IsMode(DIRECTOR_MODE_PASIVE) )
		{
			// Toca un evento de p�nico
			if ( m_hLeft4Panic.IsElapsed() )
				Panic();
		}

		// Estamos dormidos
		if ( Is(SLEEP) )
		{
			// Los jugadores recuperaron su cordura
			if ( PlysManager->GetAllSanity() >= 80.0f )
			{
				// Volvamos a nuestro evento de p�nico
				if ( m_bPanicSuspended )
				{
					Panic( 0, IsInfinitePanic() );
					m_bPanicSuspended = false;
				}

				// Si no pasamos a relajado
				else
				{
					Relaxed();
				}
			}
		}
	}

	// Los jugadores estan experimentando mucho estres
	if ( !InRules->IsSkillLevel(SKILL_HARD) && PlysManager->GetAllSanity() <= 30.0f && !Is(SLEEP) )
	{
		// Estabamos en un evento de p�nico infinito, la reanudamos al despertar
		if ( IsInfinitePanic() && Is(PANIC) )
			m_bPanicSuspended = true;

		// Dormimos
		Sleep();
	}
}

//=========================================================
// Pensamiento para el modo de juego "Survival"
//=========================================================
void CDirector::SurvivalUpdate()
{
	// En el Survival el Director siempre debe estar Relajado
	if ( !Is(RELAXED) )
	{
		Relaxed();
	}
}

//=========================================================
// Pensamiento para el modo de juego "Survival Time"
//=========================================================
void CDirector::SurvivalTimeUpdate()
{
	// No hemos empezado
	if ( !m_bSurvivalTimeStarted )
		return;

	// Han sobrevivido un segundo m�s
	++m_iSurvivalTime;

	// Algo sospechoso ha ocurrido...
	if ( Is(CLIMAX) || Is(RELAXED) )
		Panic( 0, true );

	// Mucha perdida de cordura!
	if ( PlysManager->GetAllSanity() <= 30.0 && !Is(SLEEP) && !Is(BOSS) )
	{
		m_bPanicSuspended = true;
		Sleep();
	}

	// Estamos dormidos
	if ( Is(SLEEP) )
	{
		// Los jugadores ya no estan estresados
		if ( PlysManager->GetAllSanity() >= 80.0f )
		{
			// Volvamos a nuestro evento de p�nico
			Panic( 0, true );
		}
	}
}