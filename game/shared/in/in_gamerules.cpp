//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_gamerules.h"

#include "ammodef.h"
#include "KeyValues.h"

#ifndef CLIENT_DLL

	#include "team.h"

	#include "in_player.h"
	#include "director.h"
	#include "players_manager.h"

	#ifdef APOCALYPSE
		#include "ap_director.h"
	#endif

	#include "iscorer.h"
	#include "roundrestart.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CInGameRules *InRules = NULL;

//=========================================================
// Comandos
//=========================================================

ConVar player_time_prone( "in_player_time_prone", "1.2", FCVAR_SERVER );
ConVar player_sprint_penalty( "in_player_sprint_penalty", "15", FCVAR_SERVER );

ConVar player_walk_speed( "in_player_walkspeed", "100", FCVAR_SERVER );
ConVar player_sprint_speed( "in_player_sprintspeed", "250", FCVAR_SERVER );
ConVar player_prone_speed( "in_player_pronespeed", "50", FCVAR_SERVER );

ConVar player_pushaway_interval( "player_pushaway_interval", "0.05", FCVAR_SERVER );

ConVar sv_showimpacts( "sv_showimpacts", "1", FCVAR_SERVER );

ConVar flashlight_weapon( "in_flashlight_weapon", "1", FCVAR_SERVER, "" );
ConVar flashlight_realistic( "in_flashlight_realistic", "0", FCVAR_SERVER, "" );

//=========================================================
// Comandos externos
//=========================================================

extern ConVar servercfgfile;
extern ConVar lservercfgfile;
extern ConVar aimcrosshair;
extern ConVar footsteps;
extern ConVar flashlight;
extern ConVar allowNPCs;

//=========================================================
// Información y Red
//=========================================================

// PROXY
LINK_ENTITY_TO_CLASS( in_gamerules, CInGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( InGameRulesProxy, DT_InGameRulesProxy );

REGISTER_GAMERULES_CLASS( CInGameRules );

// DT_InGameRules
BEGIN_NETWORK_TABLE_NOBASE( CInGameRules, DT_InGameRules )
END_NETWORK_TABLE()

//=========================================================
// Proxy
//=========================================================

#ifdef CLIENT_DLL
void RecvProxy_InGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CInGameRules *pRules = InGameRules();
	*pOut = pRules;
}

BEGIN_RECV_TABLE( CInGameRulesProxy, DT_InGameRulesProxy )
	RecvPropDataTable( "in_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_InGameRules ), RecvProxy_InGameRules )
END_RECV_TABLE()
#else
void *SendProxy_InGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CInGameRules *pRules = InGameRules();
	pRecipients->SetAllRecipients();

	return pRules;
}

BEGIN_SEND_TABLE( CInGameRulesProxy, DT_InGameRulesProxy )
	SendPropDataTable( "in_gamerules_data", 0, &REFERENCE_SEND_TABLE(DT_InGameRules), SendProxy_InGameRules )
END_SEND_TABLE()
#endif

//=========================================================
//=========================================================
void InitBodyQue()
{
}

//=========================================================
//=========================================================
int CInGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN );
	return iDamage;
}

//=========================================================
//=========================================================
int	CInGameRules::Damage_GetShouldGibCorpse( void )
{
	int iDamage = ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB );
	return iDamage;
}

//=========================================================
//=========================================================
int CInGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK );
	return iDamage;
}

//=========================================================
//=========================================================
int	CInGameRules::Damage_GetNoPhysicsForce( void )
{
	int iTimeBasedDamage = Damage_GetTimeBased();
	int iDamage = ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE );
	return iDamage;
}

//=========================================================
//=========================================================
int	CInGameRules::Damage_GetShouldNotBleed( void )
{
	int iDamage = ( DMG_POISON | DMG_ACID | DMG_GENERIC );
	return iDamage;
}

//=========================================================
//=========================================================
bool CInGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN ) ) != 0 );
}

//=========================================================
//=========================================================
bool CInGameRules::Damage_ShouldGibCorpse( int iDmgType )
{
	// Damage types that gib the corpse.
	return ( ( iDmgType & ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB ) ) != 0 );
}

