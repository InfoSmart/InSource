//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_PLAYER_H
#define AP_PLAHER_H

#pragma once

#include "in_player.h"

//==============================================
//==============================================
static const char *m_nPlayersTypes[] =
{
	"Abigail",
	"Frank",
};

enum PlayerSound
{
	PL_AREA_CLEAR	= 7,
	PL_BACKUP		= 8,
	PL_HURRAH		= 10,
	PL_INCOMING		= 11,

	PL_LAST_SOUND,
};

static const char *m_nPlayersSounds[] =
{
	// Singleplayer
	"SP.Dying.Major",	
	"SP.Dying.Critical",
	"SP.Dying.Critical2",
	"SP.Help",

	// Multiplayer
	"MP.Dying.Major",
	"MP.Dying.Critical"
	"MP.Dying.Critical2",
	"MP.Help",

	"AreaClear",
	"Backup",
	"FriendlyFire",
	"Hurrah",
	"Incoming",

	// General
	"Dying.Minor",
	"Attacked.Horde",
	"Hurt.Minor",
	"Hurt.Major",
	"Hurt.Critical",
	"Hurt.Dejected",

	"Start.Dejected",
	"Dejected",
	"Fall",

	"Breath.Tired",
	"Breath.Panic"
};

static const char *m_nPlayerMusic[] =
{
	"Death",
	"Death.Tag",
	"Burning",
	"Burning.Tag",
	"Incap",
	"PreDeath",
	"PreDeath.Tag",

	"ClimbIncap.Firm",
	"ClimbIncap.Weak",
	"ClimbIncap.Dangle",
	"ClimbIncap.Fall",
};

static const char *m_nHumansPlayerModels[] =
{
	"models/survivors/survivor_ana.mdl",
	"models/survivors/survivor_frank.mdl",
};

static const char *m_nSoldiersPlayerModels[] =
{
	"models/player/cellassault1.mdl",
	"models/player/cellassault2.mdl",
	"models/player/cellassault3.mdl",
	"models/player/cellassault4.mdl",
};

static const char *m_nInfectedPlayerModels[] =
{
	"models/infected/common_male01.mdl",
	"models/infected/common_female01.mdl"
};

//==============================================
// >> CAP_Player
//==============================================
class CAP_Player : public CIN_Player
{
public:
	DECLARE_CLASS( CAP_Player, CIN_Player );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	static CAP_Player *CreateSurvivorPlayer( const char *pClassName, edict_t *pEdict, const char *pPlayerName );
	virtual void CreateAnimationState();

	virtual bool IsInfected() 
	{
		return ( MyDefaultTeam() == TEAM_INFECTED );
	}

	virtual bool IsSurvivor() 
	{
		return ( MyDefaultTeam() == TEAM_HUMANS );
	}

	// Principales
	virtual void Spawn();
	virtual void Precache();

	virtual void PrecacheMultiplayer();

	virtual void PostThink();

	// Modelos
	virtual const char *GetPlayerModelValidated();
	virtual bool IsValidModel( const char *pModel );

	// Animaciones
	//virtual Activity TranslateActivity( Activity actBase ); // SHARED!

	// Sonidos
	virtual void PrepareMusic();
	virtual void StopMusic();

	virtual void PainSound( const CTakeDamageInfo &info );
	virtual void DyingSound();
	//virtual void DieSound();

	virtual void UpdateSounds();

	// Salud, muerte y daño
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );

	virtual void PlayerDeathPostThink();

	virtual void UpdateLifeLevels();
	virtual void BleedEffects();

	virtual void UpdateFallEffects();

	// Incapacitación
	virtual void UpdateIncap();

	virtual void OnStartIncap();
	virtual void OnStopIncap();

	// Comandos y cheats
	virtual bool ClientCommand( const CCommand &args );

protected:
	float m_flBloodLevel;
	int m_iHungryLevel;
	int m_iThirstLevel;

	bool m_bBleed;

	int m_iNextLifeUpdate;
	int m_iNextBleedDamage;
	int m_iNextBleedEffect;

	// Sonidos
	CLayerSound *m_nIncapMusic;
	CLayerSound *m_nDeathMusic;
	CLayerSound *m_nPreDeathMusic;
	CLayerSound *m_nClimbingIncapMusic;

	// Sonidos que deben pasarse al FacePoser
	CLayerSound *m_nTired;
	CLayerSound *m_nFall;
};

#endif