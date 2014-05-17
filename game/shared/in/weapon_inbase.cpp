//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "gamevars_shared.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "effect_dispatch_data.h"
#include "engine/ivdebugoverlay.h"

#include "in_gamerules.h"
#include "in_shareddefs.h"
#include "weapon_inbase.h"

#ifdef CLIENT_DLL

	#include "c_in_player.h"
	#include "c_te_effect_dispatch.h"
	#include "prediction.h"

#else

	#include "in_player.h"
	#include "ilagcompensationmanager.h"
	#include "te_effect_dispatch.h"

#endif

//====================================================================
// Comandos
//====================================================================

ConVar weapon_infiniteammo( "in_weapon_infiniteammo", "0", FCVAR_SERVER | FCVAR_CHEAT, "Activa la munición infinita" );
ConVar weapon_automatic_reload( "in_weapon_automatic_reload", "0", FCVAR_SERVER, "Activa la recarga del arma cuando se queda sin munición" );

ConVar weapon_autofire( "in_weapon_autofire", "0", FCVAR_SERVER, "Activa el disparo seguido al mantener el clic presionado" );
ConVar weapon_fire_spread( "in_weapon_fire_spread", "0.0" );

ConVar weapon_equip_touch( "in_weapon_equip_touch", "0", FCVAR_SERVER, "Indica si se pueden obtener las armas al tocarlas" );

// sk_plr_dmg_grenade
// NOTE: El nombre es usado por basegrenade_contact
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade", "10", FCVAR_SERVER, "Establece el daño producido por la granada" );

