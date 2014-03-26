//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef DIRECTORDEFS_H
#define DIRECTORDEFS_H

#pragma once

//==============================================
// Director
//==============================================

#define DIRECTOR_CUSTOM
//#define DIRECTOR_CUSTOM_MUSIC
//#define DIRECTOR_CUSTOM_MANAGER

#define DIRECTOR_MAX_SPAWN_TRYS		5		// Numero de intentos máximo para la creación de un hijo
#define DIRECTOR_MAX_NAV_AREAS		3000	// Numero máximo de areas de navegación a usar para la creación de un hijo
#define DIRECTOR_MAX_NAV_TRYS		20		//

#define DIRECTOR_SPAWN_SECUREDISTANCE	300	// Distancia segura

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

#define DIRECTOR_SECURE_ENTITIES	50	// Si el limite de entidades pasa esta cantidad, el Director dejará de crear hijos

enum DirectorStatus
{
	INVALID = -1,
	SLEEP,		// Dormido
	RELAXED,	// Relajado
	PANIC,		// Pánico
	POST_PANIC, // Post-Pánico
	BOSS,		// Jefe
	CLIMAX,		// Climax
	GAMEOVER,	// Juego terminado

	LAST_STATUS,
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