//=========================================================
//=========================================================
bool CInGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK ) ) != 0 );
}

//=========================================================
//=========================================================
bool CInGameRules::Damage_NoPhysicsForce( int iDmgType )
{
	// Damage types that don't have to supply a physics force & position.
	int iTimeBasedDamage = Damage_GetTimeBased();
	return ( ( iDmgType & ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE ) ) != 0 );
}

//=========================================================
//=========================================================
bool CInGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Damage types that don't make the player bleed.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID | DMG_GENERIC ) ) != 0 );
}

//=========================================================
// Constructor
//=========================================================
CInGameRules::CInGameRules()
{
	// Estas son las reglas del juego.
	InRules = this;

	#ifndef CLIENT_DLL
		
		// Cargamos el archivo de configuración
		LoadConfig();

		// Creamos los manejadores de equipo
		for ( int i = 0; i < ARRAYSIZE( sTeamNames ); ++i )
		{
			CTeam *pTeam = dynamic_cast<CTeam *>(CreateEntityByName( "in_team_manager" ));
			pTeam->Init( sTeamNames[i], i );

			// Lo agregamos a la lista.
			g_Teams.AddToTail( pTeam );
		}

	#endif // CLIENT_DLL
}

//=========================================================
// Destructor
//=========================================================
CInGameRules::~CInGameRules( )
{
	#ifndef CLIENT_DLL
		g_Teams.Purge();
	#endif
}

//=========================================================
// Devuelve el nombre del juego.
//=========================================================
const char *CInGameRules::GetGameDescription()
{
	return GAME_DESCRIPTION;
}

//=========================================================
// Devuelve la instancia que mantiene los vectores
// de vista
//=========================================================
const CViewVectors *CInGameRules::GetViewVectors() const
{
	return &g_InViewVectors;
}

//=========================================================
// Devuelve la instancia que mantiene los vectores
// de vista
//=========================================================
const InViewVectors *CInGameRules::GetInViewVectors() const
{
	return &g_InViewVectors;
}

//=========================================================
// Devuelve si habrá colisión entre el grupo 0 y 1
//=========================================================
bool CInGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	// Don't stand on COLLISION_GROUP_WEAPON
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT && collisionGroup1 == COLLISION_GROUP_WEAPON )
		return false;

	// No deben colisionar entre ellos
	if ( collisionGroup0 == COLLISION_GROUP_NOT_BETWEEN_THEM && collisionGroup1 == COLLISION_GROUP_NOT_BETWEEN_THEM )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_NPC && collisionGroup1 == COLLISION_GROUP_NPC )
		return true;
	
	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

#ifndef CLIENT_DLL

// Ayudante de voz
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

//=========================================================
// Carga el archivo de configuración
//=========================================================
void CInGameRules::LoadConfig()
{
	// Estamos en un Servidor dedicado
	if ( engine->IsDedicatedServer() )
	{
		const char *cfgfile = servercfgfile.GetString();

		if ( cfgfile && cfgfile[0] )
		{
			char szCommand[256];

			Msg( "Executing dedicated server config file\n" );
			Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
			engine->ServerCommand( szCommand );
		}
	}

	// Alguien esta Hosteando su propio Servidor
	else
	{
		const char *cfgfile = lservercfgfile.GetString();

		if ( cfgfile && cfgfile[0] )
		{
			char szCommand[256];

			Msg( "Executing listen server config file\n" );
			Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
			engine->ServerCommand( szCommand );
		}
	}
}

//=========================================================
// Guarda en caché objetos necesarios
//=========================================================
void CInGameRules::Precache()
{
	BaseClass::Precache();
}

//=========================================================
// Pensamiento.
//=========================================================
void CInGameRules::Think()
{
	BaseClass::Think();
}

//=========================================================
// Limpia el mapa y vuelve a crear todas las entidades
//=========================================================
void CInGameRules::CleanUpMap()
{
	// Se lo pasamos a su respectiva interfaz
	roundRestart->CleanUpMap();

	IGameEvent *pEvent = gameeventmanager->CreateEvent( "game_round_restart" );

	if ( pEvent )
	{
		gameeventmanager->FireEvent( pEvent );
	}
}

