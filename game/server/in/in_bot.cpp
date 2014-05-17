//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_bot.h"

#include "weapon_inbase.h"

#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"

#include "players_manager.h"
#include "in_gamerules.h"

#include "nav.h"
#include "nav_area.h"

#include "in_utils.h"

extern CBasePlayer *BotPutInServer( edict_t *pEdict, const char *pPlayerName );

//====================================================================
// Comandos
//====================================================================

ConVar bot_frozen( "bot_frozen", "0" );
ConVar bot_crouch( "bot_crouch", "0" );
ConVar bot_flashlight( "bot_flashlight", "0" );
ConVar bot_mimic( "bot_mimic", "0" );
ConVar bot_aim_player( "bot_aim_player", "1" );
ConVar bot_debug_aim( "bot_debug_aim", "0" );

ConVar bot_aim_z( "bot_aim_z", "0" );
ConVar bot_notarget( "bot_notarget", "0" );
ConVar bot_god( "bot_god", "0" );
ConVar bot_dont_attack( "bot_dont_attack", "0" );

//====================================================================
// Macros
//====================================================================

#define BOT_FROZEN		bot_frozen.GetBool()
#define BOT_CROUCH		bot_crouch.GetBool()
#define BOT_FLASHLIGHT	bot_flashlight.GetBool()
#define BOT_MIMIC		bot_mimic.GetInt()
#define BOT_AIM_PLAYER	bot_aim_player.GetBool()
#define BOT_DEBUG_AIM	bot_debug_aim.GetBool()

#define BOT_NOTARGET	bot_notarget.GetBool()
#define BOT_GOD			bot_god.GetBool()
#define BOT_DONT_ATTACK bot_dont_attack.GetBool()

#define RANDOM_AIM_INTERVAL		gpGlobals->curtime + RandomInt( 3, 10 )
#define NEW_ENEMY_INTERVAL		gpGlobals->curtime + 0.3f;

//====================================================================
// Información
//====================================================================

static int g_botID = 0;

LINK_ENTITY_TO_CLASS( in_bot, CIN_Bot );

//====================================================================
//====================================================================

const char *m_nBotsNames[] =
{
	"Alfonso",
	"Abigaíl-21",
	"Abigaíl-33",
	"Abigaíl-3001",
	"Chell",
	"Rodrigo",
	"Gordon",
	"Freeman",
	"Luis",
	"Forever Alone",
	"Troll",
	"Kolesias123 Fake",
	"La simulación es una mentira"
};

//====================================================================
// Crea un BOT y lo prepara para el Juego
//====================================================================
CBasePlayer *CreateBot( bool bFrozen )
{
	// Nombre del BOT
	const char *botName = m_nBotsNames[ RandomInt(0, ARRAYSIZE(m_nBotsNames)-1) ];

	// Creamos la instancia del BOT
	ClientPutInServerOverride( BotPutInServer );
	edict_t *botEdict = engine->CreateFakeClient( botName );
	ClientPutInServerOverride( NULL );

	if ( !botEdict )
	{
		Warning( "[CreateBot] Ha ocurrido un problema al crear un BOT \n" );
		return NULL;
	}

	// Obtenemos al BOT
	CIN_Bot *pPlayer = (CIN_Bot *)CBaseEntity::Instance( botEdict );

	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

	// Empezaras congelado
	if ( bFrozen )
		pPlayer->AddFlag( EFL_BOT_FROZEN );

	pPlayer->InitialSpawn();
	pPlayer->Spawn();

	g_botID++;
	return pPlayer;
}

//====================================================================
// Ejecuta el pensamiento de todos los Bots
//====================================================================
void Bot_RunAll()
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Bot *pPlayer = ToInBot( UTIL_PlayerByIndex(i) );

		// No existe o no es un BOT
		if ( !pPlayer )
			continue;

		pPlayer->BotThink();
	}
}

//====================================================================
// Crea un Jugador bot
//====================================================================
CBasePlayer *CIN_Bot::CreatePlayer( const char *pClassName, edict_t *pEdict, const char *pPlayerName )
{
	CBasePlayer::s_PlayerEdict = pEdict;

	CIN_Bot *pPlayer = ( CIN_Bot * )CreateEntityByName( pClassName );
	pPlayer->SetPlayerName( pPlayerName );

	return pPlayer;
}

