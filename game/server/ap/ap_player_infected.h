//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_PLAYER_INFECTED
#define AP_PLAYER_INFECTED

#include "ap_player.h"
#include "behavior_melee_character.h"

//==============================================
// >> CAP_PlayerInfected
//==============================================
class CAP_PlayerInfected : public CAP_Player, public CB_MeleeCharacter
{
public:
	DECLARE_CLASS( CAP_PlayerInfected, CAP_Player );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	static CAP_PlayerInfected *CreateInfectedPlayer( const char *pClassName, edict_t *e );

	virtual int MyDefaultTeam() 
	{ 
		return TEAM_INFECTED; 
	}

	virtual Class_T Classify()
	{
		return CLASS_INFECTED;
	}

	// Principales
	virtual void Spawn();
	virtual void RequestDirectorSpawn();

	virtual void SetClothColor();

	virtual void PostThink();

	virtual CBaseEntity *EntSelectSpawnPoint() { return NULL; }

	// Sonidos
	virtual void PrepareMusic();
	virtual void StopMusic();

	virtual const char *GetPlayerType();

	virtual void PainSound( const CTakeDamageInfo &info );
	virtual void DyingSound();
	virtual void DieSound();

	virtual void UpdateSounds();
	virtual void UpdateFallEffects() { }

	// Animaciones
	//virtual Activity TranslateActivity( Activity actBase ); // SHARED!
	virtual void HandleAnimEvent( animevent_t *pEvent );

	// Linterna
	virtual bool FlashlightTurnOn( bool playSound = true ) { return false; }
	virtual void FlashlightTurnOff( bool playSound = true ) { }
	virtual int FlashlightIsOn() { return false; }

	// Salud, muerte y Daño
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	virtual void PlayerDeathPostThink();

	virtual bool IsDejected() { return false; }
	virtual void StartDejected() { }
	virtual void StopDejected() { }
	virtual void GetUp( CIN_Player *pPlayer ) { }

	virtual void UpdateSanity() { }
	virtual void RemoveSanity( float flValue ) { }
	virtual float GetSanity() { return 100; }

	virtual void UpdateStamina() { }
	virtual void UpdateHealthRegeneration() { }
	virtual void UpdateTiredEffects() { }

	// Ataque
	virtual int GetMeleeDamage();
	virtual int GetMeleeDistance();

	// Movimiento
	virtual void UpdateSpeed();

	virtual void AddLastKnowArea( CNavArea *pArea, bool bAround = true ) { }
	virtual bool InLastKnowAreas( CNavArea *pArea ) { return false; }

protected:
	int m_iNextIdleSound;
	int m_iNextAttack;
};

#endif // AP_PLAYER_INFECTED