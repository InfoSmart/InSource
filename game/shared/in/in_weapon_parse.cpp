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
	m_iDamage				= 5;
	m_flSpread				= 0.5f;
	m_flFireRate			= 0.15f;
	m_flSecondaryFireRate	= 1.0f;
	m_flMaxRange			= 4000.0f;

	m_flViewKick = 0.1;
}

//==============================================
// Guarda informaci�n acerca del arma.
//==============================================
void CInWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	// Da�o
	// Douglas Adams 1952 - 2001
	m_iDamage	= pKeyValuesData->GetInt("Damage", 42);

	// Disperci�n de balas
	m_flSpread = pKeyValuesData->GetFloat( "BulletSpread", 0.5f );

	// Peso del arma
	m_flSpeedWeight = pKeyValuesData->GetFloat( "SpeedWeight", 0.0f );

	// Tiempo entre disparos. (Primario)
	m_flFireRate = pKeyValuesData->GetFloat( "FireRate", 0.15f );

	// Tiempo entre disparos. (Secundario)
	m_flSecondaryFireRate = pKeyValuesData->GetFloat( "SecondaryFireRate", 1.0f );

	// M�xima distancia de disparo.
	m_flMaxRange = pKeyValuesData->GetFloat( "MaxRange", 4000.0f );

	// Retroceso
	m_flViewKick = pKeyValuesData->GetFloat( "ViewKick", 3.0f );

	// Clasificaci�n
	m_iClassification = pKeyValuesData->GetInt( "WeaponClass", 0 );

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