// Ironsight
ConVar viewmodel_adjust_forward( "viewmodel_adjust_forward", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_right( "viewmodel_adjust_right", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_up( "viewmodel_adjust_up", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_pitch( "viewmodel_adjust_pitch", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_yaw( "viewmodel_adjust_yaw", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_roll( "viewmodel_adjust_roll", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_fov( "viewmodel_adjust_fov", "0", FCVAR_REPLICATED, "Note: this feature is not available during any kind of zoom" );
ConVar viewmodel_adjust_enabled( "viewmodel_adjust_enabled", "0", FCVAR_REPLICATED|FCVAR_CHEAT, "enabled viewmodel adjusting" );

//====================================================================
// Proxy
//====================================================================
#ifdef CLIENT_DLL
void RecvProxy_ToggleSights( const CRecvProxyData* pData, void* pStruct, void* pOut )
{
	CBaseInWeapon *pWeapon = (CBaseInWeapon *)pStruct;

	if ( pData->m_Value.m_Int )
		pWeapon->EnableIronsight();
	else
		pWeapon->DisableIronsight();
}
#endif

//====================================================================
// Información y Red
//====================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( BaseInWeapon, DT_BaseInWeapon );

LINK_ENTITY_TO_CLASS( weapon_in_base, CBaseInWeapon );

BEGIN_NETWORK_TABLE( CBaseInWeapon, DT_BaseInWeapon )

#ifndef CLIENT_DLL
	SendPropBool( SENDINFO( m_bIsIronsighted ) ),
	SendPropFloat( SENDINFO( m_flIronsightedTime ) ),
#else
	RecvPropInt( RECVINFO( m_bIsIronsighted ), 0, RecvProxy_ToggleSights ),
	RecvPropFloat( RECVINFO( m_flIronsightedTime ) ),
#endif

END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CBaseInWeapon )
	DEFINE_PRED_FIELD( m_bIsIronsighted, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flIronsightedTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#else
BEGIN_DATADESC( CBaseInWeapon )
END_DATADESC()
#endif

//====================================================================
// Constructor
//====================================================================
CBaseInWeapon::CBaseInWeapon()
{
	// Con predicción.
	if ( gpGlobals->maxClients > 1 )
		SetPredictionEligible( true );

	// Podemos empezar a atacar
	bIsPrimaryAttackReleased = true;

	// Podemos empezar a atacar en 1s
	SetNextAttack( 1.0 );

	// Propiedades físicas
	AddSolidFlags( FSOLID_TRIGGER );

	// Vista de Hierro
	m_bIsIronsighted	= false;
	m_flIronsightedTime = 0.0f;
}

//====================================================================
//====================================================================
bool CBaseInWeapon::IsPredicted() const
{
	return ( gpGlobals->maxClients <= 1 ) ? false : true;
}

//====================================================================
// Devuelve el jugador dueño de esta arma.
//====================================================================
CIN_Player *CBaseInWeapon::GetPlayerOwner() const
{
	return ToInPlayer( GetOwner() );
}

//====================================================================
// Devuelve la información y ajustes del arma.
//====================================================================
const CInWeaponInfo &CBaseInWeapon::GetInWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CInWeaponInfo *pInfo = static_cast< const CInWeaponInfo*>( pWeaponInfo );

	return *pInfo;
}

//====================================================================
// Devuelve la posición de disparo.
//====================================================================
Vector CBaseInWeapon::ShootPosition()
{
	return EyePosition();
}

//====================================================================
// Guardado de objetos necesarios en caché
//====================================================================
void CBaseInWeapon::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "weapon_muzzle_smoke" );
}

//====================================================================
// Bucle de ejecución de tareas.
//====================================================================
void CBaseInWeapon::ItemPostFrame()
{
	CIN_Player *pPlayer = ToInPlayer( GetOwner() );

	// Solo los jugadores pueden usar esto
	if ( !pPlayer )
		return;

	// Duración que mantenemos el clic presionado.
	m_fFireDuration = (pPlayer->IsPressingButton( IN_ATTACK )) ? (m_fFireDuration + gpGlobals->frametime) : 0.0f;

	// Checar si hemos terminado de recargar.
	if ( UsesClipsForAmmo1() )
		CheckReload();

	// Disparo secundario
	if ( pPlayer->IsPressingButton( IN_ATTACK2 ) && CanSecondaryAttack() )
	{
		SecondaryAttack();
	}

	// Disparo primario
	if ( pPlayer->IsPressingButton( IN_ATTACK ) )
	{
		if ( CanPrimaryAttack() )
		{
			if ( pPlayer->IsButtonPressed( IN_ATTACK ) && pPlayer->IsButtonReleased( IN_ATTACK2 ) )
				SetNextPrimaryAttack( );

			PrimaryAttack();

			// Estamos presionando el botón de ataque.
			bIsPrimaryAttackReleased = false;
		}
		else
		{
			m_fFireDuration = 0.0f;
		}
	}
	else
	{
		// Hemos soltado el botón de ataque.
		bIsPrimaryAttackReleased = true;
	}

	// Recarga
	if ( pPlayer->IsPressingButton( IN_RELOAD ) && UsesClipsForAmmo1() && !InReload() )
	{
		Reload( );
		m_fFireDuration = 0.0f;
	}

	// No estamos atacando ni recargando.
	if ( !(pPlayer->IsPressingButton( IN_RELOAD )) && !pPlayer->IsPressingButton( IN_ATTACK ) && !pPlayer->IsPressingButton( IN_ATTACK2 ) )
	{
		// No estamos recargando.
		if ( /*!ReloadOrSwitchWeapons() &&*/ !InReload( ) )
		{
			// Animación de no estar haciendo nada.
			WeaponIdle();
		}
	}
}

//====================================================================
//====================================================================
bool CBaseInWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	DisableIronsight();

	return BaseClass::Holster( pSwitchingTo );
}

//====================================================================
//====================================================================
void CBaseInWeapon::Drop( const Vector &vecVelocity )
{
	DisableIronsight();

	BaseClass::Drop( vecVelocity );
}

//====================================================================
//====================================================================
void CBaseInWeapon::DefaultTouch( CBaseEntity *pOther )
{
	// No podemos obtener armas solo por tocarlas
	if ( !weapon_equip_touch.GetBool() )
		return;

	BaseClass::DefaultTouch( pOther );
}

//====================================================================
// Devuelve si el arma puede efectuar un disparo primario.
// ¡Solo llamar desde PrimaryAttack()!
//====================================================================
bool CBaseInWeapon::CanPrimaryAttack()
{
	CIN_Player *pPlayer = GetPlayerOwner();

	// Jugador inválido.
	if ( !pPlayer )
	{
		Warning("[CBaseInWeapon::CanPrimaryAttack] Mi propietario no es un jugador. \n");
		return false;
	}

	// Aún no
	if ( m_flNextPrimaryAttack >= gpGlobals->curtime )
		return false;

	// Debes hacer cada disparo manual.
	if ( !weapon_autofire.GetBool() && bIsPrimaryAttackReleased == false )
		return false;

	// Es un arma de fuego
	if ( !IsMeleeWeapon() )
	{
		// Munición actual.
		int iAmmo = m_iClip1;

		// No ocupa clips para la munición. ¿granadas?
		if ( !UsesClipsForAmmo1() )
			iAmmo = pPlayer->GetAmmoCount( m_iPrimaryAmmoType );

		// Ya no tenemos munición en el arma y seguimos presionando el botón de ataque.
		// Reproducimos el sonido de "vacio".
		if ( iAmmo <= 0 )
		{
			// Descomenta esto (y habla con Iván) si deseas poner de regreso la
			// recarga automatica y/o cambio de arma cuando se acaba la munición.
			//HandleFireOnEmpty()

			WeaponSound( EMPTY );
			SendWeaponAnim( ACT_VM_DRYFIRE );
			SetNextPrimaryAttack( 0.5 );

			// Empezamos a recargar.
			if ( weapon_automatic_reload.GetBool() )
				Reload();

			return false;
		}
	}

	// Estamos bajo el agua y no podemos disparar.
	if ( pPlayer->GetWaterLevel() == 3 && !m_bFiresUnderwater )
	{
		WeaponSound( EMPTY );
		SendWeaponAnim( ACT_VM_DRYFIRE );
		SetNextPrimaryAttack( 0.5 );

		return false;
	}

	return true;
}

//====================================================================
// Devuelve si el arma puede efectuar un disparo primario.
// ¡Solo llamar desde PrimaryAttack()!
//====================================================================
bool CBaseInWeapon::CanSecondaryAttack()
{
	CIN_Player *pPlayer = GetPlayerOwner();

	// Jugador inválido.
	if ( !pPlayer )
	{
		Warning("[CBaseInWeapon::CanSecondaryAttack] Mi propietario no es un jugador. \n");
		return false;
	}

	// Aún no
	if ( m_flNextSecondaryAttack >= gpGlobals->curtime )
		return false;

	// Munición actual.
	int iAmmo = m_iClip2;

	// No ocupa clips para la munición. ¿granadas?
	if ( !UsesClipsForAmmo2() /*&& UsesSecondaryAmmo()*/ )
		iAmmo = pPlayer->GetAmmoCount( m_iSecondaryAmmoType );

	// Ya no tenemos munición en el arma y seguimos presionando el botón de ataque.
	// Reproducimos el sonido de "vacio".
	if ( iAmmo <= 0 )
	{
		WeaponSound( EMPTY );
		SendWeaponAnim( ACT_VM_DRYFIRE );
		SetNextSecondaryAttack( 0.5 );

		return false;
	}

	// Estamos bajo el agua y no podemos disparar.
	if ( pPlayer->GetWaterLevel() == 3 && !m_bAltFiresUnderwater )
	{
		WeaponSound( EMPTY );
		SendWeaponAnim( ACT_VM_DRYFIRE );
		SetNextSecondaryAttack( 0.5 );

		return false;
	}

	return true;
}

//====================================================================
// El jugador desea disparar la munición primaria.
//====================================================================
void CBaseInWeapon::PrimaryAttack()
{
	BaseClass::PrimaryAttack();
}

//====================================================================
// El jugador desea disparar la munición secundaria.
//====================================================================
void CBaseInWeapon::SecondaryAttack()
{
	BaseClass::SecondaryAttack();
}

//====================================================================
// Después de efectuar el disparo primario.
//====================================================================
void CBaseInWeapon::PostPrimaryAttack()
{
	CIN_Player *pPlayer = GetPlayerOwner();

	// Esto no debería pasar...
	if ( !pPlayer )
		return;

	// Hemos gastado uno de munición
	if ( !weapon_infiniteammo.GetBool() )
	{
		// Usamos clips de munición
		if ( UsesClipsForAmmo1() && m_iClip1 > 0 )
			--m_iClip1;
	}

	//
	pPlayer->DoMuzzleFlash();

	// Podemos volver a atacar en...
	SetNextAttack( GetInWpnData().m_flFireRate );
}

//====================================================================
// Después de efectuar el disparo secundario.
//====================================================================
void CBaseInWeapon::PostSecondaryAttack()
{
	CIN_Player *pPlayer = GetPlayerOwner( );

	// Esto no debería pasar...
	if ( !pPlayer )
		return;

	// Se ha diseñado la munición secundaria para que no tenga animación de recarga.
	// Hacemos una recarga instantanea aquí:
	if ( UsesClipsForAmmo2( ) && m_iClip2 <= 0 )
	{
		// Quitamos la munición de las reservas del jugador y le damos uno al arma.
		// Esto se hace automaticamente en el disparo en primera persona...

		pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );
		++m_iClip2;
	}

	// Podemos volver a atacar en...
	SetNextAttack( GetInWpnData().m_flSecondaryFireRate );
}

//====================================================================
// Dispara las balas del arma y realiza los efectos
// en cliente y servidor.
//====================================================================
void CBaseInWeapon::FireBullets()
{
	CIN_Player *pPlayer = ToInPlayer( GetOwner() );

	if ( !pPlayer )
		return;

	// Sonido
	WeaponSound( SINGLE, m_flNextPrimaryAttack );

#ifndef CLIENT_DLL
	// Compensación de Lag
	// Iván: LAG_COMPENSATE_HITBOXES ocaciona problemas con algunas animaciones de los NPC (Muerte)
	lagcompensation->StartLagCompensation( pPlayer, LAG_COMPENSATE_BOUNDS );
#endif

	// Información de las balas.
	FireBulletsInfo_t info;
	info.m_vecSrc			= pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting	= pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	info.m_iShots			= 1;
	info.m_flDistance		= GetInWpnData().m_flMaxRange;
	info.m_iAmmoType		= m_iPrimaryAmmoType;
	info.m_iTracerFreq		= 2;
	info.m_vecSpread		= GetBulletsSpread();
	info.m_flDamage			= GetInWpnData().m_iDamage;

	pPlayer->FireBullets( info );

#ifndef CLIENT_DLL
	// Terminamos compensación de Lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	// Ejecutamos la animación en el Viewmodel.
	SendWeaponAnim( GetPrimaryAttackActivity( ) );

	// Ejecutamos la animación en el jugador.
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	// Recoil
	AddViewKick();

	// El jugador ha disparado una bala.
	if ( pPlayer )
		pPlayer->OnFireBullets( info.m_iShots );
}

//====================================================================
// Dispara una bala del arma.
//====================================================================
void CBaseInWeapon::FireBullet()
{
	int iSeed = GetPredictionRandomSeed() & 255;
	RandomSeed( iSeed );

	float flDamage		= GetInWpnData().m_iDamage;
	float flSpread		= GetInWpnData().m_flSpread;
	float flMaxRange	= GetInWpnData().m_flMaxRange;
	float flDistance	= 0.0;

	int iDamageType		= DMG_BULLET | DMG_NEVERGIB;

	Vector vecSrc;
	trace_t tr;
	QAngle vAngles;

	CIN_Player *pPlayer		= GetPlayerOwner();
	CBaseEntity *pAttacker	= this;

	// Obtenemos la ubicación de donde saldrá la bala.
	if ( pPlayer )
	{
		vecSrc		= pPlayer->Weapon_ShootPosition();
		pAttacker	= pPlayer;
		vAngles		= pPlayer->EyeAngles() + pPlayer->GetPunchAngle();
	}
	else
	{
		vecSrc	= ShootPosition();
		vAngles = GetAbsAngles();
	}

	// Obtenemos las diferentes posiciones.
	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors( vAngles, &vecDirShooting, &vecRight, &vecUp );

	// circular gaussian spread
	float x = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );
	float y = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );

	// Calculamos hasta donde llegará la bala.
	Vector vecDir = vecDirShooting + x *
					flSpread * vecRight + y *
					flSpread * vecUp;
	VectorNormalize( vecDir );

	// Aquí terminará la trayectoría de la bala.
	Vector vecEnd = vecSrc + vecDir * flMaxRange;

	// Obtenemos las entidades/objetos que estan en la trayectoria de la bala.
	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tr );

	// No le hemos dado nada ¿disparo al aire?
	if ( tr.fraction == 1.0 )
		return;

	// Mostramos los impactos en sevidor y cliente.
	if ( sv_showimpacts.GetBool() )
	{
#ifdef CLIENT_DLL
		// draw red client impact markers
		debugoverlay->AddBoxOverlay( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 255,0,0,127, 4 );

		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			C_BasePlayer *player = ToBasePlayer( tr.m_pEnt );
			player->DrawClientHitboxes( 4, true );
		}
