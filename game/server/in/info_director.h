//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef INFO_DIRECTOR_H
#define INFO_DIRECTOR_H

#pragma once

//=========================================================
// >> CInfoDirector
//=========================================================
class CInfoDirector : public CLogicalEntity
{
public:
	DECLARE_CLASS( CInfoDirector, CLogicalEntity );
	DECLARE_DATADESC();

	virtual void InputHowAngry( inputdata_t &inputdata );
	virtual void InputWhatsYourStatus( inputdata_t &inputdata );

	virtual void InputHowAliveChilds( inputdata_t &inputdata );
	virtual void InputHowAliveSpecials( inputdata_t &inputdata );
	virtual void InputHowAliveBoss( inputdata_t &inputdata );

	virtual void InputForceRelax( inputdata_t &inputdata );
	virtual void InputForcePanic( inputdata_t &inputdata );
	virtual void InputForceInfinitePanic( inputdata_t &inputdata );
	virtual void InputForceBoss( inputdata_t &inputdata );
	virtual void InputForceClimax( inputdata_t &inputdata );

	virtual void InputSetPopulation( inputdata_t &inputdata );
	virtual void InputDisclosePlayers( inputdata_t &inputdata );

	virtual void InputKillChilds( inputdata_t &inputdata );
	virtual void InputKillNoVisibleChilds( inputdata_t &inputdata );

	virtual void InputSetSpawnInForceAreas( inputdata_t &inputdata );
	virtual void InputSetSpawnInNormalAreas( inputdata_t &inputdata );

	virtual void InputStartSurvivalTime( inputdata_t &inputdata );

	virtual void InputDisableMusic( inputdata_t &inputdata );
	virtual void InputEnableMusic( inputdata_t &inputdata );

	virtual void InputSetMaxAliveBoss( inputdata_t &inputdata );

protected:
	COutputInt OnResponseAngry;
	COutputInt OnResponseStatus;

	COutputInt OnResponseAliveChilds;
	COutputInt OnResponseAliveSpecials;
	COutputInt OnResponseAliveBoss;

	COutputEvent OnSpawnChild;
	COutputEvent OnSpawnBoss;
	COutputEvent OnSpawnSpecial;
	COutputEvent OnSpawnAmbient;
	COutputEvent OnSleep;
	COutputEvent OnRelaxed;
	COutputEvent OnPanic;
	COutputEvent OnBoss;
	COutputEvent OnClimax;

	int m_iMinAngryRange;
	int m_iMaxAngryRange;

	friend class CDirector;
};

#endif //INFO_DIRECTOR_H