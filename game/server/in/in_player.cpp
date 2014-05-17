//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_player.h"
#include "in_ragdoll.h"
#include "in_buttons.h"
#include "in_ammodef.h"

#include "director.h"

#ifdef APOCALYPSE
	#include "ap_director.h"
#endif

#include "nav.h"
#include "nav_area.h"

#include "weapon_inbase.h"
#include "predicted_viewmodel.h"
#include "in_gamerules.h"
#include "players_manager.h"

#include "physics_prop_ragdoll.h"

#include "shake.h"
#include "rumble_shared.h"
#include "soundent.h"
#include "team.h"

extern int gEvilImpulse101;
extern void respawn( CBaseEntity *pEdict, bool fCopyCorpse );

extern int AE_FOOTSTEP_RIGHT;
extern int AE_FOOTSTEP_LEFT;

//====================================================================
// Comandos
//====================================================================

ConVar playermodel_sp( "cl_playermodel_sp", "models/survivors/survivor_teenangst.mdl", FCVAR_ARCHIVE, "Establece el modelo del jugador en el modo Historia" );

ConVar respawn_afterdeath( "in_respawn_afterdeath", "0", FCVAR_CHEAT | FCVAR_SERVER, "" );
ConVar wait_deathcam( "in_wait_deathcam", "4", FCVAR_SERVER, "" );

ConVar stamina_drain( "in_stamina_drain", "3", FCVAR_CHEAT | FCVAR_SERVER, "" );
ConVar stamina_infinite( "in_stamina_infinite", "0", FCVAR_CHEAT, "" );

ConVar health_regeneration( "in_helth_regeneration", "1", FCVAR_SERVER );
ConVar health_regeneration_recover( "in_health_regeneration_recover", "1", FCVAR_CHEAT );
ConVar health_regeneration_interval( "in_health_regeneration_interval", "15", FCVAR_CHEAT );

ConVar climb_height_check( "in_climb_height_check", "500", FCVAR_CHEAT );

ConVar weapon_single( "in_weapon_single", "1", FCVAR_SERVER );

//====================================================================
// Macros
//====================================================================

#define STAMINA_DRAIN		stamina_drain.GetFloat()
#define STAMINA_INFINITE	stamina_infinite.GetBool()

#define HEALTH_REGEN			health_regeneration.GetBool()
#define HEALTH_REGEN_RECOVER	health_regeneration_recover.GetInt()
#define HEALTH_REGEN_INTERVAL	gpGlobals->curtime + health_regeneration_interval.GetInt()

#define CLIMB_HEIGHT_CHECK	climb_height_check.GetInt()
#define WEAPON_SINGLE		weapon_single.GetBool()

#define NEXT_NAV_CHECK gpGlobals->curtime + 0.8f

//====================================================================
// Información y Red
//====================================================================

// DT_TEPlayerAnimEvent
IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( hPlayer ) ),
	SendPropInt( SENDINFO( iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nData ), 32 ),
END_SEND_TABLE()

#ifndef APOCALYPSE
	LINK_ENTITY_TO_CLASS( player, CIN_Player );
	PRECACHE_REGISTER( player );
#endif

