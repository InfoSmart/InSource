//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef IN_SHAREDDEFS_H
#define IN_SHAREDDEFS_H

#pragma once

#include "cbase.h"
#include "ai_activity.h"
#include "in_playeranimstate.h"

#ifndef CLIENT_DLL
#include "nav.h"
#endif

//==============================================
// Configuración del Juego
//==============================================

#define GAME_CAN_SPRINT
#define GAME_CAN_PRONE
#define GAME_CAN_SHOOT_WHILE_SPRINTING
#define GAME_CAN_SHOOT_ON_LADDERS
#define GAME_CAN_SHOOT_WHILE_JUMPING

//==============================================
// Configuración
//==============================================

#ifdef APOCALYPSE
	#define GAME_DESCRIPTION "Apocalypse-22"
#else
	#define GAME_DESCRIPTION "InSource"
#endif

// Comandos
#define FCVAR_SERVER				FCVAR_REPLICATED
#define FCVAR_ONLY_SERVER			FCVAR_SPONLY | FCVAR_SERVER | FCVAR_SERVER_CAN_EXECUTE
#define FCVAR_ONLY_SERVER_NOTIFY	FCVAR_ONLY_SERVER | FCVAR_NOTIFY

// Generación de nodos
#define MAX_NAV_AREAS		9999
#define MAX_NODES_PER_AREA	60

enum
{
	STATS_POOR = 1,
	STATS_MED,
	STATS_GOOD,
	STATS_PERFECT,
};

//==============================================
// Navigation Mesh
//==============================================
#ifndef CLIENT_DLL

enum
{
	NAV_MESH_DONT_SPAWN		= NAV_MESH_FIRST_CUSTOM,
	NAV_MESH_PLAYER_START	= 0x00020000,
	NAV_MESH_OBSCURED		= 0x00040000,
	NAV_MESH_SPAWN_AMBIENT	= 0x00080000,
	NAV_MESH_SPAWN_HERE		= 0x00160000
};

#endif

//==============================================
// Colisiones
//==============================================

enum
{
	COLLISION_GROUP_NOT_BETWEEN_THEM = LAST_SHARED_COLLISION_GROUP,
	LAST_SHARED_IN_COLLISION_GROUP
};

//==============================================
// Jugadores
//==============================================

#define FLASHLIGHT_DISTANCE			1000	// Distancia de la linterna
#define FLASHLIGHT_OTHER_DISTANCE	300		// Distancia de la linterna de otro jugador

// Velocidad para empezar la animación
#define ANIM_WALK_SPEED				5.0f
#define ANIM_RUN_SPEED				180.0f

// Velocidad del jugador
#define PLAYER_WALK_SPEED			160.0f
#define PLAYER_RUN_SPEED			250.0f

//==============================================
// I.A.
//==============================================

#ifndef CLIENT_DLL

#endif

//==============================================
// Equipos
//==============================================

static char *sTeamNames[] =
{
	"#Team_Unassigned",
	"#Team_Spectator",

#ifdef APOCALYPSE
	"#Team_Humans",		// Humanos
	"#Team_Soldiers",	// Soldados
	"#Team_Infected"	// Infectados
#endif
};

enum
{
	//TEAM_UNASSIGNED,
	//TEAM_SPECTATOR

#ifdef APOCALYPSE
	TEAM_HUMANS = LAST_SHARED_TEAM + 1,
	TEAM_SOLDIERS,
	TEAM_INFECTED
#endif
};

//==============================================
// Modos de juego
//==============================================
enum
{
	GAME_MODE_NONE = 0,

#ifdef APOCALYPSE
	GAME_MODE_COOP,				// Coop
	GAME_MODE_SURVIVAL,			// Survival
	GAME_MODE_SURVIVAL_TIME,	// Survival Time
#endif
};

//==============================================
// Tipos de Spawn
//==============================================

enum
{
	SPAWN_MODE_NONE = 0,
	SPAWN_MODE_RANDOM,		// Seleccionar un Spawn al azar
	SPAWN_MODE_UNIQUE		// Acomodar cada jugador en un Spawn
};

//==============================================
// Tipos de daño
//==============================================

enum { };

//==============================================
// Armas
//==============================================

enum
{
	WEAPON_NONE = 0,

	WEAPON_AR15,
	WEAPON_BIZON,
	WEAPON_M4,
	WEAPON_MP5,
	WEAPON_MP5K,
	WEAPON_MRC,
	WEAPON_AK47,

	IN_WEAPON_MAX
};

static const char *sWeaponAlias[] =
{
	"none",

	"ar15",
	"bizon",
	"m4",
	"mp5",
	"mp5k",
	"mrc",
	"ak47",

	NULL
};

//==============================================
// Animaciones
//==============================================

#define ANIM_MOVE_X		"move_x"
#define ANIM_MOVE_Y		"move_y"

#define ANIM_AIM_PITCH	"body_pitch"
#define ANIM_AIM_YAW	"body_yaw"

enum
{
	IN_PLAYERANIMEVENT_NONE = PLAYERANIMEVENT_COUNT,

	PLAYERANIMEVENT_CANCEL_RELOAD,
	PLAYERANIMEVENT_NOCLIP,

	IN_PLAYERANIMEVENT_COUNT
};

//=========================================================
// Tabla de traducción de animaciones.
//=========================================================

/*static acttable_t unarmedActtable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE,					false },	// Sin hacer nada
	{ ACT_MP_WALK,						ACT_HL2MP_WALK,					false },	// Caminando
	{ ACT_MP_RUN,						ACT_HL2MP_RUN,					false },	// Corriendo
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH,			false },	// Agachado
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH,			false },	// Agachado y caminando
	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_DUEL,			false },	// Saltando
	{ ACT_MP_JUMP_START,				ACT_HL2MP_JUMP_DUEL,			false },	// Saltando
	{ ACT_MP_SWIM,						ACT_HL2MP_SWIM,					false },	// Nadando
	{ ACT_MP_SWIM_IDLE,					ACT_HL2MP_SWIM_IDLE,			false },	// Nadando sin hacer nada.
	{ ACT_MP_AIRWALK,					ACT_HL2MP_SWIM_IDLE,			false },	// Flotando (NOCLIP)
};*/


#endif IN_SHAREDDEFS_H