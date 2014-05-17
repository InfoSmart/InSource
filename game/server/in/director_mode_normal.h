//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef DIRECTOR_MODE_NORMAL_H
#define DIRECTOR_MODE_NORMAL_H

#include "director.h"
#include "director_spawn.h"

//=========================================================
// >> CDR_Normal_GM
//
// Código que establece lo que hará el Director en el Gameplay
// Normal
//=========================================================
class CDR_Normal_GM : public CDR_Gamemode
{
public:
	CDR_Normal_GM();

	virtual void Work();
	virtual void Check();

	virtual void StartRelax();

	virtual void OnSetStatus( DirectorStatus iStatus ) { }
	virtual void OnSetPhase( DirectorPhase iPhase, int iValue = 0 );

	virtual void OnSpawnChild( CBaseEntity *pEntity );

protected:
	CountdownTimer m_pCorpseBuild;
	CountdownTimer m_nCruelKill;
};

#endif // DIRECTOR_MODE_NORMAL_H