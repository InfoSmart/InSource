//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "iclientvehicle.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include "ihudlcd.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// >> CHudAmmo
// Muestra información de la munición del arma actual
//=========================================================
class CHudAmmo : public CHudNumericDisplay, public CHudElement
{
public:
	DECLARE_CLASS_SIMPLE( CHudAmmo, CHudNumericDisplay );

	CHudAmmo( const char *pElementName );

	virtual void Init();
	virtual void Reset();

	virtual void SetAmmo( int iAmmo, bool playAnimation );
	virtual void SetAmmo2( int iAmmo2, bool playAnimation );

	virtual void OnThink();

	virtual void UpdateAmmoDisplays();
	virtual void UpdatePlayerAmmo( C_BasePlayer *player );

protected:
	CHandle< C_BaseCombatWeapon > m_hCurrentActiveWeapon;
	CHandle< C_BaseEntity > m_hCurrentVehicle;

	int m_iAmmo;
	int m_iAmmo2;
};

DECLARE_HUDELEMENT( CHudAmmo );

//=========================================================
// Constructor
//=========================================================
CHudAmmo::CHudAmmo( const char *pElementName ) : BaseClass( NULL, "HudAmmo" ), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION );

	// Establecemos información predeterminada
	hudlcd->SetGlobalStat( "(ammo_primary)", "0" );
	hudlcd->SetGlobalStat( "(ammo_secondary)", "0" );
	hudlcd->SetGlobalStat( "(weapon_print_name)", "" );
	hudlcd->SetGlobalStat( "(weapon_name)", "" );
}

//=========================================================
// Inicia el elemento
//=========================================================
void CHudAmmo::Init()
{
	m_iAmmo		= -1;
	m_iAmmo2	= -1;

	wchar_t *tmpString = g_pVGuiLocalize->Find("#Valve_Hud_AMMO");

	// Munición
	if ( tmpString )
	{
		SetLabelText( tmpString );
	}
	else
	{
		SetLabelText(L"AMMO");
	}
}

//=========================================================
// Reinicia el HUD después de guardar/restaurar una partida
//=========================================================
void CHudAmmo::Reset()
{
	BaseClass::Reset();

	m_hCurrentActiveWeapon	= NULL;
	m_hCurrentVehicle		= NULL;

	m_iAmmo		= 0;
	m_iAmmo2	= 0;

	UpdateAmmoDisplays();
}


//=========================================================
// Actualiza la cantidad de munición del Jugador
//=========================================================
void CHudAmmo::UpdatePlayerAmmo( C_BasePlayer *pPlayer )
{
	// Clear out the vehicle entity
	m_hCurrentVehicle = NULL;

	// Obtenemos el arma actual del Jugador
	C_BaseCombatWeapon *pWeapon = ( pPlayer ) ? pPlayer->GetActiveWeapon() : NULL;
	
	// No hay un Jugador, no tiene un arma o esta no usa munición primaria
	if ( !pPlayer || !pWeapon || !pWeapon->UsesPrimaryAmmo() )
	{
		hudlcd->SetGlobalStat( "(ammo_primary)", "n/a" );
		hudlcd->SetGlobalStat( "(ammo_secondary)", "n/a" );

		// Ocultamos el elemento
		SetPaintEnabled( false );
		SetPaintBackgroundEnabled( false );

		return;
	}

	// Nombre del arma
	hudlcd->SetGlobalStat( "(weapon_print_name)", pWeapon->GetPrintName() );
	hudlcd->SetGlobalStat( "(weapon_name)", pWeapon->GetName() );

	// Visible
	SetPaintEnabled( true );
	SetPaintBackgroundEnabled( true );

	// Munición del arma
	int ammo1 = pWeapon->Clip1();
	int ammo2 = 0;

	if ( ammo1 < 0 )
	{
		// we don't use clip ammo, just use the total ammo count
		ammo1 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
		ammo2 = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		ammo2 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
	}

	hudlcd->SetGlobalStat( "(ammo_primary)", VarArgs( "%d", ammo1 ) );
	hudlcd->SetGlobalStat( "(ammo_secondary)", VarArgs( "%d", ammo2 ) );

	// Es la misma arma que antes, solo actualizamos el contador
	if ( pWeapon == m_hCurrentActiveWeapon )
	{
		SetAmmo( ammo1, true );
		SetAmmo2( ammo2, true );
	}

	// Es una nueva arma, cambiamos sin animación
	else
	{
		SetAmmo( ammo1, false );
		SetAmmo2( ammo2, false );

		if ( pWeapon->UsesClipsForAmmo1() )
		{
			SetShouldDisplaySecondaryValue( true );
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClips");
		}
		else
		{
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseClips");
			SetShouldDisplaySecondaryValue( false );
		}

		GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("WeaponChanged");
		m_hCurrentActiveWeapon = pWeapon;
	}
}

