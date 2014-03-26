
#include "cbase.h"
#include "ai_basemelee_npc.h"
#include "in_gamerules.h"

#include "func_break.h"

#include "in_utils.h"

extern bool RagdollManager_SaveImportant( CAI_BaseNPC *pNPC );

//=========================================================
// Información
//=========================================================

LINK_ENTITY_TO_CLASS( npc_basemelee, CAI_BaseMeleeNPC );

BEGIN_DATADESC( CAI_BaseMeleeNPC )
	DEFINE_FIELD( m_nMoveXPoseParam, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMoveYPoseParam, FIELD_INTEGER ),
	DEFINE_FIELD( m_nLeanYawPoseParam, FIELD_INTEGER ),
END_DATADESC()

//=========================================================
// Tareas programadas
//=========================================================
enum
{
	SCHED_MELEE_STAND_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_MELEE_CHASE_ENEMY
};

//=========================================================
// Condiciones
//=========================================================
enum
{
	COND_CAN_MELEE_STAND_ATTACK1 = LAST_SHARED_CONDITION,
};

//=========================================================
// Comandos
//=========================================================

ConVar ai_melee_trace_attack( "ai_melee_trace_attack", "1", FCVAR_CHEAT, "" );

//=========================================================
// Devuelve el genero del NPC
//=========================================================
const char *CAI_BaseMeleeNPC::GetGender()
{
	// Modelo femenino
	if ( Q_stristr(STRING(GetModelName()), "female") )
		return "Female";
	
	return "Male";
}

//=========================================================
//=========================================================
void CAI_BaseMeleeNPC::SetupGlobalModelData()
{
	m_nMoveXPoseParam	= LookupPoseParameter("move_x");
	m_nMoveYPoseParam	= LookupPoseParameter("move_y");
	m_nLeanYawPoseParam = LookupPoseParameter("lean_yaw");
}

//=========================================================
//=========================================================
CGib *CAI_BaseMeleeNPC::CreateGib( const char *pModelName, const Vector &vecOrigin, const Vector &vecDir, int iNum, int iSkin )
{
	static int gibsBodyPart;
	++gibsBodyPart;

	CGib *pGib = CREATE_ENTITY( CGib, "gib" );

	pGib->Spawn( pModelName, gpGlobals->curtime + 4.0f );
	pGib->m_material	= matFlesh;
	pGib->m_nBody		= iNum;
	pGib->m_nSkin		= iSkin;
	pGib->SetBloodColor( BloodColor() );
	pGib->InitGib( this, RandomInt(60, 80), RandomInt(10, 60) );
	pGib->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE_DEBRIS );

	Vector vecNewOrigin = vecOrigin;
	vecNewOrigin.x += RandomFloat( -3.0, 3.0 );
	vecNewOrigin.y += RandomFloat( -3.0, 3.0 );
	vecNewOrigin.z += RandomFloat( -3.0, 3.0 );

	{
		pGib->SetAbsOrigin( vecNewOrigin );
		//pGib->ApplyAbsVelocityImpulse( vecDir * 5 );
	}

	return pGib;
}

//=========================================================
//=========================================================
void CAI_BaseMeleeNPC::Event_Killed( const CTakeDamageInfo &info )
{
	// El código de abajo solo es para reproducir animaciones de muerte
	if ( !CanPlayDeathAnim(info) )
	{
		BaseClass::Event_Killed( info );
		return;
	}

	m_OnDeath.FireOutput( info.GetAttacker(), this );
	SendOnKilledGameEvent( info );

	AddSolidFlags( FSOLID_NOT_SOLID );
	m_lifeState = LIFE_DYING;

	// Put blood on the ground if near enough
	trace_t bloodTrace;
	AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 256 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &bloodTrace);
	
	if ( bloodTrace.fraction < 1.0f )
	{
		UTIL_BloodDecalTrace( &bloodTrace, BloodColor() );
	}

	if ( ( GetFlags() & FL_NPC ) && ( ShouldGib( info ) == false ) )
	{
		SetTouch( NULL );
	}

	DeathSound( info );

	SetActivity( ACT_DIESIMPLE );
	StudioFrameAdvance();

	// Daño que nos ha eliminado
	m_pLastDamage = info;

	SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink( &CAI_BaseMeleeNPC::DeathThink );
}

