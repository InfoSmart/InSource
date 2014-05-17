//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef IN_WEAPON_PARSE_H
#define IN_WEAPON_PARSE_H

#pragma once

#include "weapon_parse.h"
#include "networkvar.h"

//==============================================
// >> CINWeaponInfo
//==============================================
class CInWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CInWeaponInfo, FileWeaponInfo_t );

	CInWeaponInfo();

	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

public:
	int m_iDamage;
	float m_flSpread;
	float m_flSpeedWeight;

	float m_flMaxRange;
	float m_flFireRate;
	float m_flSecondaryFireRate;

	float m_flViewKick;
	int m_iClassification;

	Vector m_vecIronsightPos;
	QAngle m_angIronsightAng;
	float m_flIronsightFOV;
};

#endif // IN_WEAPON_PARSE_H