// DT_InLocalPlayerExclusive
BEGIN_SEND_TABLE_NOBASE( CIN_Player, DT_InLocalPlayerExclusive )
	SendPropVector( SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropEHandle( SENDINFO(m_nRagdoll) )
END_SEND_TABLE()

// DT_InNonLocalPlayerExclusive
BEGIN_SEND_TABLE_NOBASE( CIN_Player, DT_InNonLocalPlayerExclusive )
	SendPropVector( SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()

// DT_InPlayer
IMPLEMENT_SERVERCLASS_ST( CIN_Player, DT_InPlayer )
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),	
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseAnimating", "m_nNewSequenceParity" ),
	SendPropExclude( "DT_BaseAnimating", "m_nResetEventsParity" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	
	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	// Información exclusiva al jugador.
	SendPropDataTable( "inlocaldata", 0, &REFERENCE_SEND_TABLE(DT_InLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
	SendPropDataTable( "innonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_InNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	SendPropBool( SENDINFO(m_bInCombat) ),
	SendPropBool( SENDINFO(m_bMovementDisabled) ),

	SendPropInt( SENDINFO(m_iEyeAngleZ) ),

	SendPropBool( SENDINFO(m_bIncap) ),
	SendPropBool( SENDINFO(m_bClimbingToHell) ),
	SendPropBool( SENDINFO(m_bWaitingGroundDeath) ),
	SendPropInt( SENDINFO(m_iTimesDejected) ),
	SendPropFloat( SENDINFO(m_flHelpProgress) ),
	SendPropFloat( SENDINFO(m_flClimbingHold) ),

	SendPropEHandle( SENDINFO(m_nPlayerInventory) ),
END_SEND_TABLE()

// DATADESC
BEGIN_DATADESC( CIN_Player )
	DEFINE_FIELD( m_angEyeAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_nRagdoll, FIELD_EHANDLE ),

	DEFINE_FIELD( m_bIsSprinting, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bMovementDisabled, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flStamina, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSanity, FIELD_FLOAT ),

	DEFINE_FIELD( m_bInCombat, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIncap, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_iTimesDejected, FIELD_INTEGER ),
	DEFINE_FIELD( m_flHelpProgress, FIELD_FLOAT ),
	DEFINE_FIELD( m_flClimbingHold, FIELD_FLOAT ),

	DEFINE_FIELD( m_flNextPainSound, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextDyingSound, FIELD_FLOAT ),

	DEFINE_FIELD( m_iNextRegeneration, FIELD_INTEGER ),
	DEFINE_FIELD( m_iNextFadeout, FIELD_INTEGER ),
	DEFINE_FIELD( m_iNextEyesHurt, FIELD_INTEGER ),

	DEFINE_FIELD( m_iEyeAngleZ, FIELD_INTEGER ),
END_DATADESC()

//====================================================================
//====================================================================

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
	CPVSFilter filter( (const Vector&) pPlayer->EyePosition( ) );

	g_TEPlayerAnimEvent.hPlayer = pPlayer;
	g_TEPlayerAnimEvent.iEvent = event;
	g_TEPlayerAnimEvent.nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

//====================================================================
// Permite la creación del Jugador 
// desde este tipo de clase.
//====================================================================
CIN_Player *CIN_Player::CreatePlayer( const char *pClassName, edict_t *pEdict, const char *pPlayerName )
{
	CBasePlayer::s_PlayerEdict = pEdict;

	CIN_Player *pPlayer = ( CIN_Player * )CreateEntityByName( pClassName );
	pPlayer->SetPlayerName( pPlayerName );

	return pPlayer;
}

//====================================================================
// Constructor
//====================================================================
CIN_Player::CIN_Player()
{
	// Las animaciones se calculan en el cliente
	UseClientSideAnimation();

	// Inicializamos la variable
	m_angEyeAngles.Init();
}

//====================================================================
// Destructor
//====================================================================
CIN_Player::~CIN_Player()
{
	if ( m_nAnimState )
		m_nAnimState->Release();

	// Eliminamos nuestro inventario
	if ( m_nPlayerInventory )
		UTIL_Remove( m_nPlayerInventory );
}

//=====================================================================
// Crea el procesador de animaciones
//=====================================================================
void CIN_Player::CreateAnimationState()
{
	m_nAnimState = CreatePlayerAnimationState( this );
}

//====================================================================
// Devuelve si el jugador esta presionando un botón.
//====================================================================
bool CIN_Player::IsPressingButton( int iButton )
{
	return ((m_nButtons & iButton)) ? true : false;
}

//====================================================================
// Devuelve si el jugador ha presionado un botón.
//====================================================================
bool CIN_Player::IsButtonPressed( int iButton )
{
	return ((m_afButtonPressed & iButton)) ? true : false;
}

//====================================================================
// Devuelve si el jugador ha dejado de presionar un botón.
//====================================================================
bool CIN_Player::IsButtonReleased( int iButton )
{
	return ((m_afButtonReleased & iButton)) ? true : false;
}

//====================================================================
// Devuelve el valor de un comando de cliente/servidor
//====================================================================
const char *CIN_Player::GetConVar( const char *pName )
{
	// Comando de cliente
	if ( strncmp(pName, "cl_", 3) == 0 )
		return engine->GetClientConVarValue( entindex(), pName );

	ConVarRef pVar( pName );

	if ( pVar.IsValid() )
		return pVar.GetString();
	
	return engine->GetClientConVarValue( entindex(), pName );
}

//====================================================================
// Ejecuta un comando
//====================================================================
void CIN_Player::ExecCommand( const char *pName )
{
	engine->ClientCommand( edict(), pName );
}

//====================================================================
// Envia una lección del Instructor de Juego
//====================================================================
void CIN_Player::SendLesson( const char *pLesson, bool bOnce, CBaseEntity *pSubject )
{
	PlysManager->SendLesson( pLesson, this, bOnce, pSubject );
}

//====================================================================
//====================================================================
int CIN_Player::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//====================================================================
// Creación por primera vez.
//====================================================================
void CIN_Player::InitialSpawn()
{
	m_takedamage	= DAMAGE_YES;
	pl.deadflag		= false;
	m_lifeState		= LIFE_ALIVE;

	// Creamos el procesador de animaciones
	CreateAnimationState();

	// Creamos nuestro inventario
	m_nPlayerInventory = CreatePlayerInventory( this );

	// No somos invisibles
	RemoveEffects( EF_NODRAW );

	// Equipo
	ChangeTeam( MyDefaultTeam() );

	// Preparamos la música
	PrepareMusic();

	// No pensamos.
	SetThink( NULL );
}

//====================================================================
// Creación en el mapa
//====================================================================
void CIN_Player::Spawn()
{
	// 
	DoAnimationEvent( PLAYERANIMEVENT_SPAWN );

	// Tipo de movimiento
	SetMoveType( MOVETYPE_WALK );

	// Propiedes fisicas
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	// Nos quitamos cualquier efecto de color
	SetRenderColor( 255, 255, 255 );

	// Sonidos
	m_flNextPainSound	= gpGlobals->curtime;
	m_flNextDyingSound	= gpGlobals->curtime; 

	// Cronometros
	m_nSlowTime.Invalidate();
	m_nFinishCombat.Invalidate();
	m_nFinishUnderAttack.Invalidate();

	// Más información
	m_flSanity			= 100.0f;
	m_flStamina			= 100.0f;
	m_bIsSprinting		= true;
	m_iNextRegeneration = gpGlobals->curtime;
	m_iNextFadeout		= gpGlobals->curtime;
	m_iNextEyesHurt		= gpGlobals->curtime;
	m_iEyeAngleZ		= 0;
	m_nRagdoll			= NULL;

	m_bIncap			= false;
	m_bClimbingToHell	= false;
	m_iTimesDejected	= 0;

	m_iLastNavAreaIndex	= 0;
	m_iNextNavCheck		= gpGlobals->curtime;
	m_iNextDejectedHurt	= gpGlobals->curtime;

	m_nLessonsList.Purge();

	// Paramos la Música
	StopMusic();

	// Ya no estamos abatidos
	StopIncap();

	// Spawn!
	BaseClass::Spawn();

	// Estas en el suelo
	AddFlag( FL_ONGROUND );

	// Establecemos el modelo.
	SetModel( GetPlayerModel() );

	// Spawn para el modo Multijugador
	if ( InRules->IsMultiplayer() )
	{
		SpawnMultiplayer();
	}	
}

//====================================================================
// Creación en el mapa [Modo Multiplayer]
//====================================================================
void CIN_Player::SpawnMultiplayer()
{	
}

//====================================================================
// Guarda objetos necesarios en caché.
//====================================================================
void CIN_Player::Precache()
{
	BaseClass::Precache();

	// Este modelo es importante, no lo usamos visualmente pero si en el código (Como para generar nodos de movimiento)
	PrecacheModel( "models/player.mdl" );

	PrecacheMaterial( "sprites/light_glow01" );
	PrecacheMaterial( "sprites/glow01" );
	PrecacheMaterial( "sprites/spotlight01_proxyfade" );

	PrecacheMaterial( "effects/flashlight_border" );
	PrecacheMaterial( "effects/flashlight001" );
	PrecacheMaterial( "effects/muzzleflash_light" );
	
	PrecacheScriptSound( "Player.FlashLightOn" );
	PrecacheScriptSound( "Player.FlashLightOff" );
	PrecacheScriptSound( "Player.Dying.Minor" ); // FIXME

	// Modelo del Jugador
	PrecacheModel( GetPlayerModel() );

	// Caché para el Multiplayer
	if ( InRules->IsMultiplayer() )
	{
		PrecacheMultiplayer();
	}
}

//====================================================================
// Guarda objetos necesarios en caché para el modo Multiplayer
//====================================================================
void CIN_Player::PrecacheMultiplayer()
{
}

//====================================================================
//====================================================================
void CIN_Player::PreThink()
{
	// Estamos en un vehiculo.
	if ( IsInAVehicle() )
	{
		UpdateClientData();
		CheckTimeBasedDamage();

		CheckSuitUpdate();

		WaterMove();
		return;
	}

	// Correr
	if ( IsPressingButton(IN_SPEED) )
		StartSprint();
	else
		StopSprint();

	BaseClass::PreThink();
}

//====================================================================
//====================================================================
void CIN_Player::PostThink()
{
	// PostThink!
	BaseClass::PostThink();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );

	// Guardamos el angulo de los ojos y lo enviamos al cliente.
	m_angEyeAngles = EyeAngles();

	// Actualizamos nuestra animación
	m_nAnimState->Update();

	if ( IsAlive() )
	{
		// Regeneración de vida
		UpdateHealthRegeneration();

		// Incapacitación
		UpdateIncap();

		// Energía
		UpdateStamina();

		// Velocidad
		UpdateSpeed();

		// Cordura
		UpdateSanity();

		// Sonidos
		UpdateSounds();

		// Cansancio
		UpdateTiredEffects();

		// Caida
		UpdateFallEffects();

		// ¿Caeremos o estamos colgando?
		UpdateFallToHell();

		// ¿En combate?
		if ( m_nFinishCombat.HasStarted() && !m_nFinishCombat.IsElapsed() )
			m_bInCombat = true;
		else
			m_bInCombat = false;

		// ¿Bajo ataque?
		if ( m_nFinishUnderAttack.HasStarted() && !m_nFinishUnderAttack.IsElapsed() )
			m_bUnderAttack = true;
		else
			m_bUnderAttack = false;

		// ¿En donde estamos?
		if ( gpGlobals->curtime > m_iNextNavCheck )
		{
			UpdateLastKnownArea();
			m_iNextNavCheck = NEXT_NAV_CHECK;
		}
	}
}

//====================================================================
// El Jugador trata de usar algo
//====================================================================
void CIN_Player::PlayerUse()
{
	BaseClass::PlayerUse();

	// Was use pressed or released?
	if ( ! ((m_nButtons | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

	// Somos un espectador
	if ( IsObserver() )
		return;

	CBaseEntity *pUseEntity = FindUseEntity();

	// Verificamos si la entidad que tratamos de usar es un arma
	if ( pUseEntity && IsAllowedToPickupWeapons() )
	{
		CBaseInWeapon *pWeapon = dynamic_cast<CBaseInWeapon *>( pUseEntity );

		if ( pWeapon )
		{
			// Ya la tenemos, solo recojeremos su munición
			if ( Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType() ) )
				Weapon_EquipAmmoOnly( pWeapon );

			// Recojeremos el arma
			else
			{
				Weapon_Equip( pWeapon );
			}
		}
	}
}

//====================================================================
// Devuelve el Model pedido por el Jugador
//====================================================================
const char *CIN_Player::GetPlayerModel()
{
	// Estamos en el Modo Historia
	if ( !InRules->IsMultiplayer() )
		return playermodel_sp.GetString();

	return GetPlayerModelValidated();
}

//====================================================================
// Valida y establece el modelo del jugador
//====================================================================
const char *CIN_Player::GetPlayerModelValidated()
{
	const char *pPlayerModel = GetConVar( "cl_playermodel" );
	int iRand = 0;

	// El modelo que desea el Jugador no es válido.
	if ( !IsValidModel(pPlayerModel) )
	{
		// Seleccionamos uno al azar
		iRand			= RandomInt( 0, ARRAYSIZE(m_nPlayerModels) - 1);
		pPlayerModel	= m_nPlayerModels[ iRand ];
	}

	ExecCommand( UTIL_VarArgs("cl_playermodel %s", pPlayerModel) );

	// Devolvemos el Modelo
	return pPlayerModel;
}

//====================================================================
// Devuelve si el modelo es válido
//====================================================================
bool CIN_Player::IsValidModel( const char *pModel )
{
	// Usemos el modelo que ha seleccionado el jugador
	if ( pModel == NULL )
		pModel = GetConVar( "cl_playermodel" );

	for ( int i = 0; i < ARRAYSIZE(m_nPlayerModels); ++i )
	{
		if ( FStrEq(m_nPlayerModels[i], pModel) )
			return true;
	}

	// No es válido
	return false;
}

//====================================================================
// Prepara la música del jugador
//====================================================================
void CIN_Player::PrepareMusic()
{
}

//====================================================================
// Para la música del jugador
//====================================================================
void CIN_Player::StopMusic()
{
}

//====================================================================
// Reproduce el Script de sonido de acuerdo al modelo del jugador
//====================================================================
void CIN_Player::EmitPlayerSound( const char *pSoundName )
{
	const char *pRealSoundName	= UTIL_VarArgs("%s.%s", GetPlayerType(), pSoundName);
	Vector vecOrigin			= GetAbsOrigin();

	// El archivo de audio no existe
	CSoundParameters params;
	if ( GetParametersForSound( pRealSoundName, params, STRING(GetModelName()) ) == false )
		return;

	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

	EmitSound_t ep;
	ep.m_nChannel	= params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume	= params.volume;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags		= 0;
	ep.m_nPitch		= params.pitch;
	ep.m_pOrigin	= &vecOrigin;

	EmitSound( filter, entindex(), ep );
}

//====================================================================
//====================================================================
void CIN_Player::StopPlayerSound( const char *pSoundName )
{
	const char *pRealSoundName	= UTIL_VarArgs("%s.%s", GetPlayerType(), pSoundName);
	StopSound( pRealSoundName );
}

//====================================================================
// Devuelve el tipo de jugador
//====================================================================
const char *CIN_Player::GetPlayerType()
{
	// TODO
	return "Abigail";
}

//====================================================================
// Sonido de dolor
//====================================================================
void CIN_Player::PainSound( const CTakeDamageInfo &info )
{
	
}

//====================================================================
// Sonido de poca salud ¡me muero!
//====================================================================
void CIN_Player::DyingSound()
{
	
}

//====================================================================
// Sonido al morir
//====================================================================
void CIN_Player::DieSound()
{
	// Mi cadaver se esta disolviendo
	if ( m_nRagdoll && m_nRagdoll->GetBaseAnimating()->IsDissolving() )
		 return;

	if ( !InRules->FCanPlayDeathSound(m_nLastDamageInfo) )
		return;

	EmitPlayerSound("Death");
}

//====================================================================
// Actualiza las voces/sonidos
//====================================================================
void CIN_Player::UpdateSounds()
{
	// Sufro!
	DyingSound();
}

//====================================================================
// Enciende la linterna.
//====================================================================
bool CIN_Player::FlashlightTurnOn( bool playSound )
{
	// No podemos encender la linterna
	if ( !InRules->FAllowFlashlight() )
		return false;

	// Ya esta encendida
	if ( FlashlightIsOn() )
		return false;

	AddEffects( EF_DIMLIGHT );

	if ( playSound )
		EmitSound("Player.FlashLightOn");

	return true;
}

//====================================================================
// Apaga la linterna.
//====================================================================
void CIN_Player::FlashlightTurnOff( bool playSound )
{
	// No esta encendida
	if ( !FlashlightIsOn() )
		return;

	RemoveEffects( EF_DIMLIGHT );

	if ( playSound )
		EmitSound("Player.FlashLightOff");
}

//====================================================================
// Devuelve si la linterna esta encendida.
//====================================================================
int CIN_Player::FlashlightIsOn()
{
	return IsEffectActive( EF_DIMLIGHT );
}

//====================================================================
// Devuelve el arma de clasificación primaria
//====================================================================
CBaseInWeapon *CIN_Player::GetPrimaryWeapon()
{
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CBaseInWeapon *pWeapon = ( CBaseInWeapon * )GetWeapon( i );

		if ( !pWeapon )
			continue;

		if ( pWeapon->GetWeaponClass() == CLASS_PRIMARY_WEAPON )
			return pWeapon;
	}

	return NULL;
}

//====================================================================
// Devuelve el arma de clasificación secundaria
//====================================================================
CBaseInWeapon *CIN_Player::GetSecondaryWeapon()
{
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CBaseInWeapon *pWeapon = ( CBaseInWeapon * )GetWeapon( i );

		if ( !pWeapon )
			continue;

		if ( pWeapon->GetWeaponClass() == CLASS_SECONDARY_WEAPON )
			return pWeapon;
	}

	return NULL;
}

//====================================================================
//====================================================================
void CIN_Player::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	CBaseInWeapon *pInWeapon = (CBaseInWeapon *)pWeapon;

	if ( !pInWeapon )
		return;

	if ( WEAPON_SINGLE )
	{
		// Tiramos el arma de esta clasificación
		Weapon_DropClass( pInWeapon->GetWeaponClass() );
	}

	// No es un arma cuerpo a cuerpo, entonces recuerda apuntar
	if ( pWeapon->UsesClipsForAmmo1() )
		SendLesson( "hint_ironsight", true );

	BaseClass::Weapon_Equip( pWeapon );
}

//====================================================================
//====================================================================
void CIN_Player::Weapon_DropClass( int iWeaponClass )
{
	CBaseInWeapon *pWeapon = NULL;

	switch ( iWeaponClass )
	{
		case CLASS_PRIMARY_WEAPON:
			pWeapon = GetPrimaryWeapon();
		break;

		case CLASS_SECONDARY_WEAPON:
			pWeapon = GetSecondaryWeapon();
		break;
	}

	if ( !pWeapon )
		return;

	// Tiramos el arma
	Weapon_Drop( pWeapon, NULL, NULL );
}

//====================================================================
// [Evento] El jugador ha recibido daño
//====================================================================
int CIN_Player::OnTakeDamage( const CTakeDamageInfo &info )
{
	// Último daño hecho
	m_nLastDamageInfo = info;

	return BaseClass::OnTakeDamage( info );
}

//====================================================================
// [Evento] El jugador ha recibido daño pero sigue vivo
//====================================================================
int CIN_Player::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Mi atacante es un jugador
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		CIN_Player *pAttacker = ToInPlayer( info.GetAttacker() );

		// No debemos recibir daño de fuego amigo de un BOT
		if ( pAttacker != this && InRules->PlayerRelationship(this, pAttacker) == GR_TEAMMATE && pAttacker->IsFakeClient() )
		{
			return 0;
		}
	}

	// Nuestra salud después de recibir el daño
	int newHealth = GetHealth() - info.GetDamage();

	// El daño nos hace lentos
	if ( InRules->DamageCanMakeSlow(info) )
		m_nSlowTime.Start( 2 );

	// Quitamos cordura
	RemoveSanity( 5.0f );

	// Efectos ¡Ouch!
	color32 red = {211, 0, 0, 100}; // Rojo
	UTIL_ScreenFade( this, red, 0.4, 0, FFADE_IN );
	
	// Ouch!
	PainSound( info );

	// Estaremos muertos con este daño
	if ( newHealth <= 0 && InRules->FPlayerCanDejected(this, info) )
	{
		// ¡Nos han abatido!
		if ( !IsIncap() )
		{
			if ( m_iTimesDejected < 2 )
			{
				StartIncap();
				return 0;
			}
		}
		else
		{
			// Estabamos trepando
			if ( IsClimbingIncap() )
			{
				// Te espera la muerte abajo...
				m_flClimbingHold = -5.0f;
				return 0;
			}
		}
	}

	// ¡Estamos bajo ataque!
	if ( info.GetDamageType() & (DMG_BULLET | DMG_SLASH) )
	{
		m_nFinishUnderAttack.Start( 3 );
	}

	// Flinch
	switch ( LastHitGroup() )
	{
		case HITGROUP_HEAD:
			DoAnimationEvent( PLAYERANIMEVENT_FLINCH_HEAD );
		break;

		case HITGROUP_CHEST:
			DoAnimationEvent( PLAYERANIMEVENT_FLINCH_CHEST );
		break;

		case HITGROUP_LEFTARM:
			DoAnimationEvent( PLAYERANIMEVENT_FLINCH_LEFTARM );
		break;

		case HITGROUP_RIGHTARM:
			DoAnimationEvent( PLAYERANIMEVENT_FLINCH_RIGHTARM );
		break;

		case HITGROUP_LEFTLEG:
			DoAnimationEvent( PLAYERANIMEVENT_FLINCH_LEFTLEG );
		break;

		case HITGROUP_RIGHTLEG:
			DoAnimationEvent( PLAYERANIMEVENT_FLINCH_RIGHTLEG );
		break;
	}

	// OnTakeDamage_Alive!
	return BaseClass::OnTakeDamage_Alive( info );
}

//====================================================================
// [Evento] El jugador ha muerto
//====================================================================
void CIN_Player::Event_Killed( const CTakeDamageInfo &info )
{
	CSound *pSound			= CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex(edict()) );
	IPhysicsObject *pObject = VPhysicsGetObject();

	// Adios...!
	DieSound();

	// Notificamos a las reglas del juego sobre mi muerte
	InRules->PlayerKilled( this, info );

	//
	//gamestats->Event_PlayerKilled( this, info );

	// Paramos la vibración del control
	RumbleEffect( RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE );

	// Ya no estamos usando nada
	ClearUseEntity();

	// Paramos la música
	StopMusic();

	// Ya no estamos incapacitados
	StopIncap();

	// Reiniciamos el sonido
	if ( pSound )
		pSound->Reset();

	// Limpiamos los sonidos de las armas
	EmitSound( "BaseCombatCharacter.StopWeaponSounds" );

	// Propiedades físicas
	if ( pObject )
		pObject->RecheckContactPoints();

	// Evitamos problemas con la salud
	if ( m_iHealth < 0 )
		m_iHealth = 0;

	// Apagamos nuestra linterna
	if ( FlashlightIsOn() )
		 FlashlightTurnOff();

	// Ya no estamos en ningún area del NavMesh
	ClearLastKnownArea();

	// Informamos a nuestro atacante que nos ha eliminado
	if ( info.GetAttacker() )
	{
		info.GetAttacker()->Event_KilledOther(this, info);
		//g_EventQueue.AddEvent( info.GetAttacker(), "KilledNPC", 0.3, this, this );
	}

	//
	SendOnKilledGameEvent( info );

	// Estoy en proceso de morir
	pl.deadflag			= true;
	m_flDeathTime		= gpGlobals->curtime;
	m_lifeState			= LIFE_DYING;
	m_vecDeathOrigin	= GetAbsOrigin();

	// Animación a morir
	if ( InRules->CanPlayDeathAnim(this, info) )
		DoAnimationEvent( PLAYERANIMEVENT_DIE );
}