//=========================================================
// [Evento] Se ha conectado un cliente
//=========================================================
bool CInGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	GetVoiceGameMgr()->ClientConnected( pEntity );

	return true;
}

//=========================================================
// [Evento] Un cliente ha escrito un comando no registrado
//=========================================================
bool CInGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	// El código base ha hecho su trabajo
	if ( BaseClass::ClientCommand(pEdict, args) )
		return true;

	CIN_Player *pPlayer = ToInPlayer( pEdict );

	if ( !pPlayer )
		return false;

	// Pasamos el comando al método del Jugador
	if ( pPlayer->ClientCommand(args) )
		return true;

	return false;
}

//=========================================================
// [Evento] Se ha conectado un cliente
//=========================================================
void CInGameRules::ClientDisconnected( edict_t *pClient )
{
	if ( !pClient )
		return;

	CIN_Player *pPlayer = (CIN_Player *)CBaseEntity::Instance( pClient );

	if ( !pPlayer )
		return;

	FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

	// Destruimos
	ClientDestroy( pPlayer );

	// Establecemos que se ha desconectado
	pPlayer->SetConnected( PlayerDisconnected );
}

//=========================================================
// Destruye los objetos de un Jugador
//=========================================================
void CInGameRules::ClientDestroy( CBasePlayer *pPlayer )
{
	// Le quitamos todos sus objetos
	pPlayer->RemoveAllItems( true );

	// Destruimos sus vistas en primera persona
	pPlayer->DestroyViewModels();
}

//=========================================================
// Inicia y configura la tabla de relaciones para los NPC
//=========================================================
void CInGameRules::InitDefaultAIRelationships()
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// First initialize table so we can report missing relationships
	for ( i = 0; i < LAST_SHARED_ENTITY_CLASS; i++ )
	{
		for ( j = 0; j < LAST_SHARED_ENTITY_CLASS; j++ )
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
		}
	}

	// ------------------------------------------------------------
	//	> CLASS_INFECTED
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_INFECTED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_BOSS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_EARTH_FAUNA, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_BOSS
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_BOSS, CLASS_BOSS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_BOSS, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_BOSS, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_BOSS, CLASS_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_BOSS, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_EARTH_FAUNA
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_EARTH_FAUNA, CLASS_EARTH_FAUNA, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_EARTH_FAUNA, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_EARTH_FAUNA, CLASS_PLAYER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_EARTH_FAUNA, CLASS_INFECTED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_EARTH_FAUNA, CLASS_BOSS, D_FR, 0);
}

//=========================================================
// Devuelve la relación entre Jugadores
//=========================================================
int CInGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// Son del mismo equipo
	if ( pPlayer->GetTeamNumber() == pTarget->GetTeamNumber() )
		return GR_TEAMMATE;

	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
void CInGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, bool bIgnoreWorld )
{
		CBaseEntity *pEntity = NULL;
		trace_t		tr;
		float		flAdjustedDamage, falloff;
		Vector		vecSpot;
		Vector		vecToTarget;
		Vector		vecEndPos;

		Vector vecSrc = vecSrcIn;

		if ( flRadius )
			falloff = info.GetDamage() / flRadius;
		else
			falloff = 1.0;

		int bInWater = UTIL_PointContents ( vecSrc, MASK_WATER ) ? true : false;
		
		vecSrc.z += 1;// in case grenade is lying on the ground

		// iterate on all entities in the vicinity.
		for ( CEntitySphereQuery sphere( vecSrc, flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
		{
			if ( pEntity->m_takedamage != DAMAGE_NO )
			{
				// UNDONE: this should check a damage mask, not an ignore
				if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
				{// houndeyes don't hurt other houndeyes with their attack
					continue;
				}

				// blast's don't tavel into or out of water
				if (bInWater && pEntity->GetWaterLevel() == 0)
					continue;
				if (!bInWater && pEntity->GetWaterLevel() == 3)
					continue;

				// radius damage can only be blocked by the world
				vecSpot = pEntity->BodyTarget( vecSrc );



				bool bHit = false;

				if( bIgnoreWorld )
				{
					vecEndPos = vecSpot;
					bHit = true;
				}
				else
				{
					UTIL_TraceLine( vecSrc, vecSpot, MASK_SOLID_BRUSHONLY, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

					if (tr.startsolid)
					{
						// if we're stuck inside them, fixup the position and distance
						tr.endpos = vecSrc;
						tr.fraction = 0.0;
					}

					vecEndPos = tr.endpos;

					if( tr.fraction == 1.0 || tr.m_pEnt == pEntity )
					{
						bHit = true;
					}
				}

				if ( bHit )
				{
					// the explosion can 'see' this entity, so hurt them!
					//vecToTarget = ( vecSrc - vecEndPos );
					vecToTarget = ( vecEndPos - vecSrc );

					// decrease damage for an ent that's farther from the bomb.
					flAdjustedDamage = vecToTarget.Length() * falloff;
					flAdjustedDamage = info.GetDamage() - flAdjustedDamage;
				
					if ( flAdjustedDamage > 0 )
					{
						CTakeDamageInfo adjustedInfo = info;
						adjustedInfo.SetDamage( flAdjustedDamage );

						Vector dir = vecToTarget;
						VectorNormalize( dir );

						// If we don't have a damage force, manufacture one
						if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
						{
							CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc, 1.5	/* explosion scale! */ );
						}
						else
						{
							// Assume the force passed in is the maximum force. Decay it based on falloff.
							float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
							adjustedInfo.SetDamageForce( dir * flForce );
							adjustedInfo.SetDamagePosition( vecSrc );
						}

						pEntity->TakeDamage( adjustedInfo );
			
						// Now hit all triggers along the way that respond to damage... 
						pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, vecEndPos, dir );
					}
				}
			}
		}
}

//=========================================================
// Devuelve si el daño hará al jugador más lento
//=========================================================
bool CInGameRules::DamageCanMakeSlow( const CTakeDamageInfo &info )
{
	#ifdef APOCALYPSE
		// El daño por los infectados
		if ( info.GetAttacker()->Classify() == CLASS_INFECTED || info.GetAttacker()->IsPlayer() )
			return true;
	#endif

	return false;
}

//=========================================================
//=========================================================
bool CInGameRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{
	if ( !pPlayer->Weapon_CanSwitchTo( pWeapon ) )
	{
		// Can't switch weapons for some reason.
		return false;
	}

	if ( !pPlayer->GetActiveWeapon() )
	{
		// Player doesn't have an active item, might as well switch.
		return true;
	}

	if ( !pWeapon->AllowsAutoSwitchTo() )
	{
		// The given weapon should not be auto switched to from another weapon.
		return false;
	}

	if ( !pPlayer->GetActiveWeapon()->AllowsAutoSwitchFrom() )
	{
		// The active weapon does not allow autoswitching away from it.
		return false;
	}

	if ( pWeapon->GetWeight() > pPlayer->GetActiveWeapon()->GetWeight() )
	{
		return true;
	}

	return false;
}

//=========================================================
// Devuelve la cantidad de daño por la caida
//=========================================================
float CInGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
		return 0.0f;

	pPlayer->m_Local.m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_Local.m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

//=========================================================
// Deuelve si la victima puede recibir daño
//=========================================================
bool CInGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	return true;
}

//=========================================================
// Deuelve si la victima (Jugador) puede recibir daño
//=========================================================
bool CInGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	return true;
}

//=========================================================
// Deuelve si el Jugador puede ser abatido
//=========================================================
bool CInGameRules::FPlayerCanDejected( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
		return false;

	if ( info.GetDamageType() & (DMG_CRUSH|DMG_FALL|DMG_DISSOLVE) )
		return false;

	return true;
}

//=========================================================
// Devuelve si el Jugador puede reproducir el sonido de muerte
//=========================================================
bool CInGameRules::FCanPlayDeathSound( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & (DMG_CRUSH|DMG_FALL|DMG_DISSOLVE) )
		return false;

	return true;
}

//=========================================================
//=========================================================
void CInGameRules::PlayerThink( CBasePlayer *pPlayer )
{
}

