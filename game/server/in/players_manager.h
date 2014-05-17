//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef PLAYERS_MANAGER_H
#define PLAYERS_MANAGER_H

#pragma once

#include "ai_navtype.h"

class CIN_Player;

//=========================================================
// >> PlayersManager
//=========================================================
class PlayersManager : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS( PlayersManager, CAutoGameSystemPerFrame );

	PlayersManager();

	// Principales
	virtual bool Init();
	virtual void Shutdown();

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void FrameUpdatePreEntityThink();
	virtual void FrameUpdatePostEntityThink();

	virtual int DefaultTeam();
	virtual void ExecCommand( const char *pName );

	// Devolución
	CIN_Player *Get( int iIndex );
	CIN_Player *GetLocal();

	CIN_Player *GetRandom( int iTeam = TEAM_ANY );
	CIN_Player *GetNear( const Vector &vPosition, float &fDistance, int iTeam = TEAM_ANY, CBasePlayer *pExcept = NULL );
	CIN_Player *GetNear( const Vector &vPosition, int iTeam = TEAM_ANY );

	// Rutas
	virtual bool HasRoute( CIN_Player *pPlayer, CAI_BaseNPC *pNPC, int iTolerance = 100, Navigation_t pNavType = NAV_GROUND );
	virtual bool HasRouteToAny( CAI_BaseNPC *pNPC, int iTolerance = 100, Navigation_t pNavType = NAV_GROUND );

	// Utilidades
	virtual void RespawnAll();
	virtual bool AreLive();
	virtual void StopMusic();

	virtual bool InLastKnowAreas( CNavArea *pArea );

	// Visibilidad
	virtual bool IsVisible( const Vector &vPosition );
	virtual bool IsVisible( CBaseEntity *pEntity );

	virtual bool InViewcone( const Vector &vPosition );
	virtual bool InViewcone( CBaseEntity *pEntity );

	virtual bool InVisibleCone( const Vector &vPosition );
	virtual bool InVisibleCone( CBaseEntity *pEntity );

	// Lecciones
	void SendLesson( const char *pLesson, CIN_Player *pPlayer, bool bOnce = false, CBaseEntity *pSubject = NULL );
	void SendAllLesson( const char *pLesson, bool bOnce = false, CBaseEntity *pSubject = NULL );

	// Recursos
	virtual int GetHealth();

	virtual int GetTotal() { return m_iTotal; }
	virtual int GetWithLife() { return m_iWithLife; }
	virtual int GetConnected() { return m_iConnected; }
	virtual int GetAllHealth() { return m_iHealth; }
	virtual float GetAllSanity() { return m_flSanity; }
	virtual int GetAllDejected() { return m_iDejected; }
	virtual bool InCombat() { return m_bInCombat; }
	virtual int GetWithWeapons() { return m_iWithWeapons; }
	virtual int GetAmmoStatus() { return m_iAmmoStatus; }
	virtual int GetStatus() { return m_iStatus; }

	virtual void Scan( int iTeam = TEAM_ANY );
	virtual void Decide();

protected:
	int m_iTotal;
	int m_iWithLife;
	int m_iConnected;

	int m_iHealth;
	float m_flSanity;
	int m_iDejected;

	bool m_bInCombat;
	int m_iWithWeapons;
	int m_iAmmoStatus;

	int m_iStatus;
	int m_iNextScan;

	friend class CDirector;
};

extern PlayersManager *PlysManager;

#endif // PLAYERS_MANAGER_H