//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ai_base_infected.h"
#include "in_gamerules.h"

#include "func_break.h"
#include "director.h"

#include "in_utils.h"
#include "doors.h"
#include "BasePropDoor.h"
#include "func_breakablesurf.h"

#include "ilagcompensationmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool RagdollManager_SaveImportant( CAI_BaseNPC *pNPC );

//====================================================================
// Información
//====================================================================

//LINK_ENTITY_TO_CLASS( npc_basemelee, CBaseInfected );

BEGIN_DATADESC( CBaseInfected )
	DEFINE_FIELD( m_nMoveXPoseParam, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMoveYPoseParam, FIELD_INTEGER ),
	DEFINE_FIELD( m_nLeanYawPoseParam, FIELD_INTEGER ),

	DEFINE_FIELD( m_nPotentialEnemy, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_nLastEnemy, FIELD_CLASSPTR ),

	DEFINE_FIELD( m_nBlockingEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flEntBashYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_bFalling, FIELD_BOOLEAN ),
END_DATADESC()

//====================================================================
// Tareas
//====================================================================
enum
{
	TASK_MELEE_YAW_TO_OBJ = LAST_SHARED_TASK,
	TASK_MELEE_ATTACK_OBJ
};

//====================================================================
// Tareas programadas
//====================================================================
enum
{
	SCHED_MELEE_BASH_OBJ = LAST_SHARED_SCHEDULE,
};

//====================================================================
// Condiciones
//====================================================================
enum
{
	COND_MELEE_BLOCKED_BY_OBJ = LAST_SHARED_CONDITION,
	COND_MELEE_OBJ_VANISH,
};

//====================================================================
// Comandos
//====================================================================

ConVar ai_melee_trace_attack( "ai_melee_trace_attack", "1", FCVAR_CHEAT, "Muestra" );

//====================================================================
// Creación en el mundo
//====================================================================
void CBaseInfected::Spawn()
{
	CB_MeleeCharacter::SetOuter( this );
}

//====================================================================
// Pensamiento
//====================================================================
void CBaseInfected::NPCThink()
{
	BaseClass::NPCThink();

	if ( IsAlive() )
	{
		UpdateFall();

		// ¡No sabemos nadar!
		// TODO: Animación de "ahogo"
		if ( GetWaterLevel() >= WL_Waist )
			SetHealth( 0 );
	}
}

//====================================================================
// Crea los comportamientos necesarios
//====================================================================
bool CBaseInfected::CreateBehaviors()
{
	// Comportamiento para trepar paredes
	AddBehavior( &m_nClimbBehavior );

	return BaseClass::CreateBehaviors();
}

//====================================================================
// Actualiza la animación de "caida"
//====================================================================
void CBaseInfected::UpdateFall()
{
	// Cuando un NPC salta para llegar a su destino el código original de la IA cambia su tipo de navegación
	// a MOVETYPE_FLY, aquí verificamos que no sea un salto programado si no un salto por caida natural
	if ( GetMoveType() == MOVETYPE_FLY )
		return;

	Vector vecUp;
	GetVectors( NULL, NULL, &vecUp );

	trace_t	tr;
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + -(vecUp) * 50, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
	
	// No estamos en el suelo, ¡estamos cayendo!
	if ( !(GetFlags() & FL_ONGROUND) )
	{
		SetGravity( 0.5 );

		// No estamos a una caida considerable
		if ( tr.fraction != 1.0 )
			return;

		if ( HasActivity(ACT_GLIDE) )
			SetActivity( ACT_GLIDE );

		else if ( HasActivity(ACT_JUMP) )
			SetActivity( ACT_JUMP );

		GetNavigator()->StopMoving();
		GetMotor()->SetMoveInterval( 0 );

		m_bFalling = true;
	}

	// Estamos en el suelo
	else
	{
		// Estabamos cayendo, hemos aterrizado
		if ( m_bFalling )
		{
			float flTime = GetGroundChangeTime();
			AddStepDiscontinuity( flTime, GetAbsOrigin(), GetAbsAngles() );

			if ( HasActivity(ACT_LAND) )
				SetActivity( ACT_LAND );

			GetMotor()->SetMoveInterval( 0 );
			SetGravity( 1.0f );

			m_bFalling = false;
		}
	}
}

