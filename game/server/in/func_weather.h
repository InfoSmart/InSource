//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef FUNC_WEATHER_H
#define FUNC_WEATHER_H

#pragma once

class CWeather : public CBaseEntity
{
public:
	DECLARE_CLASS( CWeather, CBaseEntity );
	DECLARE_DATADESC();

	virtual void Spawn();
	virtual void Think();
};

#endif // FUNC_WEATHER_H