//====================================================================
// Creación en el mundo
//====================================================================
void CIN_Bot::Spawn()
{
	BaseClass::Spawn();

	// TEMP: Nos damos todo
	CheatImpulseCommands( 101 );

	// Por ahora no apuntamos a ningún lado
	RestoreAim();

	// Información
	m_flNextDirectionAim	= gpGlobals->curtime;
	m_flNextIdleCycle		= gpGlobals->curtime;

	m_vecLastEnemyAim	= vec3_invalid;
	m_nAimingEntity		= NULL;
	m_vecLastEntityPosition.Invalidate();

	m_bNeedAim		= false;
	m_flEndAimTime	= 0.0f;

	m_nDontRandomAim = false;

	m_nEnemy			= NULL;
	m_flNextEnemyCheck	= NEW_ENEMY_INTERVAL;
	m_nEnemyLostMemory.Invalidate();

	m_angLastViewAngles = GetLocalAngles();
}

//====================================================================
// Ejecuta los comandos
//====================================================================
void CIN_Bot::RunPlayerMove( CUserCmd &cmd, float frametime )
{
	// Store off the globals.. they're gonna get whacked
	float flOldFrametime	= gpGlobals->frametime;
	float flOldCurtime		= gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	SetTimeBase( flTimeBase );

	MoveHelperServer()->SetHost( this );
	PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime	= flOldFrametime;
	gpGlobals->curtime		= flOldCurtime;
}

//====================================================================
// Pensamiento para el Bot
//====================================================================
void CIN_Bot::BotThink()
{
	// Seguimos siendo un Bot
	AddFlag( FL_FAKECLIENT );

	// TEMP: Si hemos muerto, debemos reaparecer
	if ( !IsAlive() )
	{
		Spawn();
		return;
	}

	// Estamos congelados o no estamos vivos
	if ( BOT_FROZEN )
		return;

	if ( BOT_MIMIC > 0 )
	{
		// Estamos haciendo mimica de algún Jugador
		if ( BotThinkMimic(BOT_MIMIC) )
			return;
	}

	if ( BOT_GOD )
		AddFlag( FL_GODMODE );
	else
		RemoveFlag( FL_GODMODE );

	if ( BOT_NOTARGET )
		AddFlag( FL_NOTARGET );
	else
		RemoveFlag( FL_NOTARGET );

	// Instancia para los comandos
	CUserCmd cmd;
	Q_memset( &cmd, 0, sizeof(cmd) );

	// Configuración inicial
	cmd.viewangles	= m_angLastViewAngles;
	cmd.upmove		= 0;
	cmd.impulse		= 0;

	// Enemigos
	UpdateEnemy();

	// Nos movemos
	if ( !UpdateMovement( cmd ) )
	{
		// Si no, ¿que hacemos mientras?
		UpdateIdle( cmd );

		// Volteamos nuestra mirada
		UpdateDirection( cmd );
	}

	// Acciones
	UpdateActions( cmd );

	// Saltamos
	UpdateJump( cmd );

	// Hacia donde apuntar
	UpdateAim( cmd );

	// Ataque
	UpdateAttack( cmd );

	// Actualizamos el arma
	UpdateWeapon( cmd );

	// Verificamos si necesitamos la linterna
	CheckFlashlight();

	// Fix up the m_fEffects flags
	PostClientMessagesSent();

	// Simulamos los comandos
	RunPlayerMove( cmd, gpGlobals->frametime );

	m_angLastViewAngles = cmd.viewangles;
}

//====================================================================
// Pensamiento para realizar mimica de algún Jugador
//====================================================================
bool CIN_Bot::BotThinkMimic( int iPlayer )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayer );

	// El Jugador no existe
	if ( !pPlayer )
		return false;

	// No ha enviado ningún comando
	if ( !pPlayer->GetLastUserCommand() )
		return false;

	// Preparamos para enviar comandos
	CUserCmd cmd;
	Q_memset( &cmd, 0, sizeof(cmd) );

	cmd = *pPlayer->GetLastUserCommand();

	// Debemos agacharnos
	if ( bot_crouch.GetBool() )
		cmd.buttons |= IN_DUCK;

	RunPlayerMove( cmd, gpGlobals->frametime );
	return true;
}

//====================================================================
// Devuelve el Vector para apuntar hacia una entidad
//====================================================================
void CIN_Bot::GetAimCenter( CBaseEntity *pEntity, Vector &vecAim )
{
	vecAim = pEntity->edict()->GetCollideable()->GetCollisionOrigin();
}

