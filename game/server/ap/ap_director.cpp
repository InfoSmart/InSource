//==== InfoSmart. Todos los derechos reservados .===========//
//
// Inteligencia artificial encargada de la creación de enemigos (hijos)
// además de poder controlar la música, el clima y otros aspectos
// del juego.
//
//==========================================================//

#include "cbase.h"
#include "ap_director.h"

#include "director.h"
#include "director_manager.h"
#include "ap_director_music.h"

#include "in_player.h"
#include "players_manager.h"

#include "in_gamerules.h"
#include "in_utils.h"

#include "ai_basenpc.h"

CAP_Director g_Director;
CAP_Director* Director = &g_Director;

//=========================================================
// Comandos de consola
//=========================================================

ConVar ap_director_cruel( "ap_director_cruel", "0", FCVAR_SERVER );

//=========================================================
// Macros
//=========================================================

#define IS_CRUEL ap_director_cruel.GetBool()

//=========================================================
// Devuelve la distancia máxima de creación
//=========================================================
float CAP_Director::MaxDistance()
{
	float flDistance = director_max_distance.GetFloat();

	// Disminumos la distancia en una partida díficil, habrá más hijos juntos
	if ( InRules->IsSkillLevel(SKILL_HARD) )
		flDistance -= 500;

	// Apocalypse: Aumentamos la distancia
	if ( Is(PANIC) || Is(CLIMAX) )
		flDistance += 1500;

	return flDistance;
}

//=========================================================
// Devuelve el modo de trabajo del Director
//=========================================================
bool CAP_Director::IsMode( int iStatus )
{
	// Estamos jugando en "Cruel"
	if ( IS_CRUEL && iStatus == DIRECTOR_MODE_CRUEL )
		return true;

	if ( iStatus == DIRECTOR_MODE_PASIVE )
		return true;

	return false;
}

//=========================================================
// La Música de Enojo se ha reproducido
//=========================================================
void CAP_Director::OnAngryMusicPlay()
{
	// Estamos muy enojados, aprovechemos la música de fondo y mandemos algunos infectados a atacar
	if ( IsAngry(ANGRY_CRAZY) )
	{
		if ( RandomInt(0, 5) == 5 )
			DirectorManager->Disclose();
	}
}

//=========================================================
// Devuelve la cantidad máxima de hijos que se pueden crear
//=========================================================
int CAP_Director::MaxChilds()
{
	int iMax		= BaseClass::MaxChilds();
	int iSanity		= PlysManager->GetAllSanity();

	if ( iMax <= 0 )
		return 0;

	//
	// Un evento de Pánico o Climax
	//
	if ( Is(PANIC) || Is(CLIMAX) )
	{
		// El numero de Infectados depende del nivel de cordura de los Jugadores
		if ( iSanity >= 30 )
		{
			// Si los Jugadores tienen una cordura baja... que la suerte este con ellos.
			int iToAdd = (int)( 1500 / iSanity );
			iMax = iToAdd;
		}
	}

	return iMax;
}

//=========================================================
// Devuelve si el Director le ha proporcionado información
// acerca de un enemigo al Hijo
//=========================================================
bool CAP_Director::CheckNewEnemy( CBaseEntity *pEntity )
{
	// No es un NPC
	if ( !pEntity->IsNPC() )
		return false;

	CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();

	// Solo en un evento de Pánico o Climax
	if ( !Is(PANIC) && !Is(CLIMAX) )
		return false;

	// Ya tiene un enemigo
	if ( pNPC->GetEnemy() )
	{
		// En Climax siempre deben conocer la ubicación del enemigo
		if ( Is(CLIMAX) )
		{
			pNPC->UpdateEnemyMemory( pNPC->GetEnemy(), pNPC->GetEnemy()->GetAbsOrigin() );
			return true;
		}

		return false;
	}

	// Ya tiene una ruta
	if ( pNPC->GetNavigator()->IsGoalSet() )
		return false;

	// ¿What?
	if ( !m_vecPanicPosition.IsValid() )
		return false;

	AI_NavGoal_t goal( m_vecPanicPosition, ACT_RUN, AIN_HULL_TOLERANCE );
	pNPC->GetNavigator()->SetGoal( goal );
	
	return true;
}

//=========================================================
// Activa el estado de Pánico
//=========================================================
void CAP_Director::Panic( CBasePlayer *pCaller, int iSeconds, bool bInfinite )
{
	BaseClass::Panic( pCaller, iSeconds, bInfinite );

	// No hay causante, seleccionamos uno al azar
	if ( !pCaller )
		pCaller = PlysManager->GetRandom( TeamGood() );

	// Establecemos la ubicación donde los infectados iran a investigar
	if ( pCaller )
	{
		m_vecPanicPosition = pCaller->GetAbsOrigin();
	}
}