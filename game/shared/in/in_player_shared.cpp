//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_shareddefs.h"

#include "playerclass_info_parse.h"
#include "weapon_inbase.h"

#include "ai_activity.h"
#include "activitylist.h"

#ifdef CLIENT_DLL
	#include "c_in_player.h"

	#include "dlight.h"
	#include "iefx.h"

	#include "flashlighteffect.h"
#else
	#include "in_player.h"
	#include "ai_basenpc.h"

#ifdef APOCALYPSE
	#include "ap_player.h"
	#include "ap_player_infected.h"
#endif

#endif // CLIENT_DLL

#ifdef CLIENT_DLL
static ConVar muzzleflash_dynamic_brigh( "cl_muzzleflash_dynamic_bright", "15.0", FCVAR_ARCHIVE, "" );
static ConVar muzzleflash_dynamic_enabled( "cl_muzzleflash_dynamic_enabled", "1", FCVAR_ARCHIVE, "" );
#endif

//=========================================================
// Devuelve el arma activa "CBaseInWeapon"
//=========================================================
CBaseInWeapon *CIN_Player::GetActiveInWeapon() const
{
	return static_cast<CBaseInWeapon *>( GetActiveWeapon() );
}

//=========================================================
// [Evento] El jugador ha disparado
//=========================================================
void CIN_Player::OnFireBullets( int iBullets )
{
#ifdef CLIENT_DLL
	// Encendemos un brillo
	dlight_t *dl;
	dl				= effects->CL_AllocDlight( entindex() );
	dl->origin		= GetAbsOrigin();
	dl->color.r		= 235;
	dl->color.g		= 142;
	dl->color.b		= 65;
	dl->radius		= RandomFloat( 500, 800 );
	dl->die			= gpGlobals->curtime + 0.02f;

	// Sin luz dinamica
	if ( !muzzleflash_dynamic_enabled.GetBool() )
		return;

	// Creamos una linterna que tenga el efecto de un MuzzleFlash
	if ( !m_hFireEffect )
	{
		m_hFireEffect = new CFlashlightEffect( index );
		m_hFireEffect->SetMuzzleFlashEnabled( true );
		m_hFireEffect->Init();

		m_hFireEffect->SetFar( 1300.0f );
		m_hFireEffect->SetAlpha( 1.0f );
	}

	// Encendemos la luz dinamica
	if ( m_hFireEffect )
	{
		// Brillo para simular un disparo
		m_hFireEffect->SetBright( muzzleflash_dynamic_brigh.GetFloat() );

		// La encendemos
		m_hFireEffect->TurnOff();
		m_hFireEffect->TurnOn();
		m_hFireEffect->SetDie( gpGlobals->curtime + 0.1f );

		// Ubicación, origen y destino de la luz
		Vector vecForward, vecRight, vecUp;
		Vector vecPosition	= EyePosition();
		QAngle eyeAngles	= EyeAngles();
		EyeVectors( &vecForward, &vecRight, &vecUp );

		// Actualizamos la posición
		m_hFireEffect->Update( vecPosition, vecForward, vecRight, vecUp );
	}

#else
	// ¡Estamos en combate!
	m_nFinishCombat.Start( 5 );

	// Estres
	RemoveSanity( 0.3f );

	// Retrasamos un poco el sonido de sufrimiento
	m_flNextDyingSound = gpGlobals->curtime + RandomFloat( 5.0f, 10.0f );
#endif
}