//====================================================================
// Pensamiento cuando el Jugador ha muerto
//====================================================================
void CIN_Player::PlayerDeathThink()
{
	// Volvemos a pensar en 0.1s
	SetNextThink( gpGlobals->curtime + 0.1f );

	// Estamos en el suelo
	if ( IsInGround() )
	{
		float flForward = GetAbsVelocity().Length() - 20;

		if ( flForward <= 0 )
			SetAbsVelocity( vec3_origin );
		else
		{
			Vector vecNewVelocity = GetAbsVelocity();
			VectorNormalize( vecNewVelocity );
			vecNewVelocity *= flForward;
			SetAbsVelocity( vecNewVelocity );
		}
	}

	// Tenemos armas
	if ( HasWeapons() )
		PackDeadPlayerItems();

	// Seguimos esperando a que termine nuestra animación de muerte
	if ( m_lifeState == LIFE_DYING )
	{
		if ( GetModelIndex() && !IsSequenceFinished() && InRules->CanPlayDeathAnim(this, m_nLastDamageInfo) )
		{
			++m_iRespawnFrames;

			// En caso de que ocurra un error con la animación esto evitará que nos quedemos en este estado
			if ( m_iRespawnFrames < 60 )
				return;
		}

		// Creamos nuestro cadaver
		CreateRagdollEntity( m_nLastDamageInfo );

		// Ya no estamos en el suelo
		SetMoveType( MOVETYPE_FLYGRAVITY );
		SetGroundEntity( NULL );

		// Ahora no somos solidos
		AddSolidFlags( FSOLID_NOT_SOLID );

		// Somos invisibles
		AddEffects( EF_NODRAW );

		// Hemos muerto oficialmente
		m_lifeState			= LIFE_DEAD;
		m_flDeathAnimTime	= gpGlobals->curtime;
	}

	// Paramos todas las animaciones (Para este momento somos invisibles)
	StopAnimation();

	//
	AddEffects( EF_NOINTERP );
	m_flPlaybackRate = 0.0;

	// 
	PlayerDeathPostThink();
}