//====================================================================
// Devuelve si el Bot choca contra una pared en la
// distancia indicada
//====================================================================
bool CIN_Bot::HitsWall( float flDistance, int iHeight )
{
	Vector vecSrc, vecEnd, vecForward;
	AngleVectors( GetAbsAngles(), &vecForward );

	vecSrc = GetAbsOrigin() + Vector( 0, 0, iHeight );
	vecEnd = vecSrc + vecForward * flDistance;

	trace_t trace;
	UTIL_TraceHull( vecSrc, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &trace );

	// Hemos dado con una entidad
	if ( trace.m_pEnt )
	{
		// Los Jugadores o NPC no deben ser considerados aquí
		if ( trace.m_pEnt->IsPlayer() || trace.m_pEnt->IsNPC() )
			return false;
	}

	return ( trace.fraction == 1.0 ) ? false : true;
}

//====================================================================
// Devuelve si puede disparar
//====================================================================
bool CIN_Bot::CanFireMyWeapon()
{
	CBaseInWeapon *pWeapon = GetActiveInWeapon();

	// No tienes arma
	if ( !pWeapon )
		return false;

	// No debemos disparar
	if ( BOT_DONT_ATTACK )
		return false;

	if ( pWeapon->UsesClipsForAmmo1() )
	{
		int ammoClip1		= pWeapon->Clip1();
		int maxClip1		= pWeapon->GetMaxClip1();
		int maxAmmoClip1	= GetAmmoCount( pWeapon->GetPrimaryAmmoType() );

		// Sin munición en el arma (Recargando...)
		if ( ammoClip1 <= 0 )
			return false;

		// No tenemos munición
		if ( ammoClip1 <= 0 && maxAmmoClip1 <= 0 )
			return false;
	}

	if ( GetEnemy() )
	{
		// Aún no tenemos en la mira al enemigo
		//if ( !m_nEnemyAimed )
			//return false;
	}

	return true;
}

//====================================================================
// Apunta hacia una entidad
//====================================================================
void CIN_Bot::AimTo( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return;

	// Obtenemos la dirección del enemigo para apuntar
	Vector vecAim;
	GetAimCenter( pEntity, vecAim );

	m_nAimingEntity = pEntity;
	AimTo( vecAim );
}

ConVar bot_test_x( "bot_test_x", "0" );
ConVar bot_test_y( "bot_test_y", "0" );

//====================================================================
// Apunta hacia una dirección
//====================================================================
void CIN_Bot::AimTo( Vector &vecAim )
{
	// Disminumos lo de nuestra cabeza
	Vector vecDestination( vecAim );
	vecDestination -= EarPosition();

	if ( GetEnemy() && !m_nAimingEntity.Get() )
		m_nAimingEntity = GetEnemy();

	// Estamos apuntando a una entidad
	if ( m_nAimingEntity.Get() )
	{
		// Aumentamos la altura
		float heightUp		= (m_nAimingEntity.Get()->EarPosition().z / 2);
		vecDestination.z	+= heightUp;

		vecDestination.x += bot_test_x.GetInt();
		vecDestination.y += bot_test_y.GetInt();

		// Es una entidad que se mueve, predecimos donde estará para apuntar mejor
		if (  m_nAimingEntity.Get()->MyCombatCharacterPointer() || Utils::IsPhysicsObject(m_nAimingEntity.Get()) )
		{
			if ( m_vecLastEntityPosition.IsValid() )
			{
				Vector vecPredict( m_nAimingEntity.Get()->EarPosition() );
				vecPredict -= m_vecLastEntityPosition;

				float framesLeft = ( GetEndAimTime() * 60.0f );
				vecPredict *= framesLeft;

				vecDestination += vecPredict;
			}

			// Última ubicación de la entidad
			m_vecLastEntityPosition = m_nAimingEntity.Get()->EarPosition();
		}
	}
	else
	{
		m_vecLastEntityPosition.Invalidate();
	}

	// Transformamos Vector a QAngle
	QAngle angDestination;
	VectorAngles( vecDestination, angDestination );

	AimTo( angDestination );
}