//=========================================================
//=========================================================
void CAI_BaseMeleeNPC::DeathThink()
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	StudioFrameAdvance();
	DispatchAnimEvents( this );

	// Nuestra animación de muerte no ha terminado
	if ( !IsActivityFinished() )
		return;

	StopAnimation();

	if ( m_bFadeCorpse )
	{
		m_bImportanRagdoll = RagdollManager_SaveImportant( this );
	}

	SetCondition( COND_LIGHT_DAMAGE );
	SetIdealState( NPC_STATE_DEAD );

	if ( CanBecomeRagdoll() )
	{
		if ( CanBecomeRagdoll() || IsRagdoll() )
			 SetState( NPC_STATE_DEAD );

		// Nuestro cadaver en cliente
		BecomeRagdollOnClient( vec3_origin );
	}

	CleanupOnDeath( m_pLastDamage.GetAttacker() );
	m_lifeState	= LIFE_DEAD;

	SetThink( &CAI_BaseNPC::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1.0f );
}

//=========================================================
// Devuelve si el NPC puede usar una animación al morir
//=========================================================
bool CAI_BaseMeleeNPC::CanPlayDeathAnim( const CTakeDamageInfo &info )
{
	return InRules->CanPlayDeathAnim(this, info);
}

//=========================================================
// Ataque cuerpo a cuerpo
//=========================================================
CBaseEntity *CAI_BaseMeleeNPC::MeleeAttack()
{
	// No tenemos a ningún enemigo
	if ( !GetEnemy() )
		return NULL;

	// Información
	CBaseEntity *pVictim	= NULL;
	int iDamage				= GetMeleeDamage();
	int iDamageBits			= DMG_SLASH;

	Vector vecMins	= GetHullMins();
	Vector vecMaxs	= GetHullMaxs();
	vecMins.z		= vecMins.x;
	vecMaxs.z		= vecMaxs.x;

	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );

	// Veces que puedo golpear a un objeto y el numero de objetos que he golpeado.
	int iMax	= GetMaxObjectsToStrike();
	int iStrike = 0;

	ConVarRef ai_show_hull_attacks("ai_show_hull_attacks");
	trace_t	tr;

	// TODO: Para hacer que esto funcione es necesario que el NPC tenga un impulso parecido a npc_boss
	// Es decir, golpear a algo muy lejos para que los demás puedan ser golpeados también.
	for ( int i = 0; i < iMax; ++i )
	{
		// Trazamos una línea enfrente de mi y verificamos si hemos dado con algo
		AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + vecForward * GetMeleeDistance(), vecMins, vecMaxs, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

		// Handy debuging tool to visualize HullAttack trace
		if ( ai_show_hull_attacks.GetBool() )
		{
			float length	 = ( tr.endpos - tr.startpos ).Length();
			Vector direction = ( tr.endpos - tr.startpos );
			VectorNormalize( direction );

			Vector hullMaxs = vecMaxs;
			hullMaxs.x		= length + hullMaxs.x;

			NDebugOverlay::BoxDirection( tr.startpos, vecMins, hullMaxs, direction, 100,255,255,20,1.0 );
			NDebugOverlay::BoxDirection( tr.startpos, vecMins, vecMaxs, direction, 255,0,0,20,1.0 );
		}

		// No hemos dado con algo...
		if ( tr.fraction == 1.0 || !tr.m_pEnt )
			continue;
		
		// Es el mismo que he golpeado hace poco
		if ( tr.m_pEnt == pVictim )
			continue;

		CBaseCombatCharacter *pCharacter	= dynamic_cast<CBaseCombatCharacter *>( tr.m_pEnt );
		CAI_BaseNPC *pNPC					= dynamic_cast<CAI_BaseNPC *>( tr.m_pEnt );

		//
		// Es un Jugador o un NPC
		//
		if ( pCharacter )
		{
			// Le di a un amigo mio
			if ( IRelationType(pCharacter) == D_LIKE )
			{
				// No debo atacar a mi amigo
				if ( pNPC && pNPC->CapabilitiesGet() & bits_CAP_FRIENDLY_DMG_IMMUNE )
					continue;

				// Hacemos menos daño
				iDamage = RandomInt( 1, 3 );
			}

			pVictim = AttackCharacter( pCharacter );
		}

		//
		// Es un objeto
		//
		else
		{
			pVictim = AttackEntity( tr.m_pEnt );
		}

		// Hemos dado con algo/alguien
		if ( pVictim )
		{
			++iStrike;

			// Ubicación de donde proviene el impulso.
			Vector vecForce = pVictim->WorldSpaceCenter() - WorldSpaceCenter();
			VectorNormalize( vecForce );
			vecForce *= 5 * 24;

			// Realizamos el daño
			CTakeDamageInfo info( this, this, vecForce, GetAbsOrigin(), iDamage, iDamageBits );
			pVictim->TakeDamage( info );
		}
	}

	m_nLastEnemy = pVictim;

	// FIXME: Por ahora solo devolvemos la última victima que hemos golpeado.
	return pVictim;
}