//====================================================================
// Pensamiento cuando el Jugador ha muerto y listo para
// respawnear
//====================================================================
void CIN_Player::PlayerDeathPostThink()
{
	// Este Jugador no puede reaparecer
	if ( !InRules->FPlayerCanRespawn(this) )
		return;

	// ¿Hemos presionado algún boton?
	int anyButtonDown = ( m_nButtons & ~IN_SCORE );

	// ¡Podemos reaparecer!
	m_lifeState = LIFE_RESPAWNABLE;

	// ¿Podemos reaparecer ya?
	bool canRespawnNow = InRules->FPlayerCanRespawnNow( this );

	//
	// Modo Historia
	//
	if ( !InRules->IsMultiplayer() )
	{
		// Activamos la escala de grises.
		//if ( tired_effect.GetBool() )
		{
			ConVarRef mat_yuv("mat_yuv");
			ExecCommand("mat_yuv 1");
		}

		// Debemos presionar un botón para reaparecer
		if ( !anyButtonDown && canRespawnNow )
			canRespawnNow = false;
	}

	//
	// Modo Multijugador
	//
	else
	{
		// Pasamos a modo espectador ( Camara libre o mirando a los jugadores )
		if ( gpGlobals->curtime > GetDeathTime() + wait_deathcam.GetFloat() && !IsObserver() )
		{
			CIN_Player *pSpectate = NULL;
			pSpectate = PlysManager->GetNear( GetAbsOrigin(), GetTeamNumber() );

			if ( pSpectate )
			{
				Spectate( pSpectate );
			}
		}
	}

	// Ya podemos reaparecer
	if ( canRespawnNow )
	{
		SetNextThink( TICK_NEVER_THINK );
		respawn( this, false );

		m_nButtons			= 0;
		m_iRespawnFrames	= 0;
	}
}