//=========================================================
// [Evento] Un Jugador se ha creado
//=========================================================
void CInGameRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	// Por ahora no necesitamos un traje de protección, así que se lo damos al comenzar
	pPlayer->EquipSuit();

	CBaseEntity *pEquipEnt = NULL;

	// Las entidades "game_player_equip" son paquetes para comenzar una partida.
	do
	{
		pEquipEnt = gEntList.FindEntityByClassname( pEquipEnt, "game_player_equip" );

		if ( !pEquipEnt )
			continue;

		pEquipEnt->Touch( pPlayer );

	} while ( pEquipEnt );
}

//=========================================================
// [Evento] Un Jugador se ha sido eliminado
//=========================================================
void CInGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Notificación de Muerte
	DeathNotice( pVictim, info );

	// Definimos el inflictor y el atacante
	CBaseEntity *pInflictor		= info.GetInflictor();
	CBaseEntity *pKiller		= info.GetAttacker();
	CBasePlayer *pScorer		= GetDeathScorer( pKiller, pInflictor, pVictim );

	// Has muerto
	pVictim->IncrementDeathCount( 1 );

	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

	// Notificamos al Director
	if ( HasDirector() )
		Director->OnPlayerKilled( pVictim );

	// Nos has eliminado
	pKiller->Event_KilledOther( pVictim, info );

	if ( pScorer )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pScorer->IncrementFragCount( IPointsForKill( pScorer, pVictim ) );
			
		// Allow the scorer to immediately paint a decal
		pScorer->AllowImmediateDecalPainting();

		FireTargets( "game_playerkill", pScorer, pScorer, USE_TOGGLE, 0 );
	}
	else
	{  
		if ( UseSuicidePenalty() )
		{
			// Players lose a frag for letting the world kill them			
			pVictim->IncrementFragCount( -1 );
		}					
	}
}

//=========================================================
// Devuelve si el Jugador tiene la posibilidad de reaparecer
//=========================================================
bool CInGameRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return true;
}

//=========================================================
// Devuelve si el Jugador puede reaparecer en este momento
//=========================================================
bool CInGameRules::FPlayerCanRespawnNow( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	// No hay un Director
	if ( !HasDirector() )
		return true;

	// Somos un Bot o se debe reaparecer al instante
	if ( respawn_afterdeath.GetBool() || pPlayer->IsBot() )
		return true;

	// Ha pasado el tiempo suficiente
	//if ( gpGlobals->curtime > pPlayer->GetDeathTime() + wait_deathcam.GetFloat() )
		//return true;

	return false;
}

//=========================================================
// 
//=========================================================
float CInGameRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->curtime;
}

//=========================================================
//=========================================================
void CInGameRules::InitHUD( CBasePlayer *pPlayer )
{
}

//=========================================================
//=========================================================
bool CInGameRules::AllowAutoTargetCrosshair()
{
	return ( aimcrosshair.GetInt() != 0 );
}

//=========================================================
// Puntos por eliminar al Jugador
//=========================================================
int CInGameRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

//=========================================================
//=========================================================
CBasePlayer *CInGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor )
{
	if ( pKiller)
	{
			if ( pKiller->Classify() == CLASS_PLAYER )
				return (CBasePlayer*)pKiller;

			// Killing entity might be specifying a scorer player
			IScorer *pScorerInterface = dynamic_cast<IScorer*>( pKiller );
			if ( pScorerInterface )
			{
				CBasePlayer *pPlayer = pScorerInterface->GetScorer();
				if ( pPlayer )
					return pPlayer;
			}

			// Inflicting entity might be specifying a scoring player
			pScorerInterface = dynamic_cast<IScorer*>( pInflictor );
			if ( pScorerInterface )
			{
				CBasePlayer *pPlayer = pScorerInterface->GetScorer();
				if ( pPlayer )
					return pPlayer;
			}
	}

	return NULL;
}

//=========================================================
//=========================================================
CBasePlayer *CInGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	return GetDeathScorer( pKiller, pInflictor );
}