//====================================================================
// Apunta hacia una dirección
//====================================================================
void CIN_Bot::AimTo( QAngle &angAim )
{
	// Estamos apuntando al mismo lugar
	if ( m_angAimTo == angAim || m_bNeedAim )
		return;

	m_angAimTo		= angAim;
	m_nAimingEntity = NULL;
	m_bNeedAim		= false;
	m_nEnemyAimed	= false;

	if ( angAim != vec3_angle )
	{
		m_bNeedAim		= true;
		m_flEndAimTime	= gpGlobals->curtime + GetEndAimTime();
	}
}

//====================================================================
// Establece que no se debe apuntar a nadie en especifico
//====================================================================
void CIN_Bot::RestoreAim()
{
	QAngle angInvalid = vec3_angle;
	AimTo( angInvalid );
}

//====================================================================
// Devuelve el tiempo en el que se debe apuntar
//====================================================================
float CIN_Bot::GetEndAimTime()
{
	static const int iAngDiff		= 40; // 30 degrees to make time difference in bot's aim.
	static const int iEndAimSize	= 180/iAngDiff + 1;
	static const float aEndAimTime[LAST_AIM_SPEED][iEndAimSize] = 
	{
		{ 0.50f, 0.70f, 0.90f, 1.10f, 1.30f }, // AIM_SPEED_VERYLOW
		{ 0.50f, 0.60f, 0.70f, 0.80f, 0.90f }, // AIM_SPEED_LOW
		{ 0.50f, 0.55f, 0.60f, 0.65f, 0.70f }, // AIM_SPEED_NORMAL
		{ 0.40f, 0.45f, 0.50f, 0.55f, 0.60f }, // AIM_SPEED_FAST
		{ 0.25f, 0.30f, 0.35f, 0.40f, 0.45f }, // AIM_SPEED_VERYFAST
	};

	int aimSpeed = AIM_SPEED_VERYLOW;

	// ¡Estamos en combate!
	if ( InCombat() )
		aimSpeed = AIM_SPEED_NORMAL;

	// Bajo ataque
	if ( IsUnderAttack() || GetEnemy() )
		aimSpeed = AIM_SPEED_VERYFAST;

	QAngle angNeeded( m_angAimTo );
	Utils::GetAngleDifference( GetLocalAngles(), angNeeded, angNeeded );

	int iPitch = abs( (int)angNeeded.x );
	int iYaw = abs( (int)angNeeded.y );

	iPitch /= iAngDiff;
	iYaw /= iAngDiff;

	if ( iPitch < iYaw)
		iPitch = iYaw; // iPitch = MAX2( iPitch, iYaw );

	float aimTime = aEndAimTime[aimSpeed][iPitch];
	return aimTime;
}

//====================================================================
// Actualiza hacia donde apuntar
//====================================================================
void CIN_Bot::UpdateAim( CUserCmd &cmd )
{
	// No queremos apuntar a ningún lado
	if ( m_angAimTo == vec3_angle )
		return;

	QAngle angAim( m_angAimTo );

	// Simulamos apuntar como humano
	if ( m_bNeedAim )
	{
		// Obtenemos los frames que faltan para terminar de apuntar
		float timeLeft = m_flEndAimTime - gpGlobals->curtime;
		int framesLeft = (int)( 60.0f * timeLeft );

		if ( BOT_DEBUG_AIM )
		{
			DevMsg( "[CIN_Bot::UpdateAim] %s: m_bNeedAim - m_flEndAimTime: %f - timeLeft: %f - framesLeft: %i \n", GetPlayerName(), m_flEndAimTime, timeLeft, framesLeft );
		}

		// Aún faltan frames, apuntamos poco a poco... (humanamente)
		if ( framesLeft > 0 )
		{
			Utils::GetAngleDifference( cmd.viewangles, angAim, angAim );
			angAim /= framesLeft;

			cmd.viewangles += angAim;
		}
		else
		{
			// Hemos terminado de apuntar
			m_bNeedAim = false;

			if ( GetEnemy() )
				m_nEnemyAimed = true;
		}
	}

	//
	Utils::DeNormalizeAngle( cmd.viewangles.x );
	Utils::DeNormalizeAngle( cmd.viewangles.y );
}