/*
void CHudAmmo::UpdateVehicleAmmo( C_BasePlayer *player, IClientVehicle *pVehicle )
{
	m_hCurrentActiveWeapon = NULL;
	CBaseEntity *pVehicleEnt = pVehicle->GetVehicleEnt();

	if ( !pVehicleEnt || pVehicle->GetPrimaryAmmoType() < 0 )
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);

	// get the ammo in our clip
	int ammo1 = pVehicle->GetPrimaryAmmoClip();
	int ammo2;
	if (ammo1 < 0)
	{
		// we don't use clip ammo, just use the total ammo count
		ammo1 = pVehicle->GetPrimaryAmmoCount();
		ammo2 = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		ammo2 = pVehicle->GetPrimaryAmmoCount();
	}

	if (pVehicleEnt == m_hCurrentVehicle)
	{
		// same weapon, just update counts
		SetAmmo(ammo1, true);
		SetAmmo2(ammo2, true);
	}
	else
	{
		// diferent weapon, change without triggering
		SetAmmo(ammo1, false);
		SetAmmo2(ammo2, false);

		// update whether or not we show the total ammo display
		if (pVehicle->PrimaryAmmoUsesClips())
		{
			SetShouldDisplaySecondaryValue(true);
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClips");
		}
		else
		{
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseClips");
			SetShouldDisplaySecondaryValue(false);
		}

		GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("WeaponChanged");
		m_hCurrentVehicle = pVehicleEnt;
	}
}
*/

//=========================================================
// Pensamiento: Bucle de ejecución de tareas
//=========================================================
void CHudAmmo::OnThink()
{
	UpdateAmmoDisplays();
}

//=========================================================
// Actualiza el contador con la información más reciente
//=========================================================
void CHudAmmo::UpdateAmmoDisplays()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	UpdatePlayerAmmo( pPlayer );
}

//=========================================================
// Actualiza el contador de munición primaria
//=========================================================
void CHudAmmo::SetAmmo( int iAmmo, bool playAnimation )
{
	// Ha cambiado
	if ( iAmmo != m_iAmmo )
	{
		// Sin munición
		if ( iAmmo <= 0 )
		{
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("AmmoEmpty");
		}

		// Hemos gastado munición
		else if ( iAmmo < m_iAmmo )
		{
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("AmmoDecreased");
		}

		// Hemos obtenido munición
		else
		{
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("AmmoIncreased");
		}

		m_iAmmo = iAmmo;
	}

	SetDisplayValue( iAmmo );
}

//=========================================================
// Actualiza el contador de munición total
//=========================================================
void CHudAmmo::SetAmmo2( int iAmmo, bool playAnimation )
{
	// Ha cambiado
	if ( iAmmo != m_iAmmo2 )
	{
		// Sin munición
		if ( iAmmo <= 0 )
		{
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("Ammo2Empty");
		}

		// Hemos gastado munición
		else if ( iAmmo < m_iAmmo )
		{
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("Ammo2Decreased");
		}

		// Hemos obtenido munición
		else
		{
			GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("Ammo2Increased");
		}

		m_iAmmo2 = iAmmo;
	}

	SetSecondaryValue( iAmmo );
}

//-----------------------------------------------------------------------------
// Purpose: Displays the secondary ammunition level
//-----------------------------------------------------------------------------
/*
class CHudSecondaryAmmo : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudSecondaryAmmo, CHudNumericDisplay );

public:
	CHudSecondaryAmmo( const char *pElementName ) : BaseClass( NULL, "HudAmmoSecondary" ), CHudElement( pElementName )
	{
		m_iAmmo = -1;

		SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_WEAPONSELECTION | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
	}

	void Init( void )
	{
	}

	void VidInit( void )
	{
	}

	void SetAmmo( int ammo )
	{
		if (ammo != m_iAmmo)
		{
			if (ammo == 0)
			{
				GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryEmpty");
			}
			else if (ammo < m_iAmmo)
			{
				// ammo has decreased
				GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryDecreased");
			}
			else
			{
				// ammunition has increased
				GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryIncreased");
			}

			m_iAmmo = ammo;
		}
		SetDisplayValue( ammo );
	}

	void Reset()
	{
		// hud reset, update ammo state
		BaseClass::Reset();
		m_iAmmo = 0;
		m_hCurrentActiveWeapon = NULL;
		SetAlpha( 0 );
		UpdateAmmoState();
	}

protected:
	virtual void OnThink()
	{
		// set whether or not the panel draws based on if we have a weapon that supports secondary ammo
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		C_BaseCombatWeapon *wpn = player ? player->GetActiveWeapon() : NULL;
		IClientVehicle *pVehicle = player ? player->GetVehicle() : NULL;
		if (!wpn || !player || pVehicle)
		{
			m_hCurrentActiveWeapon = NULL;
			SetPaintEnabled(false);
			SetPaintBackgroundEnabled(false);
			return;
		}
		else
		{
			SetPaintEnabled(true);
			SetPaintBackgroundEnabled(true);
		}

		UpdateAmmoState();
	}

	void UpdateAmmoState()
	{
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		C_BaseCombatWeapon *wpn = player ? player->GetActiveWeapon() : NULL;

		if (player && wpn && wpn->UsesSecondaryAmmo())
		{
			SetAmmo(player->GetAmmoCount(wpn->GetSecondaryAmmoType()));
		}

		if ( m_hCurrentActiveWeapon != wpn )
		{
			if ( wpn->UsesSecondaryAmmo() )
			{
				// we've changed to a weapon that uses secondary ammo
				GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesSecondaryAmmo");
			}
			else 
			{
				// we've changed away from a weapon that uses secondary ammo
				GetClientMode()->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseSecondaryAmmo");
			}
			m_hCurrentActiveWeapon = wpn;
		}
	}

private:
	CHandle< C_BaseCombatWeapon > m_hCurrentActiveWeapon;
	int		m_iAmmo;
};

DECLARE_HUDELEMENT( CHudSecondaryAmmo );
*/