//====================================================================
// Crea el cadaver del jugador al morir
//====================================================================
void CIN_Player::CreateRagdollEntity( const CTakeDamageInfo &info )
{
	// Eliminamos nuestro último cadaver
	if ( m_nRagdoll )
		UTIL_Remove( m_nRagdoll );

	// Creamos un cadaver Server-side
	// FIXME: Cadaveres client-side
	m_nRagdoll = CreateServerRagdoll( this, m_nForceBone, info, COLLISION_GROUP_DEBRIS );
	
	/*
	CIN_Ragdoll *pRagdoll = ToRagdoll( m_nRagdoll.Get() );

	// Eliminamos nuestro anterior cadaver
	if ( m_nRagdoll )
	{
		UTIL_Remove( m_nRagdoll );
		m_nRagdoll = NULL;
	}

	// Creamos un nuevo cadaver
	pRagdoll = ToRagdoll( CreateEntityByName("in_ragdoll") );

	if ( pRagdoll )
	{
		pRagdoll->hPlayer = this;

		pRagdoll->vecRagdollOrigin		= GetAbsOrigin();
		pRagdoll->vecRagdollVelocity	= GetAbsVelocity();
		pRagdoll->m_nModelIndex			= m_nModelIndex;
		pRagdoll->m_nForceBone			= m_nForceBone;
		pRagdoll->m_vecForce			= vec3_origin;
	}

	m_nRagdoll = pRagdoll;
	*/
}


//====================================================================
// Pensamiento: Actualiza el aguante del Jugador
//====================================================================
void CIN_Player::UpdateStamina()
{
	// No estas usando tu aguante
	if ( !(m_nButtons & IN_SPEED) || !IsMoving() || STAMINA_INFINITE )
	{
		// Estamos llenos de energía
		if ( m_flStamina >= 100.0f )
			return;

		m_flStamina += 5 * gpGlobals->frametime;
	}

	// Estas corriendo
	else
	{
		if ( m_flStamina < 0.0f )
			return;

		m_flStamina -= STAMINA_DRAIN * gpGlobals->frametime;
	}

	// No tienes suficiente energía
	//if ( m_flStamina <= 10.0f )
		//StopSprint();

	// Evitamos valores inválidos
	if ( m_flStamina <= -1 )
		m_flStamina = 0.0f;
	if ( m_flStamina > 100.0f )
		m_flStamina = 100.0f;
}

//====================================================================
// Pensamiento: Actualiza la regenación de salud
//====================================================================
void CIN_Player::UpdateHealthRegeneration()
{
	// La regeneración esta desactivada
	if ( !HEALTH_REGEN )
		return;

	// Estamos incapacitados
	if ( IsIncap() )
		return;

	// No hay regeneración de salud en Dificil
	if ( InRules->IsSkillLevel(SKILL_HARD) )
		return;

	// Aún no toca
	if ( gpGlobals->curtime <= m_iNextRegeneration )
		return;

	int toRecover = HEALTH_REGEN_RECOVER;

	// 2 más en Fácil
	if ( InRules->IsSkillLevel(SKILL_EASY) )
		toRecover += 2;

	TakeHealth( toRecover, DMG_GENERIC );
	m_iNextRegeneration = HEALTH_REGEN_INTERVAL;
}

//====================================================================
// Pensamiento: Actualiza los efectos de cansancio
//====================================================================
void CIN_Player::UpdateTiredEffects()
{
	color32 black = { 0, 0, 0, 255 };

	//
	// Estamos incapacitados
	//
	if ( IsIncap() )
	{
		SnapEyeAnglesZ( 10 );

		// Cerramos los ojos
		if ( gpGlobals->curtime >= m_iNextFadeout && !IsClimbingIncap() && GetHealth() > 30 )
		{
			UTIL_ScreenFade( this, black, 1.0f, 0.5f, FFADE_IN );
			m_iNextFadeout = gpGlobals->curtime + RandomInt(3, 10);
		}

		return;
	}
	
	//
	// Tenemos menos de 30% de salud
	//
	if ( GetHealth() <= 30 )
	{
		SnapEyeAnglesZ( 3 );

		// Encerio, nos duele
		if ( gpGlobals->curtime >= m_iNextEyesHurt )
		{
			int angHealth = ( 70 - GetHealth() ) / 8;
			ViewPunch( QAngle(RandomFloat(2.0, angHealth), RandomFloat(2.0, angHealth), RandomFloat(2.0, angHealth)) );

			m_iNextEyesHurt = gpGlobals->curtime + RandomInt(5, 15);
		}
	}
	else
	{
		SnapEyeAnglesZ( 0 );
	}
}