#else
		// draw blue server impact markers
		NDebugOverlay::Box( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,0,255,127, 4 );

		if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( tr.m_pEnt );
			player->DrawServerHitboxes( 4, true );
		}
#endif
	}

	// Calculamos la nueva distancia recorrida.
	flDistance += tr.fraction * flMaxRange;

	// Calculamos el nuevo daño según la distancia.
	flDamage *= pow( 0.85f, (flDistance / 500) );

#ifdef CLIENT_DLL
	// La bala ha terminado en el agua.
	if ( enginetrace->GetPointContents(tr.endpos) & (CONTENTS_WATER | CONTENTS_SLIME) )
	{
		trace_t waterTrace;
		UTIL_TraceLine( vecSrc, tr.endpos, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &waterTrace );

		if ( waterTrace.allsolid != 1 )
		{
			CEffectData	data;
			data.m_vOrigin = waterTrace.endpos;
			data.m_vNormal = waterTrace.plane.normal;
			data.m_flScale = random->RandomFloat( 8, 12 );

			if ( waterTrace.contents & CONTENTS_SLIME )
				data.m_fFlags |= FX_WATER_IN_SLIME;

			// Mostramos el efecto.
			DispatchEffect( "gunshotsplash", data );
		}
	}

	// Verificamos si ha terminado en algo solido.
	else
	{
		// No es: Skybox, NoDraw, Hint, Skip
		if ( !(tr.surface.flags & (SURF_SKY | SURF_NODRAW | SURF_HINT | SURF_SKIP)) )
		{
			UTIL_ImpactTrace( &tr, iDamageType );
		}
	}