//====================================================================
// Actualiza y procesa el movimiento
// Por ahora: Al azar
//====================================================================
bool CIN_Bot::UpdateMovement( CUserCmd &cmd )
{
	// Estamos congelados, no podemos movernos
	if ( IsEFlagSet(EFL_BOT_FROZEN) )
		return false;

	// El Jugador más cercano
	float nearPlayer = 0.0f;
	m_nNearPlayer = PlysManager->GetNear( GetLocalOrigin(), nearPlayer, MyDefaultTeam(), this );

	// Velocidad inicial
	float flSpeed = 600.0;

	// Tenemos un enemigo
	if ( GetEnemy() )
	{
		// No nos movemos
		flSpeed = 0;

		float enemyDistance = GetEnemyDistance();

		// El enemigo esta muy cerca, nos movemos hacia atras
		// TODO: Algo más avanzado, aquí solo simulamos que se esta apuntando hacia donde esta el enemigo
		if ( enemyDistance <= 250 )
		{
			flSpeed = -600.0;

			// ¡Muy cerca! Debemos correr
			if ( enemyDistance <= 150 )
			{
				flSpeed = -800.0;
				cmd.buttons |= IN_SPEED;
			}
		}
	}
	else
	{
		// Hay un Jugador cerca de nosotros, no hay que movernos
		if ( nearPlayer <= 80.0f )
		{
			UpdatePlayerActions( cmd );
			flSpeed = 0;
		}
		else
		{
			// Apuntemos al Jugador
			if ( BOT_AIM_PLAYER )
			{
				AimTo( m_nNearPlayer );
			}

			// El Jugador se ha alejado un poco, debemos correr
			if ( nearPlayer >= 200.0f )
			{
				cmd.buttons |= IN_SPEED;
			}
		}
	}

	cmd.forwardmove	= flSpeed;
}

//====================================================================
// Actualiza algunas acciones (Presionar algunas teclas)
//====================================================================
void CIN_Bot::UpdateActions( CUserCmd &cmd )
{
	// Tenemos un enemigo y un arma
	if ( GetEnemy() && GetActiveInWeapon() )
	{
		float enemyDistance		= GetEnemyDistance();
		CBaseInWeapon *pWeapon	= GetActiveInWeapon();

		if ( enemyDistance > 350 )
		{
			// Nos agachamos para apuntar mejor
			cmd.buttons |= IN_DUCK;
		}

		// Esta un poco lejos
		if ( enemyDistance > 200 )
		{
			// Apuntamos
			if ( !pWeapon->IsIronsighted() )
				pWeapon->EnableIronsight();
		}
		else
		{
			// Dejamos de apuntar
			if ( pWeapon->IsIronsighted() )
				pWeapon->DisableIronsight();
		}
	}
}

//====================================================================
// Actualiza la dirección a donde voltear
//====================================================================
void CIN_Bot::UpdateDirection( CUserCmd &cmd )
{
	// No mientras tengamos un enemigo
	if ( GetEnemy() )
		return;

	// Estamos concentrado en algo, no debemos mirar a otro lado
	if ( m_nDontRandomAim )
		return;

	// Aún no debemos voltear
	if ( gpGlobals->curtime < m_flNextDirectionAim )
		return;

	// Aveces solo miraremos al Jugador más cercano
	if ( RandomInt(0, 5) == 5 )
	{
		AimTo( m_nNearPlayer );
		m_flNextDirectionAim = RANDOM_AIM_INTERVAL;
		return;
	}

	float angleDelta	= RandomInt( -180, 180 );
	QAngle angle		= GetLocalAngles();

	angle.x = RandomInt(-30, 30);
	angle.z = cmd.viewangles.z;
	angle.y = angleDelta;

	// Nos volteamos
	AimTo( angle );
	m_flNextDirectionAim = RANDOM_AIM_INTERVAL;
}

//====================================================================
// Verifica si es necesario saltar
//====================================================================
void CIN_Bot::UpdateJump( CUserCmd &cmd )
{
	// Algo nos bloquea solo en los pies
	if ( HitsWall(20, 13) && !HitsWall(20, 36) )
		cmd.buttons |= IN_JUMP;
}

//====================================================================
// Actualiza el arma y realiza verificaciones
//====================================================================
void CIN_Bot::UpdateWeapon( CUserCmd &cmd )
{
	CBaseInWeapon *pWeapon = GetActiveInWeapon();

	// No tenemos arma
	if ( !pWeapon )
		return;

	int ammoClip1		= pWeapon->Clip1();
	int maxClip1		= pWeapon->GetMaxClip1();
	int maxAmmoClip1	= GetAmmoCount( pWeapon->GetPrimaryAmmoType() );

	// Sin munición, debemos cambiar de arma
	if ( ammoClip1 <= 0 && maxAmmoClip1 <= 0 )
	{
		SwitchToNextBestWeapon( pWeapon );
		return;
	}

	// Hora de recargar
	if ( ammoClip1 <= 0 )
		cmd.buttons |= IN_RELOAD;

	if ( !GetEnemy() && !InCombat() )
	{
		// Recargamos para no necesitar después
		if ( ammoClip1 < maxClip1 )
			cmd.buttons |= IN_RELOAD;
	}
}