//=========================================================
// Notificación de Muerte de algún Jugador
//=========================================================
void CInGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Herramienta asesina
	const char *pKillerWeapon = "el mundo";

	// Definimos el inflictor y el atacante
	CBaseEntity *pInflictor		= info.GetInflictor();
	CBaseEntity *pKiller		= info.GetAttacker();
	CBasePlayer *pScorer		= GetDeathScorer( pKiller, pInflictor, pVictim );

	// Daño personalizado
	if ( info.GetDamageCustom() )
	{
		pKillerWeapon = GetDamageCustomString( info );
	}
	else
	{
		// Lo ha asesinado un Jugador
		if ( pScorer )
		{
			if ( pInflictor )
			{
				// El Inflictor es el asesino, usaremos el nombre de su arma
				if ( pInflictor == pScorer )
				{
					if ( pScorer->GetActiveWeapon() )
					{
						pKillerWeapon = pScorer->GetActiveWeapon()->GetDeathNoticeName();
					}
				}
				else
				{
					pKillerWeapon = STRING( pInflictor->m_iClassname );
				}
			}
			else
			{
				if ( pKiller->IsPlayer() )
					pKillerWeapon = pKiller->GetPlayerName();
				else
					pKillerWeapon = STRING( pKiller->m_iClassname );
			}
		}
		else
		{
			pKillerWeapon = STRING( pInflictor->m_iClassname );
		}
	}

	// Quitamos el prefijo de la herramienta
	if ( strncmp( pKillerWeapon, "weapon_", 7 ) == 0 )
	{
		pKillerWeapon += 7;
	}
	else if ( strncmp( pKillerWeapon, "npc_", 8 ) == 0 )
	{
		pKillerWeapon += 8;
	}
	else if ( strncmp( pKillerWeapon, "func_", 5 ) == 0 )
	{
		pKillerWeapon += 5;
	}

	// TMP
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs("%s ha sido asesinado por %s", pVictim->GetPlayerName(), pKillerWeapon) );
}

//=========================================================
// Devuelve si el Jugador puede obtener el arma
//=========================================================
bool CInGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
{
	if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
		return false;

	for ( int i = 0 ; i < pPlayer->WeaponCount() ; i++ )
	{
		// Ya tienes esta arma
		if ( pPlayer->GetWeapon(i) == pItem )
		{
			return false;
		}
	}

	return BaseClass::CanHavePlayerItem( pPlayer, pItem );
}

//=========================================================
// Devuelve si el Jugador puede obtener el objeto
//=========================================================
bool CInGameRules::CanHaveItem( CBasePlayer *pPlayer, CBaseEntity *pItem )
{
	if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
		return false;

	return true;
}

//=========================================================
// [Evento] El Jugador ha obtenido un objeto
//=========================================================
void CInGameRules::PlayerGotItem( CBasePlayer *pPlayer, CBaseEntity *pItem )
{
}

//=========================================================
// [Evento] El Jugador ha obtenido munición
//=========================================================
void CInGameRules::PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
int CInGameRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
// Devuelve si el Objeto puede reaparecer
//=========================================================
int CInGameRules::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}

//=========================================================
// Devuelve si la entidad puede aparecer
//=========================================================
bool CInGameRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return true;
}

//=========================================================
//=========================================================
CBaseCombatWeapon *CInGameRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	CBaseCombatWeapon *pCheck;
		CBaseCombatWeapon *pBest;// this will be used in the event that we don't find a weapon in the same category.

		int iCurrentWeight = -1;
		int iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
		pBest = NULL;

		// If I have a weapon, make sure I'm allowed to holster it
		if ( pCurrentWeapon )
		{
			if ( !pCurrentWeapon->AllowsAutoSwitchFrom() || !pCurrentWeapon->CanHolster() )
			{
				// Either this weapon doesn't allow autoswitching away from it or I
				// can't put this weapon away right now, so I can't switch.
				return NULL;
			}

			iCurrentWeight = pCurrentWeapon->GetWeight();
		}

		for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
		{
			pCheck = pPlayer->GetWeapon( i );
			if ( !pCheck )
				continue;

			// If we have an active weapon and this weapon doesn't allow autoswitching away
			// from another weapon, skip it.
			if ( pCurrentWeapon && !pCheck->AllowsAutoSwitchTo() )
				continue;

			if ( pCheck->GetWeight() > -1 && pCheck->GetWeight() == iCurrentWeight && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category. 
				if ( pCheck->HasAnyAmmo() )
				{
					if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
					{
						return pCheck;
					}
				}
			}
			else if ( pCheck->GetWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				//Msg( "Considering %s\n", STRING( pCheck->GetClassname() );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if ( pCheck->HasAnyAmmo() )
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->GetWeight();
					pBest = pCheck;
				}
			}
		}

		// if we make it here, we've checked all the weapons and found no useable 
		// weapon in the same catagory as the current weapon. 
		
		// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
		// at least get the crowbar, but ya never know.
		return pBest;
}

