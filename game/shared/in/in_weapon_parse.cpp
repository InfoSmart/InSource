//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include <KeyValues.h>
#include "in_weapon_parse.h"

//==============================================
//
//==============================================
FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CInWeaponInfo;
}

//==============================================
// Constructor
//==============================================
CInWeaponInfo::CInWeaponInfo()
{
	iDamage				= 5;
	flSpread			= 0.5f;
	flFireRate			= 0.15f;
	flSecondaryFireRate = 1.0f;
	flMaxRange			= 4000.0f;

	flViewKickDampen = 0.1;
	flViewKickVertical = 13.0;
	flViewKickSlideLimit = 5.0;
}

//==============================================
// Guarda información acerca del arma.
//==============================================
void CInWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	// Daño
	// Douglas Adams 1952 - 2001
	iDamage	= pKeyValuesData->GetInt("Damage", 42);

	// Disperción de balas
	flSpread = pKeyValuesData->GetFloat( "BulletSpread", 0.5f );

	// Peso del arma
	flSpeedWeight = pKeyValuesData->GetFloat( "SpeedWeight", 0.0f );

	// Tiempo entre disparos. (Primario)
	flFireRate = pKeyValuesData->GetFloat( "FireRate", 0.15f );

	// Tiempo entre disparos. (Secundario)
	flSecondaryFireRate = pKeyValuesData->GetFloat( "SecondaryFireRate", 1.0f );

	// Máxima distancia de disparo.
	flMaxRange = pKeyValuesData->GetFloat( "MaxRange", 4000.0f );

	//
	// Retroceso
	//
	KeyValues *pKick = pKeyValuesData->FindKey( "ViewKick" );

	if ( pKick )
	{
		flViewKickDampen		= pKick->GetFloat( "Dampen", 0.1 );
		flViewKickVertical		= pKick->GetFloat( "Vertical", 13.0 );
		flViewKickSlideLimit	= pKick->GetFloat( "Slide", 5.0 );
	}
	else
	{
		flViewKickDampen		= 0.1;
		flViewKickVertical		= 13.0;
		flViewKickSlideLimit	= 5.0;
	}

	//
	// Vista de acero
	//
	KeyValues *pSights = pKeyValuesData->FindKey( "IronSight" );

	if ( pSights )
	{
		m_vecIronsightPos.x = pSights->GetFloat( "Forward", 0.0f );
		m_vecIronsightPos.y = pSights->GetFloat( "Right", 0.0f );
		m_vecIronsightPos.z = pSights->GetFloat( "Up", 0.0f );

		m_angIronsightAng[PITCH]	= pSights->GetFloat( "Pitch", 0.0f );
		m_angIronsightAng[YAW]		= pSights->GetFloat( "Yaw", 0.0f );
		m_angIronsightAng[ROLL]		= pSights->GetFloat( "Roll", 0.0f );

		m_flIronsightFOV = pSights->GetFloat( "Fov", 0.0f );
	}
	else
	{
		m_vecIronsightPos = vec3_origin;
		m_angIronsightAng.Init();
		m_flIronsightFOV = 0.0f;
	}
}