//====================================================================
// Verifica si se puede trepar sobre una pared
//====================================================================
void CBaseInfected::UpdateClimb()
{
	// Ya estamos trepando
	if ( m_nClimbBehavior.IsClimbing() )
		return;

	// Parece que hay una pared enfrente que puedo trepar
	if ( m_nClimbBehavior.CanClimb() )
	{
		// Cancelamos cualquier acción y pasamos el funcionamiento al CAI_ClimbBehavior
		ClearSchedule( "Start Climb" );
		SetPrimaryBehavior( &m_nClimbBehavior );
	}
	else
	{
		SetPrimaryBehavior( NULL );
	}
}

//====================================================================
// Devuelve el genero del NPC
//====================================================================
const char *CBaseInfected::GetGender()
{
	// Modelo femenino
	if ( Q_stristr(STRING(GetModelName()), "female") )
		return "Female";
	
	return "Male";
}

//====================================================================
//====================================================================
void CBaseInfected::SetupGlobalModelData()
{
	m_nMoveXPoseParam	= LookupPoseParameter("move_x");
	m_nMoveYPoseParam	= LookupPoseParameter("move_y");
	m_nLeanYawPoseParam = LookupPoseParameter("lean_yaw");
}

//====================================================================
// Crea una parte del cuerpo
//====================================================================
CGib *CBaseInfected::CreateGib( const char *pModelName, const Vector &vecOrigin, const Vector &vecDir, int iNum, int iSkin )
{
	CGib *pGib = CREATE_ENTITY( CGib, "gib" );

	pGib->Spawn( pModelName, 10 );
	pGib->m_material	= matFlesh;
	pGib->m_nBody		= iNum;
	pGib->m_nSkin		= iSkin;

	pGib->InitGib( this, RandomInt(100, 200), RandomInt(200, 350) );
	pGib->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE_DEBRIS );

	Vector vecNewOrigin = vecOrigin;
	vecNewOrigin.x += RandomFloat( -3.0, 3.0 );
	vecNewOrigin.y += RandomFloat( -3.0, 3.0 );
	vecNewOrigin.z += RandomFloat( -3.0, 3.0 );

	pGib->SetAbsOrigin( vecNewOrigin );
	return pGib;
}

//====================================================================
// Devuelve si hay una entidad obstruyendo el camino
//====================================================================
bool CBaseInfected::HasObstruction()
{
	CBaseEntity *pBlockingEnt = GetBlockingEnt();

	// No hay ninguna obstrucción
	if ( !pBlockingEnt )
		return false;

	CBreakableSurface *pSurf = dynamic_cast<CBreakableSurface *>( pBlockingEnt );

	// Es una superficie de cristal y ya esta rota
	if ( pSurf && pSurf->m_bIsBroken )
		return false;

	CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>( pBlockingEnt );

	// Es una puerta pero ya se ha abierto
	if ( pPropDoor && pPropDoor->IsDoorOpen() )
		return false;
	
	CBaseDoor *pDoor = dynamic_cast<CBaseDoor *>( pBlockingEnt );

	// Es una puerta pero ya se ha abierto
	if ( pDoor && (pDoor->m_toggle_state == TS_AT_TOP || pDoor->m_toggle_state == TS_GOING_UP)  )
		return false;

	return true;
}

//====================================================================
// Procesa el siguiente movimiento
//====================================================================
bool CBaseInfected::OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	CBaseEntity *pObstruction = pMoveGoal->directTrace.pObstruction;

	// No hay ninguna entidad bloqueando nuestro camino
	if ( !pObstruction )
		return false;

	// Solo nos interesan objetos
	if ( pObstruction->IsPlayer() || pObstruction->IsNPC() || pObstruction->MyCombatCharacterPointer() )
		return false;
	
	// Algo esta obstruyendo nuestro camino (y podemos romperlo/quitarlo)
	if ( OnObstruction(pMoveGoal, pObstruction, distClear, pResult) )
	{
		// Mientras no estemos muy cerca, marquemos el camino como "libre"
		if ( distClear < 0.1 )
		{
			*pResult = ( pObstruction->IsWorld() ) ? AIMR_BLOCKED_WORLD : AIMR_BLOCKED_ENTITY;
		}
		else
		{
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_OK;
		}

		// Estamos muy cerca, hora de romper el bloqueo
		if ( IsMoveBlocked(*pResult) )
		{
			m_nBlockingEnt = pObstruction;
			m_flEntBashYaw = -1;

			if ( pMoveGoal->directTrace.vHitNormal != vec3_origin )
				m_flEntBashYaw = UTIL_VecToYaw( pMoveGoal->directTrace.vHitNormal * -1 );				
		}

		return true;
	}

	return false;
}