//====================================================================
// Actualiza los efectos de una GRAN caida
//====================================================================
void CIN_Player::UpdateFallEffects()
{

}

//====================================================================
// Actualiza y verifica si el Jugador va a caer
//====================================================================
void CIN_Player::UpdateFallToHell()
{
	// Ya te has quedado trepando
	if ( m_bClimbingToHell )
		return;

	// Estas en el suelo
	if ( IsInGround() )
		return;

	// Somos dios o estamos volando
	if ( IsGod() || GetMoveType() != MOVETYPE_WALK )
		return;

	trace_t	frontTrace;
	trace_t frontDownTrace;
	trace_t downTrace;

	// Obtenemos los vectores de las distintas posiciones
	Vector vecForward, vecRight, vecUp, vecSrc;
	GetVectors( &vecForward, &vecRight, &vecUp );

	// Nuestra ubicación
	vecSrc = GetAbsOrigin();

	// Trazamos una linea de 50 delante nuestra
	UTIL_TraceLine( vecSrc, vecSrc + vecForward * 50, MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &frontTrace );

	// Trazamos una línea de 600 hacia abajo de donde termina la línea delante
	UTIL_TraceLine( frontTrace.endpos, frontTrace.endpos + -vecUp * CLIMB_HEIGHT_CHECK, MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &frontDownTrace );

	// Trazamos una línea de 600 hacia abajo de nosotros
	UTIL_TraceLine( vecSrc, vecSrc + -vecUp * CLIMB_HEIGHT_CHECK, MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &downTrace );

	// El salto es seguro
	if ( frontDownTrace.fraction != 1.0 || downTrace.fraction != 1.0 )
		return;

	StartIncap( true );
}


//====================================================================
// El Jugador ha sido incapacitado
//====================================================================
void CIN_Player::StartIncap( bool bClimbing )
{
	// Desactivamos el movimiento y la capacidad de saltar/agacharse
	DisableButtons( IN_JUMP | IN_DUCK );
	DisableMovement();

	// Hemos caido
	if ( !m_bIncap )
	{
		// Restauramos nuestra salud
		SetHealth( GetMaxHealth() );
		SetHullType( HULL_WIDE_SHORT );

		// No es lógico que digamos esto si estamos solos
		if ( InRules->IsMultiplayer() && !bClimbing )
			EmitPlayerSound("Start.Dejected");

		++m_iTimesDejected;
	}

	// Hemos quedado trepando
	if ( bClimbing )
	{
		StartClimbIncap();
	}
	else
	{
		// Ajustamos la vista
		SetViewOffset( InRules->GetInViewVectors()->m_nVecDejectedView );
	}

	m_bIncap			= true;
	m_flHelpProgress	= 0.0f;

	OnStartIncap();
}

//===============================================================================
// El Jugador ha sido revivido
//===============================================================================
void CIN_Player::StopIncap()
{
	// Activamos el movimiento y los botones
	EnableButtons( IN_JUMP | IN_DUCK | IN_ATTACK | IN_ATTACK2 );
	EnableMovement();

	m_bIncap				= false;
	m_bClimbingToHell		= false;
	m_bWaitingGroundDeath	= false;

	// Restauramos la mitad de la salud máxima
	int newHealth = GetMaxHealth() / 2;

	// En dificil restauramos solo 1/3
	if ( InRules->IsSkillLevel(SKILL_HARD) )
		newHealth = GetMaxHealth() / 3;

	SetHealth( newHealth );

	// Restauramos
	SetHullType( HULL_HUMAN );
	SetViewOffset( VEC_VIEW );

	OnStopIncap();
}

//===============================================================================
//===============================================================================
void CIN_Player::StartClimbIncap()
{
	// Desactivamos los botones de ataque
	DisableButtons( IN_ATTACK | IN_ATTACK2 );

	// Estas trepando por tu vida
	m_bClimbingToHell	= true;
	m_flClimbingHold	= 350.0f;
	
	// Cambiamos el tipo de movimiento para que quede flotando
	SetMoveType( MOVETYPE_FLY );
	SetAbsVelocity( vec3_origin );
}

//===============================================================================
//===============================================================================
void CIN_Player::StopClimbIncap()
{
	// Te espera la muerte abajo...
	m_bWaitingGroundDeath = true;

	// Regresamos a la normalidad ¡cayendo!
	SetMoveType( MOVETYPE_WALK );
}

//====================================================================
// Pensamiento: Al estar incapacitado
//====================================================================
void CIN_Player::UpdateIncap()
{
	// No estamos incapacitados
	if ( !IsIncap() )
		return;

	//
	// Incapacitación en el suelo
	//
	if ( !IsClimbingIncap() )
	{
		// Pierdes vida por estar incapacitado
		if ( gpGlobals->curtime >= m_iNextDejectedHurt )
		{
			int newHealth = GetHealth() - RandomInt(1, 3);

			if ( newHealth > 0 )
				SetHealth( newHealth );
			else
				TakeDamage( CTakeDamageInfo(this, this, 5, DMG_GENERIC) );

			m_iNextDejectedHurt = gpGlobals->curtime + RandomInt( 3, 6 );
		}

		// Nos ayudamos a nosotros mismos
		if ( IsPressingButton(IN_USE) )
		{
			HelpIncap( this );
		}

		return;
	}

	//
	// Incapacitación por quedar colgando
	//

	// No debemos movernos
	SetAbsVelocity( vec3_origin );

	// Estabas esperando la muerte en el suelo
	if ( IsWaitingGroundDeath() )
	{
		// Ya estas en el suelo
		if ( IsInGround() )
		{
			// Iván: En ocaciones el código puede detectar erroneamente que el "siguiente paso" del Jugador
			// es una caida hacia su muerte, por lo que no hay que ser injustos y en caso de que caiga a 
			// una altura muy baja solo paramos el estado de incapacitación
			StopIncap();
		}
	}

	// ¡Sigues colgando!
	else
	{
		// Presionar varias veces la tecla de uso ayuda
		if ( IsButtonPressed(IN_USE) )
		{
			m_flClimbingHold += 0.5;
		}

		// ¡Vas perdiendo aguante!
		m_flClimbingHold -= 8 * gpGlobals->frametime;

		// ¡¡Ya no aguanto más!!
		if ( m_flClimbingHold <= 0.0f )
		{
			// Te espera la muerte abajo...
			StopClimbIncap();
		}
	}
}