//====================================================================
//====================================================================
void CIN_Bot::UpdatePlayerActions( CUserCmd &cmd )
{
	CIN_Player *pPlayer = ToInPlayer( m_nNearPlayer );

	if ( !pPlayer )
		return;

	if ( m_nDontRandomAim )
		m_nDontRandomAim = false;

	// Esta incapacitado, ayudarlo
	if ( pPlayer->IsIncap() )
	{
		// Apuntamos al Jugador
		m_nDontRandomAim = true;
		AimTo( m_nNearPlayer);

		// Lo ayudamos
		cmd.buttons |= IN_USE;
	}
}

//====================================================================
// Actualiza el ataque
//====================================================================
void CIN_Bot::UpdateAttack( CUserCmd &cmd )
{
	// No podemos disparar
	if ( !CanFireMyWeapon() )
		return;

	// No tenemos enemigo o lo hemos perdido
	if ( !GetEnemy() || EnemyIsLost() )
		return;

	// Disparamos
	if ( RandomFloat(0.0, 1.0) > 0.5 )
		cmd.buttons |= IN_ATTACK;
}

//====================================================================
// Actualiza los enemigos
//====================================================================
void CIN_Bot::UpdateEnemy()
{
	CBaseEntity *pEnemy = GetEnemy();

	// Aún no tenemos un enemigo
	if ( !pEnemy )
	{
		RestoreAim();

		// Aún no toca
		if ( gpGlobals->curtime < m_flNextEnemyCheck )
			return;

		// Invalidamos
		if ( m_nEnemyLostMemory.HasStarted() )
			m_nEnemyLostMemory.Invalidate();

		pEnemy = FindEnemy();

		// ¡Hemos encontrado un enemigo!
		if ( pEnemy )
		{
			SetEnemy( pEnemy );
		}

		m_flNextEnemyCheck = NEW_ENEMY_INTERVAL;
		return;
	}

	NDebugOverlay::Box( pEnemy->GetAbsOrigin(), -Vector(5, 5, 5), Vector(5, 5, 5), 255, 255, 0, 20, 0.2f );
	
	// Ya no podemos ver a nuestro enemigo
	if ( !FVisible(pEnemy) )
	{
		// Esperamos unos segundos
		if ( !m_nEnemyLostMemory.HasStarted() )
			m_nEnemyLostMemory.Start( RandomInt(2, 5) );
	}
	else
	{
		if ( m_nEnemyLostMemory.HasStarted() )
			m_nEnemyLostMemory.Invalidate();

		// Seguimos actualizando su ubicación
		GetAimCenter( pEnemy, m_vecLastEnemyAim );
	}

	// Apuntamos hacia la última ubicación conocida
	AimTo( m_vecLastEnemyAim );

	// Hemos perdido a nuestro enemigo
	if ( CanForgotEnemy() || !pEnemy->IsAlive() )
	{
		SetEnemy( NULL );
	}
}

//====================================================================
//====================================================================
void CIN_Bot::CheckFlashlight()
{
	// Nos obligan a usar la linterna
	if ( bot_flashlight.GetBool() )
	{
		FlashlightTurnOn();
		return;
	}

	CNavArea *pArea = GetLastKnownArea();

	// No hemos visitado ningún NavArea
	if ( !pArea )
		return;

	// Obtenemos la intensidad de luz
	float flLight = pArea->GetLightIntensity();

	// Esta oscuro, necesitamos la linterna
	if ( flLight <= 0.5f )
	{
		FlashlightTurnOn();
	}
	else
	{
		FlashlightTurnOff();
	}
}