//====================================================================
// Devuelve si es posible romper la obstrucción
//====================================================================
bool CBaseInfected::OnObstruction( AILocalMoveGoal_t *pMoveGoal, CBaseEntity *pEntity, float distClear, AIMoveResult_t *pResult )
{
	CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>( pEntity );

	//
	// Una puerta
	//
	if ( pPropDoor && OnUpcomingPropDoor(pMoveGoal, pPropDoor, distClear, pResult) )
		return true;

	//
	// Una superficie rompible, dejemos que el NPC lo rompa al momento de llegar
	//
	if ( Utils::IsBreakableSurf(pEntity) )
		return true;

	return false;
}

//====================================================================
// Hay una puerta obstruyendo nuestro camino
//====================================================================
bool CBaseInfected::OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, float distClear, AIMoveResult_t *pResult )
{
	if ( BaseClass::OnObstructingDoor(pMoveGoal, pDoor, distClear, pResult) )
	{
		// Establecemos la puerta como nuestro destino a romper
		if ( IsMoveBlocked(*pResult) && pMoveGoal->directTrace.vHitNormal != vec3_origin )
		{
			m_nBlockingEnt = pDoor;
			m_flEntBashYaw = UTIL_VecToYaw( pMoveGoal->directTrace.vHitNormal * -1 );
		}

		return true;
	}

	return false;
}

//====================================================================
// Hay una puerta obstruyendo nuestro camino
//====================================================================
bool CBaseInfected::OnUpcomingPropDoor( AILocalMoveGoal_t *pMoveGoal, CBasePropDoor *pDoor, float distClear, AIMoveResult_t *pResult )
{
	// El NPC no fue capaz de abrir la puerta
	if ( !BaseClass::OnUpcomingPropDoor(pMoveGoal, pDoor, distClear, pResult) )
	{
		return true;
	}

	return false;
}

