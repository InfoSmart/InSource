//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef DIRECTORDEFS_H
#define DIRECTORDEFS_H

#pragma once

//==============================================
// Director
//==============================================

#define CUSTOM_DIRECTOR
//#define CUSTOM_DIRECTOR_MUSIC
//#define CUSTOM_DIRECTOR_MANAGER

#define DIRECTOR_MAX_SPAWN_TRYS		5		// Numero de intentos máximo para la creación de un hijo
#define DIRECTOR_MAX_NAV_AREAS		3000	// Numero máximo de areas de navegación a usar para la creación de un hijo
#define DIRECTOR_MAX_NAV_TRYS		20		//

#define DIRECTOR_SPAWN_SECUREDISTANCE	500	// Distancia segura

#define DIRECTOR_POPULATION_FILE	"scripts/director_population.txt"
#define DIRECTOR_ITEMS_FILE			"scripts/director_items.txt"

#define DIRECTOR_CHILD_NAME		"director_child"			// Nombre de los hijos
#define DIRECTOR_BOSS_NAME		"director_boss"				// Nombre de los jefes
#define DIRECTOR_SPECIAL_NAME	"director_special_child"	// Nombre de los hijos especiales
#define DIRECTOR_AMBIENT_NAME	"director_ambient_child"	// Nombre de los hijos de ambiente
#define DIRECTOR_CUSTOM_NAME	"director_custom_child"

#define DIRECTOR_CON_NORMAL Color(1, 58, 223)
#define DIRECTOR_CON_DEBUG	Color(116, 1, 223)
#define DIRECTOR_CON_MUSIC	Color(255, 128, 0)

#define DIRECTOR_SECURE_ENTITIES	100	// Si el limite de entidades pasa esta cantidad, el Director dejará de crear hijos
#define DIRECTOR_MAX_CHILDS 50

enum DirectorStatus
{
	ST_INVALID = -1,

	ST_NORMAL,
	ST_BOSS,
	ST_COMBAT,
	ST_FINALE,
	ST_GAMEOVER,

	LAST_STATUS
};

enum DirectorPhase
{
	PH_INVALID = -1,

	PH_RELAX,			// Tiempo de Relajación. No se crean Hijos
	PH_BUILD_UP,		// Se deben crear Hijos
	PH_CRUEL_BUILD_UP,	// Se deben crear Hijos ¡siempre! Sin descansos
	PH_SANITY_FADE,		// No se deben crear Hijos hasta mejorar la cordura
	PH_POPULATION_FADE, // No se deben crear Hijos hasta no quedar muchos hijos
	PH_EVENT,			// Evento

#ifdef APOCALYPSE
	PH_CORPSE_BUILD,	// Construcción de cadaveres, como decoración
#endif

	LAST_PHASE,
};

enum DirectorAngry
{
	ANGRY_INVALID = 0,
	ANGRY_LOW,
	ANGRY_MEDIUM,
	ANGRY_HIGH,
	ANGRY_CRAZY,

	LAST_ANGRY
};

enum
{
	DIRECTOR_MODE_INVALID = -1,
	DIRECTOR_MODE_PASIVE,
	DIRECTOR_MODE_NORMAL,
	DIRECTOR_MODE_CRUEL,

	LAST_DIRECTOR_MODE,
};

enum
{
	DIRECTOR_CHILD = 0,
	DIRECTOR_SPECIAL_CHILD,
	DIRECTOR_BOSS,
	DIRECTOR_AMBIENT_CHILD,

	DIRECTOR_CUSTOMCHILD, // Use en casos especiales

	LAST_DIRECTOR_SPAWN,
};

#endif //DIRECTORDEFS_H