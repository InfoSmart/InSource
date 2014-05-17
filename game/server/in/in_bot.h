//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_BOT_H
#define IN_BOT_H

#pragma once

#ifdef APOCALYPSE
	#include "ap_player.h"
	#define BOT_CLASS CAP_Player
#else
	#include "in_player.h"
	#define BOT_CLASS CIN_Player
#endif

enum
{
	AIM_SPEED_VERYLOW = 0,
	AIM_SPEED_LOW,
	AIM_SPEED_NORMAL,
	AIM_SPEED_FAST,
	AIM_SPEED_VERYFAST,

	LAST_AIM_SPEED
};

//====================================================================
// CIN_Bot
//
// >> Un BOT (Dah)
//====================================================================
class CIN_Bot : public BOT_CLASS
{
public:
	DECLARE_CLASS( CIN_Bot, BOT_CLASS );

	static CBasePlayer *CreatePlayer( const char *pClassName, edict_t *pEdict, const char *pPlayerName );

	// Principales
	virtual void Spawn();
	virtual void RunPlayerMove( CUserCmd &cmd, float frametime );

	virtual void BotThink();
	virtual bool BotThinkMimic( int iPlayer );

	// Utilidades
	virtual void GetAimCenter( CBaseEntity *pEntity, Vector &vecAim );

	virtual bool HitsWall( float flDistance = 10 , int iHeight = 36 );
	virtual bool CanFireMyWeapon();

	virtual void AimTo( CBaseEntity *pEntity );
	virtual void AimTo( Vector &vecAim );
	virtual void AimTo( QAngle &angAim );

	virtual void RestoreAim();
	virtual float GetEndAimTime();

	// Inteligencia Artificial
	virtual void UpdateAim( CUserCmd &cmd );
	virtual bool UpdateMovement( CUserCmd &cmd );
	virtual void UpdateIdle( CUserCmd &cmd ) { }
	virtual void UpdateActions( CUserCmd &cmd );
	virtual void UpdateDirection( CUserCmd &cmd );
	virtual void UpdateJump( CUserCmd &cmd );
	virtual void UpdateWeapon( CUserCmd &cmd );

	virtual void UpdatePlayerActions( CUserCmd &cmd );

	virtual void UpdateAttack( CUserCmd &cmd );
	virtual void UpdateEnemy();

	virtual void CheckFlashlight();

	// Enemigos
	virtual bool CheckEnemy( CBaseEntity *pEntity, CBaseEntity *pCandidate );
	virtual CBaseEntity *FindEnemy( bool bCheckVisible = true );
	virtual bool CanBeEnemy( CBaseEntity *pEntity, bool bCheckVisible = true );
	virtual bool CanForgotEnemy();
	virtual bool EnemyIsLost();

	virtual float GetEnemyDistance();

	// Salud, muerte y daño.
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	// Enemigos
	virtual CBaseEntity	*GetEnemy() { return m_nEnemy.Get(); }
	virtual CBaseEntity	*GetEnemy() const { return m_nEnemy.Get(); }

	virtual void SetEnemy( CBaseEntity *pEnemy );

protected:
	float m_flNextDirectionAim;
	float m_flNextIdleCycle;

	QAngle m_angAimTo;
	Vector m_vecLastEnemyAim;
	EHANDLE m_nAimingEntity;
	Vector m_vecLastEntityPosition;

	bool m_bNeedAim;
	float m_flEndAimTime;

	EHANDLE m_nEnemy;
	CountdownTimer m_nEnemyLostMemory;
	float m_flNextEnemyCheck;
	bool m_nEnemyAimed;

	bool m_nDontRandomAim;

	CBasePlayer *m_nNearPlayer;
	QAngle m_angLastViewAngles;
};

inline CIN_Bot *ToInBot( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	CBasePlayer *pPlayer = ToBasePlayer( pEntity );

	if ( !pPlayer || !pPlayer->IsFakeClient() )
		return NULL;

	return dynamic_cast<CIN_Bot *>( pEntity );
}

#endif