//====================================================================
// Devuelve si el NPC choca contra una pared
//====================================================================
bool CBaseInfected::HitsWall( float flDistance, int iHeight  )
{
	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward, vecSrc, vecEnd;
	GetVectors( &vecForward, NULL, NULL );

	vecSrc = WorldSpaceCenter() + Vector( 0, 0, iHeight );
	vecEnd = vecSrc + vecForward * flDistance;

	trace_t	tr;
	AI_TraceHull( vecSrc, vecEnd, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	return ( tr.fraction == 1.0 ) ? false : true;
}

//====================================================================
// Establece la salud dependiendo del nivel de dificultad
//====================================================================
void CBaseInfected::SetIdealHealth( int iHealth, int iMulti )
{
	iHealth += ( InRules->GetSkillLevel() * iMulti );

	SetMaxHealth( iHealth );
	SetHealth( iHealth );
}

//====================================================================
// [Evento] Hemos recibido daño
//====================================================================
int CBaseInfected::OnTakeDamage( const CTakeDamageInfo &info )
{
	//
	// FIXME: Una partida Multiplayer no muestra rastros de sangre
	//
	if ( gpGlobals->maxClients > 1 && info.GetDamagePosition() != vec3_origin )
	{
		Vector vecDir		= info.GetDamageForce();
		Vector vecOrigin	= info.GetDamagePosition();

		UTIL_BloodSpray( vecOrigin, vecDir, BloodColor(), 5, FX_BLOODSPRAY_GORE );
		UTIL_BloodDrips( vecOrigin, vecDir, BloodColor(), 5 );

		// Pintamos sangre en las paredes
		for ( int i = 0 ; i < 5; i++ )
		{
			vecDir.x += random->RandomFloat( -0.9f, 0.9f );
			vecDir.y += random->RandomFloat( -0.9f, 0.9f );
			vecDir.z += random->RandomFloat( -0.9f, 0.9f );
 
			trace_t tr;
			UTIL_TraceLine( vecOrigin, vecOrigin + ( vecDir * 200.0f ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction != 1.0 )
				UTIL_BloodDecalTrace( &tr, BloodColor() );
		}
	}

	return BaseClass::OnTakeDamage( info );
}

//=========================================================
// Devuelve si es conveniente atacar
//=========================================================
int CBaseInfected::MeleeAttack1Conditions( float flDot, float flDist )
{
	m_nPotentialEnemy = NULL;

	// No tenemos un enemigo
	if ( !GetEnemy() )
		return COND_NONE;

	// Aún no podemos atacar
	if ( !CanAttack() )
		return COND_NONE;

	// Hull
	Vector vecMins	= GetHullMins();
	Vector vecMaxs	= GetHullMaxs();
	vecMins.z		= vecMins.x;
	vecMaxs.z		= vecMaxs.x;

	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );

	// Trazamos una línea y verificamos si hemos dado con algo
	trace_t	tr;
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + vecForward * GetMeleeDistance(), vecMins, vecMaxs, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	// Hemos dado con algo
	if ( tr.fraction != 1.0 && tr.m_pEnt )
	{
		m_nPotentialEnemy = tr.m_pEnt;

		// Podemos atacar a esto
		if ( CanAttackEntity(tr.m_pEnt) )
			return COND_CAN_MELEE_ATTACK1;
	}

	// Esta muy lejos
	return COND_TOO_FAR_TO_ATTACK;
}

//====================================================================
// [Evento] Hemos muerto
//====================================================================
void CBaseInfected::Event_Killed( const CTakeDamageInfo &info )
{
	// Si no podemos reproducir una animación de muerte entonces
	// usemos el código original de muerte
	if ( !CanPlayDeathAnim(info) )
	{
		BaseClass::Event_Killed( info );
		return;
	}

	//
	// Con esto reproducimos una animación de muerte, esperamos a que termine y entonces
	// creamos el cadaver del NPC (y eliminamos al NPC)
	//

	// Estamos muriendo...
	m_lifeState = LIFE_DYING;

	// Animación de muerte
	SetActivity( ACT_DIESIMPLE );
	StudioFrameAdvance();

	PreDeath( info );

	// Ahora pensamos con DeathThink
	SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink( &CBaseInfected::DeathThink );
}

//====================================================================
// Prepara todo para la muerte del NPC
//====================================================================
void CBaseInfected::PreDeath( const CTakeDamageInfo &info )
{
	Wake( false );
	CleanupOnDeath( info.GetAttacker() );

	// Paramos los sonidos y usamos nuestro sonido de muerte
	StopLoopingSounds();
	DeathSound( info );

	if ( ( GetFlags() & FL_NPC ) && ( ShouldGib( info ) == false ) )
		SetTouch( NULL );
	
	/// CBaseCombatCharacter
	EmitSound( "BaseCombatCharacter.StopWeaponSounds" );

	// Tell my killer that he got me!
	if( info.GetAttacker() )
	{
		info.GetAttacker()->Event_KilledOther(this, info);
		//g_EventQueue.AddEvent( info.GetAttacker(), "KilledNPC", 0.3, this, this );
	}

	m_OnDeath.FireOutput( info.GetAttacker(), this );
	SendOnKilledGameEvent( info );
	/// CBaseCombatCharacter

	if ( m_bFadeCorpse )
		m_bImportanRagdoll = RagdollManager_SaveImportant( this );

	// Make sure this condition is fired too (OnTakeDamage breaks out before this happens on death)
	SetCondition( COND_LIGHT_DAMAGE );
	SetIdealState( NPC_STATE_DEAD );

	trace_t bloodTrace;
	AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 256 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &bloodTrace);
	
	// Dibujamos sangre en el suelo
	if ( bloodTrace.fraction < 1.0f )
	{		
		UTIL_BloodDecalTrace( &bloodTrace, BloodColor() );
	}

	// Daño que nos ha matado
	m_pLastDamage = info;
}