//=========================================================
// Realiza el ataque a una entidad
//=========================================================
CBaseEntity *CAI_BaseMeleeNPC::AttackEntity( CBaseEntity *pEntity )
{
	IPhysicsObject *pPhysObj = pEntity->VPhysicsGetObject();

	// No es algo que pueda romper o mover
	if ( !Utils::IsBreakable(pEntity) && !pPhysObj || pEntity->IsWorld() )
		return NULL;

	// Tiene físicas, darle un empujon
	if ( pPhysObj )
	{
		float flEntityMass	= pPhysObj->GetMass();
		float flMaxMass		= GetMaxMassObject();

		// No es pesado para mi
		if ( flEntityMass <= flMaxMass )
		{
			PhysicsImpactSound( GetEnemy(), pPhysObj, CHAN_BODY, pPhysObj->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 1800 );

			Vector vecPhysicsCenter = GetEnemy()->WorldSpaceCenter() - pEntity->WorldSpaceCenter();
			VectorNormalize( vecPhysicsCenter );

			// Impulso hacia adelante y hacia arriba
			vecPhysicsCenter	= vecPhysicsCenter * (flMaxMass*8);
			vecPhysicsCenter.z	+= (flMaxMass*2);

			// Ahora aplicamos el impulso de verdad
			AngularImpulse angVelocity( RandomFloat(-80, 80), 20, RandomFloat(-160, 160) );
			pPhysObj->AddVelocity( &vecPhysicsCenter, &angVelocity );
		}
	}

	return pEntity;
}

//=========================================================
// Realiza el ataque a un personaje
//=========================================================
CBaseEntity *CAI_BaseMeleeNPC::AttackCharacter( CBaseCombatCharacter *pCharacter )
{
	if ( !GetEnemy() )
		return NULL;

	// Punch
	EmitSound("Infected.Hit");

	// Nuestra victima es un jugador, le distorcionamos la vista
	if ( pCharacter->IsPlayer() )
		pCharacter->ViewPunch( QAngle(5, 0, -5) );

	return pCharacter;
}	