//====================================================================
// Devuelve si la entidad puede ser el nuevo candidato a enemigo
//====================================================================
bool CIN_Bot::CheckEnemy( CBaseEntity *pEntity, CBaseEntity *pCandidate )
{
	if ( !pEntity )
		return false;

	// No puede ser mi enemigo
	if ( !CanBeEnemy(pEntity) )
		return false;

	float flDistance = pEntity->GetAbsOrigin().DistTo( GetAbsOrigin() );

	// Esta muy lejos
	if ( flDistance > 1000 )
		return false;

	// Ya tenemos a un candidato
	if ( pCandidate )
	{
		float flCandDistance = pCandidate->GetAbsOrigin().DistTo( GetAbsOrigin() );

		// Esta más lejos que el candidato
		if ( flDistance > flCandDistance )
			return false;
	}

	return true;
}

//====================================================================
// Encuentra y establece un nuevo enemigo
//====================================================================
CBaseEntity *CIN_Bot::FindEnemy( bool bCheckVisible )
{
	CBaseEntity *pEnemy		= NULL;
	CBaseEntity *pNewEnemy	= NULL;

	//
	// NPC
	//
	do
	{
		pNewEnemy = gEntList.FindEntityByClassname( pNewEnemy, "npc_*" );

		if ( !CheckEnemy(pNewEnemy, pEnemy) )
			continue;

		// El nuevo candidato
		pEnemy = pNewEnemy;

	} while ( pNewEnemy );

	//
	// Jugadores
	//
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CIN_Player *pPlayer = PlysManager->Get(i);

		if ( !CheckEnemy(pPlayer, pEnemy) )
			continue;

		// El nuevo candidato
		pEnemy = pPlayer;
	}

	return pEnemy;
}

//====================================================================
// Devuelve si la entidad puede ser mi enemigo
//====================================================================
bool CIN_Bot::CanBeEnemy( CBaseEntity *pEntity, bool bCheckVisible )
{
	// No esta vivo
	if ( !pEntity->IsAlive() )
		return false;

	// Es invisible
	if ( pEntity->IsEffectActive(EF_NODRAW) )
		return false;

	if ( bCheckVisible )
	{
		// No lo podemos ver
		if ( !FInViewCone(pEntity->WorldSpaceCenter()) )
			return false;

		// No lo podemos ver
		if ( !FVisible(pEntity->WorldSpaceCenter()) )
			return false;
	}

	// Es mi amigo
	if ( InRules->PlayerRelationship(this, pEntity) == GR_TEAMMATE )
		return false;

	return true;
}

//====================================================================
// Devuelve si se puede olvidar al enemigo actual
//====================================================================
bool CIN_Bot::CanForgotEnemy()
{
	// No tenemos ningún enemigo
	if ( !GetEnemy() )
		return false;

	// Tiempo de espera superado
	if ( m_nEnemyLostMemory.HasStarted() && m_nEnemyLostMemory.IsElapsed() )
		return true;

	/*CBaseEntity *pOtherEnemy = FindEnemy();

	// Otro enemigo esta más cerca
	if ( pOtherEnemy != GetEnemy() )
		return true;*/

	return false;
}

//====================================================================
// Devuelve si hemos perdido al enemigo
//====================================================================
bool CIN_Bot::EnemyIsLost()
{
	// Ni siquiera tenemos un enemigo
	if ( !GetEnemy() )
		return false;

	return ( m_nEnemyLostMemory.HasStarted() );
}

//====================================================================
//====================================================================
float CIN_Bot::GetEnemyDistance()
{
	// Ni siquiera tenemos un enemigo
	if ( !GetEnemy() )
		return -1;

	return GetEnemy()->GetAbsOrigin().DistTo( GetAbsOrigin() );
}

//====================================================================
// [Evento] El jugador ha recibido daño pero sigue vivo
//====================================================================
int CIN_Bot::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CBaseEntity *pAttacker = info.GetAttacker();

	if ( pAttacker && CanBeEnemy(pAttacker, false) )
	{
		SetEnemy( pAttacker );
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//====================================================================
// Establece nuestro enemigo
//====================================================================
void CIN_Bot::SetEnemy( CBaseEntity *pEnemy )
{
	m_nEnemy = pEnemy;

	// Apuntamos hacia el enemigo
	if ( pEnemy )
		AimTo( pEnemy );
}

//====================================================================
//====================================================================
CON_COMMAND_F( bot_add, "Agrega un(os) bot(s)", FCVAR_SERVER )
{
	// Look at -count.
	int count = args.FindArgInt( "-count", 1 );
	count = clamp( count, 1, 16 );

	// Look at -frozen.
	bool bFrozen = !!args.FindArg( "-frozen" );
		
	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{
		CreateBot( bFrozen );
	}
}