//====================================================================
// Pensamiento al estar muerto
//====================================================================
void CBaseInfected::DeathThink()
{
	// Volvemos a pensar en .1s
	SetNextThink( gpGlobals->curtime + 0.1f );

	// Seguimos reproduciendo la animación
	AutoMovement();
	StudioFrameAdvance();
	DispatchAnimEvents( this );

	// Nuestra animación de muerte no ha terminado
	if ( !IsActivityFinished() )
		return;

	StopAnimation();

	if ( CanBecomeRagdoll() )
	{
		// Nos convertimos en un cadaver
		BecomeRagdollOnClient( m_pLastDamage.GetDamageForce() );
	}

	// Hemos muerto, ahora si
	m_lifeState	= LIFE_DEAD;

	// Nos eliminamos en 1s
	SetThink( &CAI_BaseNPC::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1.0f );
}

//====================================================================
// Devuelve si el NPC puede usar una animación al morir
//====================================================================
bool CBaseInfected::CanPlayDeathAnim( const CTakeDamageInfo &info )
{
	return InRules->CanPlayDeathAnim(this, info);
}

//=========================================================
// Devuelve la velocidad ideal
//=========================================================
float CBaseInfected::GetIdealSpeed() const
{
	float flSpeed = BaseClass::GetIdealSpeed();

	if ( flSpeed <= 0 ) 
		flSpeed = 6.0f;

	return flSpeed;
}

//=========================================================
//=========================================================
float CBaseInfected::GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence )
{
	float flSpeed = BaseClass::GetSequenceGroundSpeed( pStudioHdr, iSequence );

	if( flSpeed <= 0 ) 
		flSpeed = 6.0f;

	return flSpeed;
}

//=========================================================
// Sobrescribe el funcionamiento del Movimiento
// Permite usar move_x y move_y
//=========================================================
bool CBaseInfected::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	// required movement direction
	float flMoveYaw = UTIL_VecToYaw( move.dir );

	// FIXME: move this up to navigator so that path goals can ignore these overrides.
	Vector dir;
	float flInfluence = GetFacingDirection( dir );
	dir = move.facing * (1 - flInfluence) + dir * flInfluence;
	VectorNormalize( dir );

	// ideal facing direction
	float idealYaw = UTIL_AngleMod( UTIL_VecToYaw( dir ) );
		
	// FIXME: facing has important max velocity issues
	GetMotor()->SetIdealYawAndUpdate( idealYaw );	

	// find movement direction to compensate for not being turned far enough
	float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y );

	// Setup the 9-way blend parameters based on our speed and direction.
	Vector2D vCurMovePose( 0, 0 );

	vCurMovePose.x = cos( DEG2RAD( flDiff ) ) * 1.0f; //flPlaybackRate;
	vCurMovePose.y = -sin( DEG2RAD( flDiff ) ) * 1.0f; //flPlaybackRate;

	SetPoseParameter( m_nMoveXPoseParam, vCurMovePose.x );
	SetPoseParameter( m_nMoveYPoseParam, vCurMovePose.y );

	if ( m_nLeanYawPoseParam >= 0 )
	{
		float targetLean	= GetPoseParameter( m_nLeanYawPoseParam ) * 30.0f;
		float curLean		= GetPoseParameter( m_nLeanYawPoseParam );

		if( curLean < targetLean )
			curLean += MIN(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);
		else
			curLean -= MIN(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);
		SetPoseParameter( m_nLeanYawPoseParam, curLean );
	}

	return true;
}

