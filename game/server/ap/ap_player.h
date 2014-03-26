//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_PLAYER_H
#define AP_PLAHER_H

#pragma once

#include "in_player.h"

//==============================================
//==============================================
static const char *m_nPlayersTypes[] =
{
	"Player",
	"Male",
	"Female",
	"Adan",
	"Alyx",
	"Admin",
	"Burned",
	"Cristian",
	"Kleiner",
	"Magnusson",
	"Monk",
	"Mossman",
	"Odessa",
	"Police",
	"Soldier",
};

static const char *m_nPlayersSounds[] =
{
	"SP.Dying.Critical2",
	"SP.Dying.Major",
	"SP.Dying.Critical",

	"MP.Dying.Critical2",
	"MP.Dying.Major",
	"MP.Dying.Critical"
	"MP.Help"

	"Dying.Minor",
	"Attacked.Horde",
	"FriendlyFire",
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
	"Pain.Terror",
	"Burning",
	"Burning.Tag",
	"Dejected",
	"SanityTerror"
};

static const char *m_nHumansPlayerModels[] =
{
	"models/survivors/survivor_ana.mdl",
	"models/survivors/survivor_kat.mdl",
	"models/survivors/survivor_producer.mdl",
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
	DECLARE_DATADESC();

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
	EnvMusic *m_nLowStaminaSound;
	EnvMusic *m_nSanityTerror;
	EnvMusic *m_nDeathMusic;

	// Sonidos que deben pasarse al FacePoser
	EnvMusic *m_nTired;
	EnvMusic *m_nFall;
};

#endif