//====================================================================
// Alguien nos esta ayudando
//====================================================================
void CIN_Player::HelpIncap( CIN_Player *pPlayer )
{
	// No estas incapacitado
	if ( !IsIncap() )
		return;

	m_flHelpProgress += 0.4f;

	DevMsg( "[CIN_Player::GetUp] %s esta ayudando a %s (%f) \n", pPlayer->GetPlayerName(), GetPlayerName(), m_flHelpProgress );

	Vector vecView = InRules->GetInViewVectors()->m_nVecDejectedView;
	vecView.z += 0.47 * m_flHelpProgress;

	// Efecto de "levantarse"
	SetViewOffset( vecView );

	// ¡Arriba!
	if ( m_flHelpProgress >= 100.0f )
	{
		StopIncap();
	}
}

//====================================================================
// Pensamiento: Actualiza el nivel de cordura
//====================================================================
void CIN_Player::UpdateSanity()
{
	// Tenemos el nivel de cordura llena
	if ( m_flSanity >= 100.0f )
		return;

	if ( InCombat() )
		m_flSanity += 0.3 * gpGlobals->frametime;
	else
		m_flSanity += 3.5f * gpGlobals->frametime;
}

//====================================================================
// Agrega estres al jugador
//====================================================================
void CIN_Player::RemoveSanity( float flValue )
{
	m_flSanity -= flValue;

	if ( m_flSanity <= 0.0f )
		m_flSanity = 0.0f;
}

//====================================================================
// Actualiza la velocidad del jugador
//====================================================================
void CIN_Player::UpdateSpeed()
{	
	// Estamos incapacitados
	if ( IsIncap() )
	{
		SetMaxSpeed( 1 );
		return;
	}

	// Nos ha hecho daño algo que nos hace lentos
	if ( m_nSlowTime.HasStarted() && !m_nSlowTime.IsElapsed() )
	{
		SetMaxSpeed( 90 );
		return;
	}

	// Velocidad al caminar
	float flMax = PLAYER_WALK_SPEED;

	// Estamos corriendo
	if ( m_bIsSprinting )
	{
		flMax = PLAYER_RUN_SPEED;

		// Estamos cansados
		if ( GetStamina() <= 10.0f )
		{
			flMax -= 60.0f;
		}
	}

	// Tenemos un arma
	if ( GetActiveInWeapon() )
	{
		// Disminuimos nuestra velocidad según el peso del arma
		flMax -= GetActiveInWeapon()->GetInWpnData().m_flSpeedWeight;

		// Estamos apuntando
		if ( GetActiveInWeapon()->IsIronsighted() )
			flMax -= 35;
	}

	// Disminuimos la velocidad según nuestra salud
	if ( GetHealth() <= 50 )
		flMax -= 15;
	if ( GetHealth() <= 30 )
		flMax -= 20;
	else if ( GetHealth() <= 10 )
		flMax -= 35;

	// Poca cordura
	if ( m_flSanity <= 20.0f )
		flMax -= 10;

	// Establecemos la velocidad máxima
	SetMaxSpeed( flMax );
}

//====================================================================
// Comienza a correr
//====================================================================
void CIN_Player::StartSprint()
{
	// No te queda fuerza
	//if ( GetStamina() <= 10.0f )
		//return;

	m_bIsSprinting = true;
}

//====================================================================
// Para de correr
//====================================================================
void CIN_Player::StopSprint()
{
	m_bIsSprinting = false;
}

//====================================================================
// [Evento] Hemos saltado
//====================================================================
void CIN_Player::Jump()
{
	DoAnimationEvent( PLAYERANIMEVENT_JUMP );
}

//====================================================================
// [Evento] Hemos cambiado nuestra ubicación
//====================================================================
void CIN_Player::OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea )
{
	BaseClass::OnNavAreaChanged( enteredArea, leftArea );

	if ( !enteredArea )
		return;

	AddLastKnowArea( enteredArea );
}

//====================================================================
// Agrega la Area de Navegación a la lista de donde hemos estado
//====================================================================
void CIN_Player::AddLastKnowArea( CNavArea *pArea, bool bAround )
{
	// Reiniciamos el contador
	if ( m_iLastNavAreaIndex >= 1000 )
		m_iLastNavAreaIndex = 0;

	m_nLastNavAreas[ m_iLastNavAreaIndex ] = pArea;
	++m_iLastNavAreaIndex;

	// Agregamos 3 Areas alrededor
	// TODO: Algo mejor
	if ( bAround )
	{
		for ( int i = 0; i <= 3; ++i )
		{
			NavDirType iDirection	= (NavDirType)RandomInt(0, 3);
			CNavArea *pOther		= pArea->GetRandomAdjacentArea( iDirection );

			if ( !pOther )
				continue;

			AddLastKnowArea( pOther, false );
		}
	}
}

//====================================================================
//====================================================================
bool CIN_Player::InLastKnowAreas( CNavArea *pArea )
{
	for ( int i = 0; i <= ARRAYSIZE(m_nLastNavAreas); ++i )
	{
		if ( m_nLastNavAreas[i] == pArea )
			return true;
	}

	return false;
}

//====================================================================
// Crea el modelo en primera persona.
//====================================================================
void CIN_Player::CreateViewModel( int iIndex )
{
	// Ya ha sido creado.
	if ( GetViewModel(iIndex) )
		return;

	CPredictedViewModel *vm = (CPredictedViewModel *)CreateEntityByName("predicted_viewmodel");

	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( iIndex );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );

		m_hViewModel.Set( iIndex, vm );
	}
}