//=========================================================
// Aplica las condiciones dependiendo del entorno
//=========================================================
void CBaseInfected::GatherConditions()
{
	BaseClass::GatherConditions();

	static int conditionsToClear[] = 
	{
		COND_MELEE_BLOCKED_BY_OBJ,
		COND_MELEE_OBJ_VANISH,
	};

	ClearConditions( conditionsToClear, ARRAYSIZE(conditionsToClear) );

	// 
	// Nuestro camino (ya) no esta obstruido
	//
	if ( !HasObstruction() )
	{
		SetCondition( COND_MELEE_OBJ_VANISH );

		// Restauramos
		if ( GetBlockingEnt() )
			m_nBlockingEnt = NULL;
	}
	else
	{
		// Sigue obstruido
		SetCondition( COND_MELEE_BLOCKED_BY_OBJ );
		return;
	}
}

//=========================================================
// Comienza la ejecución de una tarea
//=========================================================
void CBaseInfected::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		//
		// Giramos hacia la entidad que bloquea nuestro camino
		//
		case TASK_MELEE_YAW_TO_OBJ:
		{
			if ( GetBlockingEnt() && m_flEntBashYaw > -1 )
			{
				GetMotor()->SetIdealYaw( m_flEntBashYaw );
			}

			TaskComplete();
			break;
		}

#ifdef APOCALYPSE

		//
		// Encontrar una ruta hacia el enemigo
		//
		case TASK_GET_CHASE_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = GetEnemy();

			// No tenemos ningún enemigo
			if ( !pEnemy )
			{
				TaskFail( FAIL_NO_ROUTE );
				return;
			}

			sharedtasks_e iTask = TASK_GET_PATH_TO_ENEMY;

			// No estamos en el Climax, seremos realistas
			if ( !Director->IsStatus(ST_FINALE) )
			{
				// Si estamos viendo a nuestro enemigo vayamos directamente hacia donde esta, 
				// de otra forma solo hacia el último lugar donde lo vimos

				if ( IsInFieldOfView(pEnemy) && FVisible(pEnemy) )
					iTask = TASK_GET_PATH_TO_ENEMY;
				else
					iTask = TASK_GET_PATH_TO_ENEMY_LKP;
			}

			ChainStartTask( iTask );

			// No hemos podido ir
			if ( !TaskIsComplete() && !HasCondition(COND_TASK_FAILED) )
				TaskFail( FAIL_NO_ROUTE );

			break;
		}

#endif // APOCALYPSE

		default:
		{
			BaseClass::StartTask( pTask );
			break;
		}
	}
}

//=========================================================
// Ejecuta una tarea y espera a que termine
//=========================================================
void CBaseInfected::RunTask( const Task_t *pTask )
{
	BaseClass::RunTask( pTask );
}

//=========================================================
// Selecciona una nueva tarea por fallar la anterior
//=========================================================
int CBaseInfected::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	// Nuestro camino ha sido obstruido por algo
	if ( HasCondition(COND_MELEE_BLOCKED_BY_OBJ) && GetBlockingEnt() && failedSchedule != SCHED_MELEE_BASH_OBJ )
	{
		ClearCondition( COND_MELEE_BLOCKED_BY_OBJ );
		return SCHED_MELEE_BASH_OBJ;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//=========================================================
// Selecciona una tarea
//=========================================================
int CBaseInfected::SelectSchedule()
{
	return BaseClass::SelectSchedule();
}

//=========================================================
// Traduce una tarea registrada a otra
//=========================================================
int CBaseInfected::TranslateSchedule( int scheduleType )
{
	switch ( scheduleType )
	{
		
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
// Inteligencia artificial personalizada
//=========================================================
AI_BEGIN_CUSTOM_NPC( npc_basemelee, CBaseInfected )

	DECLARE_TASK( TASK_MELEE_YAW_TO_OBJ )
	DECLARE_TASK( TASK_MELEE_ATTACK_OBJ )

	DECLARE_CONDITION( COND_MELEE_BLOCKED_BY_OBJ )
	DECLARE_CONDITION( COND_MELEE_OBJ_VANISH )

	DEFINE_SCHEDULE
	(
		SCHED_MELEE_BASH_OBJ,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		"		TASK_MELEE_YAW_TO_OBJ			0"
		"		TASK_FACE_IDEAL					0"
		"		TASK_MELEE_ATTACK_OBJ			0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_MELEE_BASH_OBJ"
		"	"
		"	Interrupts"
		"		COND_MELEE_OBJ_VANISH"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
	)

AI_END_CUSTOM_NPC()