//=========================================================
// Devuelve si es conveniente hacer un ataque cuerpo a cuerpo.
//=========================================================
int CAI_BaseMeleeNPC::MeleeAttack1Conditions( float flDot, float flDist )
{
	m_nPotentialEnemy = NULL;

	// No tenemos un enemigo
	if ( !GetEnemy() )
		return COND_NONE;

	// Aún no podemos atacar
	if ( !CanAttack() )
		return COND_TOO_FAR_TO_ATTACK;

	int iMeleeDamage = GetMeleeDistance();

	// Distancia adecuada ¡Atacamos!
	if ( flDist <= (iMeleeDamage/2) )
	{
		DevMsg(2, "[CAI_BaseMeleeNPC::MeleeAttack1Conditions] %f \n", flDot);

		m_nPotentialEnemy = GetEnemy();
		return COND_CAN_MELEE_ATTACK1;
	}

	if ( !ai_melee_trace_attack.GetBool() )
		return COND_TOO_FAR_TO_ATTACK;

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

		// Es algo que se puede romper
		if ( Utils::IsBreakable(tr.m_pEnt) )
			return COND_CAN_MELEE_STAND_ATTACK1;

		// Es algo que se puede mover (Pero no necesariamente algo ligero)
		if ( Utils::IsPhysicsObject(tr.m_pEnt) )
			return COND_CAN_MELEE_ATTACK1;

		// Esta entidad sostiene a mi enemigo
		if ( !tr.m_pEnt->IsWorld() && GetEnemy()->GetGroundEntity() == tr.m_pEnt )
			return COND_CAN_MELEE_STAND_ATTACK1;

		// Es mi enemigo
		if ( tr.m_pEnt == GetEnemy() )
			return COND_CAN_MELEE_ATTACK1;
	}

	// Esta muy lejos
	return COND_TOO_FAR_TO_ATTACK;
}

//=========================================================
//=========================================================
float CAI_BaseMeleeNPC::GetIdealSpeed() const
{
	float flSpeed = BaseClass::GetIdealSpeed();

	if ( flSpeed <= 0 ) 
		flSpeed = 6.0f;

	return flSpeed;
}

//=========================================================
//=========================================================
float CAI_BaseMeleeNPC::GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence )
{
	float flSpeed = BaseClass::GetSequenceGroundSpeed( pStudioHdr, iSequence );

	if( flSpeed <= 0 ) 
		flSpeed = 6.0f;

	return flSpeed;
}

//=========================================================
// Sobrescribe el funcionamiento del Movimiento
// 
// Permite usar move_x y move_y como formas para moverse
//=========================================================
bool CAI_BaseMeleeNPC::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
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
//=========================================================
int CAI_BaseMeleeNPC::SelectSchedule()
{
	//
	// Debemos hacer un ataque sin movernos
	//
	if ( HasCondition(COND_CAN_MELEE_STAND_ATTACK1) )
	{
		ClearCondition( COND_CAN_MELEE_STAND_ATTACK1 );
		return SCHED_MELEE_STAND_ATTACK1;
	}

	return BaseClass::SelectSchedule();
}

//=========================================================
//=========================================================
int CAI_BaseMeleeNPC::TranslateSchedule( int scheduleType )
{
	switch ( scheduleType )
	{
		case SCHED_CHASE_ENEMY:
			return SCHED_MELEE_CHASE_ENEMY;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
// Inteligencia artificial personalizada
//=========================================================
AI_BEGIN_CUSTOM_NPC( npc_basemelee, CAI_BaseMeleeNPC )

	DECLARE_CONDITION( COND_CAN_MELEE_STAND_ATTACK1 )

	DEFINE_SCHEDULE
	(
		SCHED_MELEE_STAND_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING				0"	// Parar de movernos
		"		TASK_MELEE_ATTACK1				0"	// Atacar
		"		TASK_MELEE_ATTACK1				0"
		"		TASK_STOP_MOVING				0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_OCCLUDED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_MELEE_CHASE_ENEMY,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_GET_CHASE_PATH_TO_ENEMY	300"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_CAN_MELEE_STAND_ATTACK1"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_LOST_ENEMY"
		"		COND_BETTER_WEAPON_AVAILABLE"
		"		COND_HEAR_DANGER"
	)

AI_END_CUSTOM_NPC()