//=========================================================
// Devuelve que sucederá con las armas en los Jugadores muertos
//=========================================================
int CInGameRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	// Soltamos nuestra arma activa
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
// Devuelve que sucederá con la munición en los Jugadores muertos
//=========================================================
int CInGameRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	 return GR_PLR_DROP_AMMO_ACTIVE;
}

//=========================================================
// Devuelve la entidad donde se creará al Jugador
//=========================================================
CBaseEntity *CInGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	return BaseClass::GetPlayerSpawnSpot( pPlayer );
}

//=========================================================
// Devuelve
//=========================================================
bool CInGameRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
{
	return ( PlayerRelationship(pListener, pSpeaker) == GR_TEAMMATE );
}

//=========================================================
// Devuelve si se puede reproducir sonidos de pasos
//=========================================================
bool CInGameRules::PlayFootstepSounds( CBasePlayer *pPlayer )
{
	return footsteps.GetBool();
}

//=========================================================
// Devuelve si se puede encender la linterna
//=========================================================
bool CInGameRules::FAllowFlashlight()
{
	return flashlight.GetBool();
}

//=========================================================
// Devuelve si los NPC estan permitidos
//=========================================================
bool CInGameRules::FAllowNPCs()
{
	return allowNPCs.GetBool();
}

//=========================================================
// Devuelve si el Spawn no esta siendo ocupado por otro jugador
//=========================================================
bool CInGameRules::IsSpawnFree( CBaseEntity *pSpawn )
{
	int iSlot = hSpawnSlots.Find( pSpawn->entindex() );
	return ( iSlot == -1 );
}

//=========================================================
// Agrega el Spawn a la lista de "utilizados"
//=========================================================
void CInGameRules::AddSpawnSlot( CBaseEntity *pSpawn )
{
	// El Spawn ya ha sido registrado
	if ( !IsSpawnFree(pSpawn) )
		return;

	hSpawnSlots.AddToTail( pSpawn->entindex() );
}

//=========================================================
// Elimina el Spawn de la lista de "utilizados"
//=========================================================
void CInGameRules::RemoveSpawnSlot( CBaseEntity *pSpawn )
{
	// El Spawn no ha sido registrado
	if ( IsSpawnFree(pSpawn) )
		return;

	hSpawnSlots.FindAndRemove( pSpawn->entindex() );
}

//=========================================================
// Devuelve si el daño producido permitirá la animación de muerte
//=========================================================
bool CInGameRules::CanPlayDeathAnim( CBaseEntity *pEntity, const CTakeDamageInfo &info )
{
	// Es un Jugador
	if ( pEntity->IsPlayer() )
	{
		CIN_Player *pPlayer = ToInPlayer( pEntity );

		if ( pPlayer )
		{
			// Esta incapacitado
			// NOTE: Hasta que no tengamos animaciones propias
			if ( pPlayer->IsIncap() )
				return false;
		}
	}

	// No esta en el suelo
	if ( !(pEntity->GetFlags() & FL_ONGROUND) )
		return false;

	return CanPlayDeathAnim( info );
}

//=========================================================
// Devuelve si el daño producido permitirá la animación de muerte
//=========================================================
bool CInGameRules::CanPlayDeathAnim( const CTakeDamageInfo &info )
{
	// Solo en daño por bala o garra
	if ( info.GetDamageType() & (DMG_BULLET | DMG_SLASH) )
		return true;

	return false;
}

#endif // 