//=========================================================
// Traduce una animación a otra
//=========================================================
Activity CIN_Player::TranslateActivity( Activity actBase )
{
	CBaseInWeapon *pWeapon	= GetActiveInWeapon();
	Activity pActivity		= actBase;

	// Tenemos un arma, usar su animación
	if ( pWeapon )
	{
		pActivity = pWeapon->ActivityOverride( actBase, false );
		return pActivity;
	}

	//
	// Humanos/Supervivientes
	//
	if ( GetTeamNumber() == TEAM_HUMANS )
	{
		switch ( actBase )
		{
			// Sin hacer nada
			case ACT_MP_STAND_IDLE:
			{
				return ACT_IDLE_SMG;
				break;
			}

			// Caminar
			case ACT_MP_WALK:
			{
				return ACT_WALK_SMG;
				break;
			}

			// Correr
			case ACT_MP_RUN:
			{
				return ACT_RUN_SMG;
				break;
			}

			// Agacharse
			case ACT_MP_CROUCH_IDLE:
			{
				return ACT_CROUCHIDLE_SMG;
				break;
			}

			// Agacharse
			case ACT_MP_CROUCHWALK:
			{
				return ACT_CROUCHIDLE_SMG;
				break;
			}

			// Agacharse
			case ACT_MP_JUMP:
			{
				return ACT_JUMP_SMG;
				break;
			}

			// Agacharse
			case ACT_MP_SWIM:
			{
				return ACT_WALK_SMG;
				break;
			}

			// Agacharse
			case ACT_MP_SWIM_IDLE:
			{
				return ACT_IDLE_SMG;
				break;
			}

			// Estamos muriendo
			case ACT_DIESIMPLE:
			{
				return ACT_DIE_STANDING;
				break;
			}
		}
	}

	//
	// Infectados
	//
	if ( GetTeamNumber() == TEAM_INFECTED )
	{
		switch ( actBase )
		{
			// Sin hacer nada
			case ACT_MP_STAND_IDLE:
			{
				return ACT_TERROR_IDLE_NEUTRAL;
				break;
			}

			// Caminar
			case ACT_MP_WALK:
			{
				return ACT_TERROR_WALK_INTENSE;
				break;
			}

			// Correr
			case ACT_MP_RUN:
			{
				return ACT_TERROR_RUN_INTENSE;
				break;
			}

			// Agacharse
			case ACT_MP_CROUCH_IDLE:
			{
				return ACT_TERROR_CROUCH_IDLE_NEUTRAL;
				break;
			}

			// Agacharse
			case ACT_MP_CROUCHWALK:
			{
				return ACT_TERROR_CROUCH_RUN_INTENSE;
				break;
			}

			// Agacharse
			case ACT_MP_JUMP:
			{
				return ACT_TERROR_JUMP;
				break;
			}

			// Estamos muriendo
			case ACT_DIESIMPLE:
			{
				return ACT_TERROR_DIE_FROM_STAND;
				break;
			}

			// Ataque
			case ACT_MP_ATTACK_STAND_PRIMARYFIRE:
			{
				return ACT_TERROR_ATTACK;
				break;
			}

			// Cayendo
			case ACT_FALL:
			{
				return ACT_TERROR_FALL;
				break;
			}
		}
	}

	return pActivity;
}

#ifdef APOCALYPSE

/*
Activity CAP_Player::TranslateActivity( Activity actBase )
{
	CBaseInWeapon *pWeapon	= GetActiveInWeapon();
	Activity pActivity		= actBase;

	// Tenemos un arma, usar su animación
	if ( pWeapon )
	{
		pActivity = pWeapon->ActivityOverride( actBase, false );
		return pActivity;
	}

	switch ( actBase )
	{
		// Sin hacer nada
		case ACT_MP_STAND_IDLE:
		{
			if ( IsDejected() )
				return ACT_IDLE_INCAP_PISTOL;

			return ACT_IDLE_SMG;
			break;
		}

		// Caminar
		case ACT_MP_WALK:
		{
			return ACT_WALK_SMG;
			break;
		}

		// Correr
		case ACT_MP_RUN:
		{
			return ACT_RUN_SMG;
			break;
		}

		// Agacharse
		case ACT_MP_CROUCH_IDLE:
		{
			return ACT_CROUCHIDLE_SMG;
			break;
		}

		// Agacharse
		case ACT_MP_CROUCHWALK:
		{
			return ACT_CROUCHIDLE_SMG;
			break;
		}

		// Agacharse
		case ACT_MP_JUMP:
		{
			return ACT_JUMP_SMG;
			break;
		}

		// Agacharse
		case ACT_MP_SWIM:
		{
			return ACT_WALK_SMG;
			break;
		}

		// Agacharse
		case ACT_MP_SWIM_IDLE:
		{
			return ACT_IDLE_SMG;
			break;
		}

		// Estamos muriendo
		case ACT_DIESIMPLE:
		{
			return ACT_DIE_STANDING;
			break;
		}
	}

	return pActivity;
}

Activity CAP_PlayerInfected::TranslateActivity( Activity actBase )
{
	Activity pActivity	= actBase;

	switch ( actBase )
	{
		// Sin hacer nada
		case ACT_MP_STAND_IDLE:
		{
			return ACT_TERROR_IDLE_NEUTRAL;
			break;
		}

		// Caminar
		case ACT_MP_WALK:
		{
			return ACT_TERROR_WALK_INTENSE;
			break;
		}

		// Correr
		case ACT_MP_RUN:
		{
			return ACT_TERROR_RUN_INTENSE;
			break;
		}

		// Agacharse
		case ACT_MP_CROUCH_IDLE:
		{
			return ACT_TERROR_IDLE_NEUTRAL;
			break;
		}

		// Agacharse
		case ACT_MP_CROUCHWALK:
		{
			return ACT_TERROR_IDLE_NEUTRAL;
			break;
		}

		// Agacharse
		case ACT_MP_JUMP:
		{
			return ACT_TERROR_JUMP;
			break;
		}

		// Estamos muriendo
		case ACT_DIESIMPLE:
		{
			return ACT_TERROR_DIE_FROM_STAND;
			break;
		}
	}
}
*/

#endif // APOCALYPSE