//====================================================================
// Recibe un evento de animación
//====================================================================
void CIN_Player::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->Event() == AE_FOOTSTEP_LEFT )
	{
		return;
	}

	if ( pEvent->Event() == AE_FOOTSTEP_RIGHT )
	{
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//====================================================================
//====================================================================
void CIN_Player::SetAnimation( PLAYER_ANIM playerAnim  )
{
	if ( playerAnim == PLAYER_RELOAD )
		DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
}

//====================================================================
// Ejecuta una animación en el modelo del jugador
//====================================================================
void CIN_Player::DoAnimationEvent( PlayerAnimEvent_t pEvent, int nData )
{
	m_nAnimState->DoAnimationEvent( pEvent, nData );
	TE_PlayerAnimEvent( this, pEvent, nData );
}

//====================================================================
// Comienza el modo espectador
//====================================================================
void CIN_Player::Spectate( CIN_Player *pTarget )
{
	// Ya estamos como espectador.
	if ( IsObserver() )
		return;
	
	// Seguimos vivos, hay que matarnos
	if ( IsAlive() )
		CommitSuicide();

	RemoveAllItems( true );

	if ( pTarget )
	{
		StartObserverMode( OBS_MODE_CHASE );
		SetObserverTarget( pTarget );
	}
	else
	{
		StartObserverMode( OBS_MODE_ROAMING );
	}
}

//====================================================================
// Permite ejecutar un comando no registrado
//====================================================================
bool CIN_Player::ClientCommand( const CCommand &args )
{
	//
	// Convertirse en espectador
	//
	if ( FStrEq(args[0], "spectate") )
	{
		Spectate();
		return true;
	}

	//
	// Tirar nuestra arma actual
	//
	if ( FStrEq(args[0], "dropweapon") )
	{
		Weapon_Drop( NULL, NULL , NULL );
		return true;
	}

	//
	// Reaparecer
	//
	if ( FStrEq(args[0], "respawn") )
	{
		//if ( ShouldRunRateLimitedCommand(args) )
		{
			if ( IsAlive() )
				CommitSuicide();
			
			Spawn();
		}

		return true;
	}

	//
	// Unirse a un equipo.
	//
	if ( FStrEq(args[0], "jointeam") ) 
	{
		if ( args.ArgC() < 2 )
			Warning("Player sent bad jointeam syntax \n");
 
		//if ( ShouldRunRateLimitedCommand(args) )
		{
			int iTeam = atoi(args[1]);
			ChangeTeam( iTeam );
		}

		return true;
	}

	//
	// Activar o Desactivar la mira de Hierro
	//
	if ( FStrEq(args[0], "toggle_ironsight") ) 
	{
		CBaseInWeapon *pWeapon = GetActiveInWeapon();

		// No tienes un arma
		if ( !pWeapon )
			return false;

		pWeapon->ToggleIronsight();
		return true;
	}

	//
	// Dejar un objeto del inventario
	//
	if ( FStrEq(args[0], "drop_item") ) 
	{
		// Debe haber 2 argumentos
		if ( args.ArgC() == 1 )
			return false;

		int iSlot = atoi( args[1] );

		// No tengo inventario
		if ( !GetInventory() )
			return false;

		GetInventory()->DropBySlot( iSlot );
		return true;
	}

	//
	// Usa un objeto del inventario
	//
	if ( FStrEq(args[0], "use_item") ) 
	{
		// Debe haber 2 argumentos
		if ( args.ArgC() == 1 )
			return false;

		int iSlot = atoi( args[1] );

		// No tengo inventario
		if ( !GetInventory() )
			return false;

		GetInventory()->UseItem( iSlot );
		return true;
	}

	if ( FStrEq(args[0], "drop_items") ) 
	{
		// No tengo inventario
		if ( !GetInventory() )
			return false;

		GetInventory()->DropAll();
		return true;
	}

	return BaseClass::ClientCommand( args );
}

//====================================================================
// Procesa un comando "impulse"
//====================================================================
void CIN_Player::CheatImpulseCommands( int iImpulse )
{
	switch ( iImpulse )
	{
		case 100:
		break;

		case 101:

			gEvilImpulse101 = true;

			// El traje nos proporciona el HUD
			EquipSuit();

			// Munición
			GiveAmmo( sk_max_556mm.GetInt(), "5,56mm" );
			GiveAmmo( sk_max_68mm.GetInt(), "6,8mm" );
			GiveAmmo( sk_max_9mm.GetInt(), "9mm" );
			GiveAmmo( sk_max_10mm.GetInt(), "10mm" );

			// Armas
			GiveNamedItem( "weapon_bizon" );
			GiveNamedItem( "weapon_ar15" );
			GiveNamedItem( "weapon_m4" );
			GiveNamedItem( "weapon_mp5" );
			GiveNamedItem( "weapon_mrc" );
			GiveNamedItem( "weapon_ak47" );
			//GiveNamedItem( "weapon_mp5k" );
		
			// Algo de salud.
			if ( !InRules->IsGameMode(GAME_MODE_SURVIVAL_TIME) )
			{
				if ( GetHealth() < 100 )
					TakeHealth( 25, DMG_GENERIC  );
			}

			gEvilImpulse101 = false;

		break;

		default:
			BaseClass::CheatImpulseCommands( iImpulse );
	}
}

//====================================================================
// [Evento] El Jugador esta siendo "usado"
//====================================================================
void CIN_Player::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	BaseClass::Use( pActivator, pCaller, useType, value );

	if ( !pActivator || !pActivator->IsPlayer() )
		return;

	CIN_Player *pPlayerActivator = ToInPlayer( pActivator );

	// Alguien nos esta levantando
	if ( IsIncap() )
	{
		HelpIncap( pPlayerActivator );
	}
}

//====================================================================
// Procesa los Inputs del Jugador
//====================================================================
void CIN_Player::PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	// No moverse.
	if ( m_bMovementDisabled )
	{
		ucmd->forwardmove	= 0;
		ucmd->sidemove		= 0;
		ucmd->upmove		= 0;
	}

	BaseClass::PlayerRunCommand( ucmd, moveHelper );
}

//====================================================================
// Procesa los Inputs del Jugador
//====================================================================
void CIN_Player::ProcessUsercmds( CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused )
{
	BaseClass::ProcessUsercmds( cmds, numcmds, totalcmds, dropped_packets, paused );
}

//====================================================================
// Activa la capacidad de presionar los botones
//====================================================================
/*void CIN_Player::EnableButtons( int iButtons )
{
	m_afButtonDisabled &= ~iButtons;
}

//====================================================================
// Desactiva la capacidad de presionar los botones
//====================================================================
void CIN_Player::DisableButtons( int iButtons )
{
	m_afButtonDisabled |= iButtons;
}*/

//====================================================================
// Selecciona el "info_player_start" en donde crear el
// jugador.
//====================================================================
CBaseEntity *CIN_Player::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot				= NULL;
	const char *pSpawnPointName		= "info_player_start";

	//
	// Seleccionamos un punto de aparición al azar
	//
	if ( InRules->IsSpawnMode(SPAWN_MODE_RANDOM) )
	{
		do
		{
			pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnPointName );

			if ( !pSpot )
				continue;
			
			if ( InRules->IsSpawnPointValid(pSpot, this) )
			{
				// El punto no tiene una ubicación válida
				if ( pSpot->GetLocalOrigin() == vec3_origin )
					continue;

				return pSpot;
			}
			 
		} while ( pSpot );
	}

	//
	// Cada jugador debe tener su propio punto de aparición
	//
	if ( InRules->IsSpawnMode(SPAWN_MODE_UNIQUE) )
	{
		// Ya tenemos nuestro punto de aparición
		if ( m_nSpawnSpot )
			return m_nSpawnSpot;

		do
		{
			pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnPointName );

			if ( !pSpot )
				continue;
			
			if ( InRules->IsSpawnPointValid(pSpot, this) )
			{
				// El punto no tiene una ubicación válida
				if ( pSpot->GetLocalOrigin() == vec3_origin )
					continue;

				// El punto ya lo utiliza alguien mas
				if ( !InRules->IsSpawnFree(pSpot) )
					continue;

				// Este punto lo estamos usando.
				InRules->AddSpawnSlot( pSpot );

				m_nSpawnSpot = pSpot;
				return m_nSpawnSpot;
			}
			 
		} while ( pSpot );
	}

	return gEntList.FindEntityByClassname( NULL, pSpawnPointName );
}

//====================================================================
//====================================================================
int CIN_Player::ObjectCaps()
{
	// Estamos abatidos, pueden usar E continuamente para levantarnos
	if ( IsIncap() )
		return ( BaseClass::ObjectCaps() | FCAP_CONTINUOUS_USE );

	return BaseClass::ObjectCaps();
}

//====================================================================
//====================================================================
void CIN_Player::SnapEyeAnglesZ( int iAngle )
{
	m_iEyeAngleZ = iAngle;
}