#else

	//
	ClearMultiDamage();

	// Información del daño que hará la bala.
	CTakeDamageInfo pInfo( pAttacker, pAttacker, flDamage, iDamageType );

	// Rellenamos la información del daño con información de la bala.
	CalculateBulletDamageForce( &pInfo, GetBulletType(), vecDir, tr.endpos );

	// Notificamos del daño a los triggers
	TraceAttackToTriggers( pInfo, tr.startpos, tr.endpos, vecDir );

	ApplyMultiDamage();
#endif
}

//====================================================================
// Dispara/Lanza una granada.
//====================================================================
void CBaseInWeapon::FireGrenade()
{
	CBaseCombatCharacter *pOwner = GetOwner( );
	CIN_Player *pPlayer = ToInPlayer( pOwner );

	// Ejecutamos la animación en el jugador.
	if ( pPlayer && !pPlayer->IsDormant() )
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	// Ejecutamos la animación en el Viewmodel.
	SendWeaponAnim( GetSecondaryAttackActivity() );

	// Daño producido.
	//int iDamage = sk_plr_dmg_grenade.GetInt( );

	// Sonido de disparo. (Bum!)
	WeaponSound( WPN_DOUBLE );

#ifndef CLIENT_DLL
	// Compensación de Lag
	if ( pPlayer )
		lagcompensation->StartLagCompensation( pPlayer, LAG_COMPENSATE_HITBOXES );
#endif

	// Origen de disparo.
	Vector vecSrc = pOwner->EyePosition();
	Vector vecThrow;

	// 
	if ( pPlayer )
		AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	else
		AngleVectors( pPlayer->EyeAngles(), &vecThrow );

	//
	VectorScale( vecThrow, 1000.0f, vecThrow );

	// TODO: Lanzar una granada

	//pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );

#ifndef CLIENT_DLL
	if ( pPlayer )
		lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

//====================================================================
// Devuelve cada cuanto se puede lanzar una bala
//====================================================================
float CBaseInWeapon::GetFireRate()
{
	return GetInWpnData().m_flFireRate;
}

//====================================================================
// Devuelve el rango de propagación de las balas
//====================================================================
Vector CBaseInWeapon::GetBulletsSpread()
{
	CIN_Player *pPlayer = GetPlayerOwner();

	if ( !pPlayer )
		return VECTOR_CONE_5DEGREES;

	float flSpread = GetFireSpread();
	flSpread += ( 0.00873 * 1 );

	// Incapacitados
	if ( pPlayer->IsIncap() )
		flSpread += ( 0.00873 * 5 );

#ifndef CLIENT_DLL
	// Te estas moviendo
	if ( pPlayer->IsMoving() )
		flSpread += ( 0.00873 * 2 );
#endif

	// Esta saltando
	if ( !pPlayer->IsInGround() )
		flSpread += ( 0.00873 * 3 );

	// Estamos apuntando
	if ( IsIronsighted() )
		flSpread -= ( 0.00873 * 3 );

	// Esta agachado
	if ( (pPlayer->m_nButtons & IN_DUCK) )
		flSpread -= ( 0.00873 * 2 );

	// No debe ser menor a esta cantidad
	if ( flSpread < 0.00873 )
		flSpread = 0.00873;
	if ( flSpread > 0.17365 )
		flSpread = 0.17365;

	return Vector( flSpread, flSpread, flSpread );
}

//====================================================================
// Devuelve la propagación de las balas.
//====================================================================
float CBaseInWeapon::GetFireSpread()
{
	if ( weapon_fire_spread.GetFloat() > 0.0f )
		return weapon_fire_spread.GetFloat();

	return GetInWpnData().m_flSpread;
}

//====================================================================
//====================================================================
float CBaseInWeapon::GetFireKick()
{
	CIN_Player *pPlayer = GetPlayerOwner();

	float flKick = GetInWpnData().m_flViewKick;

	if ( !pPlayer )
		return flKick;

	// Incapacitados
	if ( pPlayer->IsIncap() )
		flKick += 4.0f;

#ifndef CLIENT_DLL
	// Te estas moviendo
	if ( pPlayer->IsMoving() )
		flKick += 0.5f;
#endif

	// Esta saltando
	if ( !pPlayer->IsInGround() )
		flKick += 2.0f;

	// Estamos apuntando
	if ( IsIronsighted() )
		flKick -= 1.5f;

	// Esta agachado
	if ( (pPlayer->m_nButtons & IN_DUCK) )
		flKick -= 1.5f;

	return flKick;
}

//====================================================================
// Establece en cuantos segundos más se podrá
// volver a disparar/atacar.
//====================================================================
void CBaseInWeapon::SetNextAttack( float flTime )
{
	SetNextPrimaryAttack( flTime );
	SetNextSecondaryAttack( flTime );
}

//====================================================================
// Establece en cuantos segundos más se podrá
// volver a disparar/atacar.
//====================================================================
void CBaseInWeapon::SetNextPrimaryAttack( float flTime )
{
	m_flNextPrimaryAttack = gpGlobals->curtime + flTime;
}

//====================================================================
// Establece en cuantos segundos más se podrá
// volver a disparar/atacar.
//====================================================================
void CBaseInWeapon::SetNextSecondaryAttack( float flTime )
{
	m_flNextSecondaryAttack = gpGlobals->curtime + flTime;
}

//====================================================================
// Establece en cuantos segundos más se podrá
// pasar al estado "Idle"
//====================================================================
void CBaseInWeapon::SetNextIdleTime( float flTime )
{
	SetWeaponIdleTime( gpGlobals->curtime + flTime );
}

//====================================================================
// Devuelve la posición del Viewmodel apuntando
//====================================================================
Vector CBaseInWeapon::GetIronsightPosition() const
{
	if( viewmodel_adjust_enabled.GetBool() )
		return Vector( viewmodel_adjust_forward.GetFloat(), viewmodel_adjust_right.GetFloat(), viewmodel_adjust_up.GetFloat() );

	return GetInWpnData().m_vecIronsightPos;
}

//====================================================================
// Devuelve la posición del Viewmodel apuntando
//====================================================================
QAngle CBaseInWeapon::GetIronsightAngles() const
{
	if( viewmodel_adjust_enabled.GetBool() )
		return QAngle( viewmodel_adjust_pitch.GetFloat(), viewmodel_adjust_yaw.GetFloat(), viewmodel_adjust_roll.GetFloat() );

	return GetInWpnData().m_angIronsightAng;
}

//====================================================================
// Devuelve el FOV del Viewmodel apuntando
//====================================================================
float CBaseInWeapon::GetIronsightFOV() const
{
	if ( viewmodel_adjust_enabled.GetBool() )
		return viewmodel_adjust_fov.GetFloat();

	return GetInWpnData().m_flIronsightFOV;
}

//====================================================================
// Devuelve si se esta apuntando con el arma
//====================================================================
bool CBaseInWeapon::IsIronsighted()
{
	return ( m_bIsIronsighted || viewmodel_adjust_enabled.GetBool() );
}

//====================================================================
// Activa o Desactiva el apuntado
//====================================================================
void CBaseInWeapon::ToggleIronsight()
{
	if ( m_bIsIronsighted )
		DisableIronsight();
	else
		EnableIronsight();
}

//====================================================================
// Activa la vista de hierro
//====================================================================
void CBaseInWeapon::EnableIronsight( void )
{
#ifdef CLIENT_DLL
	if ( gpGlobals->maxClients > 1 )
	{
		if ( !prediction->IsFirstTimePredicted() )
			return;
	}
#endif

	// Esta arma no puede apuntar
	if ( !HasIronsight() || IsIronsighted() )
		return;
 
	CIN_Player *pOwner = GetPlayerOwner();
 
	if ( !pOwner )
		return;

	// Estas incapacitado
	if ( pOwner->IsIncap() )
		return;

	// Ajustamos el FOV deseado
	bool bFOV = pOwner->SetFOV( this, pOwner->GetDefaultFOV() + GetIronsightFOV(), 1.0f );
	
	// Estamos apuntando
	if ( bFOV )
	{
		m_bIsIronsighted = true;
		SetIronsightTime();
	}
}

//====================================================================
// Desactiva la vista de hierro
//====================================================================
void CBaseInWeapon::DisableIronsight()
{
#ifdef CLIENT_DLL
	if ( gpGlobals->maxClients > 1 )
	{
		if ( !prediction->IsFirstTimePredicted() )
			return;
	}
#endif

	// Esta arma no puede apuntar
	if( !HasIronsight() || !IsIronsighted() )
		return;
 
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
 
	if( !pOwner )
		return;

	bool bFOV = pOwner->SetFOV( this, 0, 0.4f );
 
	if( bFOV )
	{
		m_bIsIronsighted = false;
		SetIronsightTime();
	}
}
 
//====================================================================
// Establece el tiempo que se activo la vista de Hierro
//====================================================================
void CBaseInWeapon::SetIronsightTime()
{
	const float delta = gpGlobals->curtime - m_flIronsightedTime;
	//note: that 2.5f should correspond to the 2.5f used in CBaseViewModel::CalcIronsights for the delta; you might want to make it a proper constant instead of some magic number

	if ( delta < 2.5f )
	{
		m_flIronsightedTime = gpGlobals->curtime - ( delta / 2.5f );
	}
	else
		m_flIronsightedTime = gpGlobals->curtime;
}

//====================================================================
//====================================================================
void CBaseInWeapon::WeaponSound( WeaponSound_t pType, float flSoundTime )
{
	if ( gpGlobals->maxClients <= 1 )
	{
		BaseClass::WeaponSound( pType, flSoundTime );
		return;
	}

#ifdef CLIENT_DLL
	const char *shootsound = GetShootSound( pType );

	if ( !shootsound || !shootsound[0] )
	{
		DevWarning( "[CBaseInWeapon::WeaponSound] Sonido invalido para: %s : %s \n", GetClassname(), shootsound );
		return;
	}

	CBroadcastRecipientFilter filter;

	if ( !te->CanPredict() )
		return;

	CBaseEntity::EmitSound( filter, GetPlayerOwner()->entindex(), shootsound, &GetPlayerOwner()->GetAbsOrigin() );
#else
	BaseClass::WeaponSound( pType, flSoundTime );
#endif
}

#ifndef CLIENT_DLL

//====================================================================
//====================================================================
void CBaseInWeapon::Spawn()
{
	BaseClass::Spawn();

	// Colisiones
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
}

//====================================================================
//====================================================================
void CBaseInWeapon::SendReloadEvents()
{
	CIN_Player *pPlayer = dynamic_cast< CIN_Player* >( GetOwner() );

	if ( !pPlayer )
		return;

	// Send a message to any clients that have this entity to play the reload.
	CPASFilter filter( pPlayer->GetAbsOrigin() );
	filter.RemoveRecipient( pPlayer );

	UserMessageBegin( filter, "ReloadEffect" );
		WRITE_SHORT( pPlayer->entindex() );
	MessageEnd();

	// Make the player play his reload animation.
	//pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
}

#else // CLIENT_DLL

void CBaseInWeapon::OnDataChanged( DataUpdateType_t pType )
{
	BaseClass::OnDataChanged( pType );

	if ( GetPredictable() && !ShouldPredict() )
		ShutdownPredictable();
}

bool CBaseInWeapon::ShouldPredict()
{
	if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
		return true;
	
	return false;
}

#endif // CLIENT_DLL