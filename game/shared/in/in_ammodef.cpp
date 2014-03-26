//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "in_ammodef.h"
#include "in_gamerules.h"
#include "ammodef.h"

//=========================================================
// Comandos
//=========================================================

ConVar sk_max_556mm("sk_max_556mm", "99");
ConVar sk_max_68mm("sk_max_68mm", "99");
ConVar sk_max_9mm("sk_max_9mm", "99");
ConVar sk_max_10mm("sk_max_10mm", "99");

//=========================================================
// Configuración
//=========================================================

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

//=========================================================
// Munición
//=========================================================
CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;
		
		def.AddAmmoType("5,56mm",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,	0,	"sk_max_556mm",			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("6,8mm",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,	0,	"sk_max_68mm",			BULLET_IMPULSE(300, 1325),	0 );
		def.AddAmmoType("9mm",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,	0,	"sk_max_9mm",			BULLET_IMPULSE(400, 1425),	0 );
		def.AddAmmoType("10mm",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,	0,	"sk_max_10mm",			BULLET_IMPULSE(500, 1525),	0 );
	}

	return &def;
}