//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "c_in_player.h"

#include "prediction.h"
#include "iviewrender_beams.h"
#include "beam_shared.h"

#include "weapon_inbase.h"
#include "in_gamerules.h"
#include "c_in_ragdoll.h"

#include "dlight.h"
#include "iefx.h"

#include "input.h"
#include "iinput.h"

#include "toolframework/itoolframework.h"
#include "toolframework_client.h"

#include "tier0/memdbgon.h"

#ifdef CIN_Player
	#undef CIN_Player
#endif

//=========================================================
// Comandos
//=========================================================

static ConVar playermodel("cl_playermodel", "models/player/group01/female_01.mdl", FCVAR_ARCHIVE, "");

static ConVar firstperson_ragdoll("cl_firstperson_ragdoll", "0", FCVAR_CHEAT, "");

//=========================================================
// Información y Red
//=========================================================

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

// DT_TEPlayerAnimEvent
BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropEHandle( RECVINFO( hPlayer ) ),
	RecvPropInt( RECVINFO( iEvent ) ),
	RecvPropInt( RECVINFO( nData ) )
END_RECV_TABLE( )

// DT_InLocalPlayerExclusive
BEGIN_RECV_TABLE_NOBASE( C_IN_Player, DT_InLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropEHandle( RECVINFO( m_nRagdoll ) ),
END_RECV_TABLE()

// DT_InNonLocalPlayerExclusive
BEGIN_RECV_TABLE_NOBASE( C_IN_Player, DT_InNonLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
END_RECV_TABLE()

// DT_INPlayer
IMPLEMENT_CLIENTCLASS_DT( C_IN_Player, DT_InPlayer, CIN_Player )
	RecvPropDataTable( "inlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_InLocalPlayerExclusive ) ),
	RecvPropDataTable( "innonlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_InNonLocalPlayerExclusive ) ),	

	RecvPropBool( RECVINFO(m_bDejected) ),
	RecvPropBool( RECVINFO(m_bInCombat) ),
END_RECV_TABLE()

// Predicción
BEGIN_PREDICTION_DATA( C_IN_Player )
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA( )

//=========================================================
//=========================================================
C_IN_Player::C_IN_Player( ) : iv_angEyeAngles( "C_IN_Player::iv_angEyeAngles" )
{
	// Iniciamos la variable
	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &iv_angEyeAngles, LATCH_SIMULATION_VAR );

	// Creamos el manejador de animaciones
	m_nAnimState = CreatePlayerAnimationState( this );

	// Evitamos el recorte de luces
	ConVarRef scissor( "r_flashlightscissor" );
	scissor.SetValue( "0" );

	// TODO: Ubicar el origen
	ConVarRef cam_idealpitch( "cam_idealpitch" );
	cam_idealpitch.SetValue( 0 );
	ConVarRef cam_idealdist( "cam_idealdist" );
	cam_idealdist.SetValue( 100 );

	::input->CAM_ToFirstPerson();

	// Evitamos el Crosshair
	//ConVarRef crosshair("crosshair");
	//crosshair.SetValue( "0" );
}

//=========================================================
//=========================================================
C_IN_Player::~C_IN_Player( )
{
	ReleaseFlashlight();

	if ( m_nAnimState )
		m_nAnimState->Release();
}

//=========================================================
//=========================================================
C_IN_Player* C_IN_Player::GetLocalInPlayer( )
{
	return ToInPlayer( C_BasePlayer::GetLocalPlayer( ) );
}

//=========================================================
//=========================================================
void C_IN_Player::PreThink()
{
	QAngle vTempAngles = GetLocalAngles( );

	if ( GetLocalPlayer() == this )
		vTempAngles[PITCH] = EyeAngles()[PITCH];
	else
		vTempAngles[PITCH] = m_angEyeAngles[PITCH];

	if ( vTempAngles[YAW] < 0.0f )
		vTempAngles[YAW] += 360.0f;

	// Actualizamos el efecto de disparo
	if ( m_hFireEffect )
		m_hFireEffect->Think();

	SetLocalAngles( vTempAngles );
	BaseClass::PreThink( );
}

//=========================================================
// Devuelve si el jugador esta presionando un botón.
//=========================================================
bool C_IN_Player::IsPressingButton( int iButton )
{
	return ((m_nButtons & iButton)) ? true : false;
}

//=========================================================
// Devuelve si el jugador ha presionado un botón.
//=========================================================
bool C_IN_Player::IsButtonPressed( int iButton )
{
	return ((m_afButtonPressed & iButton)) ? true : false;
}

//=========================================================
// Devuelve si el jugador ha dejado de presionar un botón.
//=========================================================
bool C_IN_Player::IsButtonReleased( int iButton )
{
	return ((m_afButtonReleased & iButton)) ? true : false;
}

//=========================================================
//=========================================================
void C_IN_Player::PostDataUpdate( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );

	BaseClass::PostDataUpdate( updateType );
}

//=========================================================
//=========================================================
void C_IN_Player::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
		SetNextClientThink( CLIENT_THINK_ALWAYS );

	UpdateVisibility( );
}

//=========================================================
//=========================================================
const QAngle& C_IN_Player::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_nAnimState->GetRenderAngles();
	}
}

//=========================================================
// Actualiza las animaciones en el cliente
//=========================================================
void C_IN_Player::UpdateClientSideAnimation()
{
	m_nAnimState->Update();
	BaseClass::UpdateClientSideAnimation();
}

//=========================================================
// Devuelve los angulos de los ojos
//=========================================================
const QAngle &C_IN_Player::EyeAngles()
{
	if ( IsLocalPlayer() )
		return BaseClass::EyeAngles();
	
	return m_angEyeAngles;
}

//=========================================================
//=========================================================
void C_IN_Player::CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	//
	// Heos muerto
	//
	if ( !IsAlive() )
	{
		// Obtenemos nuestro cadaver
		CBaseAnimating *pRagdoll = (CBaseAnimating *)m_nRagdoll.Get();

		//
		// Muerte en primera persona
		//
		if ( firstperson_ragdoll.GetBool() )
		{
			if ( !pRagdoll )
				pRagdoll = this;

			// Obtenemos el origen de los ojos del cadaver
			pRagdoll->GetAttachment( pRagdoll->LookupAttachment("eyes"), eyeOrigin, eyeAngles );

			Vector vecForward; 
			AngleVectors( eyeAngles, &vecForward );

			// Verificamos si la nueva vista no choca contra alguna pared
			trace_t tr;
			UTIL_TraceLine( eyeOrigin, eyeOrigin + ( vecForward * 10000 ), MASK_ALL, pRagdoll, COLLISION_GROUP_NONE, &tr );

			// La vista no choca, todo bien
			if ( !(tr.fraction < 1) || (tr.endpos.DistTo(eyeOrigin) > 30) )
				return;
		}

		//
		// Muerte en tercera persona
		//
		eyeOrigin		= vec3_origin;
		eyeAngles		= vec3_angle;

		// Adjuntamos la cámara sobre nosotros
		Vector origin	= GetAbsOrigin();
		origin.z		+= VEC_DEAD_VIEWHEIGHT.z;

		// Nuestro cadaver existe, ajustamos la cámara sobre el
		if ( pRagdoll )
		{
			origin		= pRagdoll->GetAbsOrigin();
			origin.z	+= VEC_DEAD_VIEWHEIGHT.z;
		}

		BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );

		eyeOrigin = origin;
		Vector vecForward; 

		AngleVectors( eyeAngles, &vecForward );
		VectorNormalize( vecForward );
		VectorMA( origin, -50.0F, vecForward, eyeOrigin );

		Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
		Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

		trace_t trace; // clip against world
		C_BaseEntity::EnableAbsRecomputations( false );

		UTIL_TraceHull(origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace);
		C_BaseEntity::EnableAbsRecomputations( true );

		if ( trace.fraction < 1.0 )
			eyeOrigin = trace.endpos;

		return;
	}

	BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );
}

//=========================================================
// [Evento] Un nuevo modelo
//=========================================================
CStudioHdr *C_IN_Player::OnNewModel()
{
	CStudioHdr *pHDR = BaseClass::OnNewModel();
	InitializePoseParams();

	// Reset the players animation states, gestures
	if ( m_nAnimState )
		m_nAnimState->OnNewModel();

	return pHDR;
}
//=========================================================
//=========================================================
void C_IN_Player::InitializePoseParams()
{
	if ( !IsAlive() )
		return;

	iHeadYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( iHeadYawPoseParam, flHeadYawMin, flHeadYawMax );

	iHeadPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( iHeadPitchPoseParam, flHeadPitchMin, flHeadPitchMax );

	CStudioHdr *pHDR = GetModelPtr();

	for ( int i = 0; i < pHDR->GetNumPoseParameters() ; i++ )
		SetPoseParameter( pHDR, i, 0.0 );
}

//=========================================================
//=========================================================
bool C_IN_Player::ShouldDrawLocalPlayer()
{
	if ( m_lifeState == LIFE_DYING )
		return true;

	// Estamos ejecutando la animación de muerte
	/*if ( m_lifeState == LIFE_DYING )
		return true;

	// Estamos en tercera persona
	if ( input->CAM_IsThirdPerson() )
		return true;

	if ( ToolsEnabled() && ToolFramework_IsThirdPersonCamera() )
		return true;*/

	return BaseClass::ShouldDrawLocalPlayer();
}

//=========================================================
//=========================================================
bool C_IN_Player::ShouldDraw()
{
	if ( m_lifeState == LIFE_DYING )
		return true;

	// Estoy muerto, nuestro cadaver es el que mostrará
	/*if ( m_lifeState == LIFE_DEAD )
		return false;

	// Estamos como espectador
	if ( GetTeamNumber() == TEAM_SPECTATOR )
		return false;

	// Somos
	if ( this != GetSplitScreenViewPlayer() )
		return true;

	//
	if ( !C_IN_Player::ShouldDrawLocalPlayer() )
		return false;

	//
	if ( GetObserverMode() == OBS_MODE_DEATHCAM )
		return true;*/
	
	return BaseClass::ShouldDraw();
}

//=========================================================
// Ejecuta una animación
//=========================================================
void C_IN_Player::DoAnimationEvent( PlayerAnimEvent_t pEvent, int nData )
{
	if ( IsLocalPlayer() )
	{
		if ( ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_nAnimState->DoAnimationEvent( pEvent, nData );
}

//=========================================================
//=========================================================
C_BaseAnimating * C_IN_Player::BecomeRagdollOnClient()
{
	return NULL;
}

//=========================================================
// Devuelve el cadaver del jugador
//=========================================================
IRagdoll* C_IN_Player::GetRepresentativeRagdoll( ) const
{
	/*if ( m_nRagdoll.Get() )
	{
		C_IN_Ragdoll *pRagdoll = (C_IN_Ragdoll *)m_nRagdoll.Get();
		return pRagdoll->GetIRagdoll();
	}
	else*/
	{
		return NULL;
	}
}

//=========================================================
// Tipo de sombra.
//=========================================================
ShadowType_t C_IN_Player::ShadowCastType()
{
	if ( !IsVisible() )
		 return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

//=========================================================
// Devuelve si el jugador puede recibir luz de las linternas.
//=========================================================
bool C_IN_Player::ShouldReceiveProjectedTextures( int iFlags )
{
	if ( IsEffectActive(EF_NODRAW) )
		 return false;

	if ( iFlags & SHADOW_FLAGS_FLASHLIGHT )
		return true;

	return BaseClass::ShouldReceiveProjectedTextures( iFlags );
}

//=========================================================
//=========================================================
void C_IN_Player::AddEntity()
{
	//BaseClass::AddEntity();

	// Esto no aplica a nosotros mismos
	if ( this == C_BasePlayer::GetLocalPlayer() )
		return;

	// Ya no tiene la linterna activada
	if ( !IsEffectActive( EF_DIMLIGHT ) )
	{
		ReleaseFlashlight();
		return;
	}

	// Ubicación, origen y destino de la luz.
	Vector vecForward, vecRight, vecUp;
	Vector vecPosition = EyePosition();

	// Acoplamiento de donde vendra la luz
	int iAttachment = -1;

	// ¿Desde la boquilla del arma?
	bool bFromWeapon = ( flashlight_weapon.GetBool() && GetActiveInWeapon() );

	// La luz debe venir de la boquilla de nuestra arma
	if ( bFromWeapon )
	{
		iAttachment = GetActiveInWeapon()->LookupAttachment( "muzzle" );
	}

	// De otra forma ¿de nuestros ojos?
	else
	{
		iAttachment = LookupAttachment( "eyes" );
	}

	// Podemos trabajar con el acoplamiento
	if ( iAttachment >= 0 )
	{
		// Ubicación del origen deseado
		QAngle eyeAngles = EyeAngles();

		if ( bFromWeapon )
			GetActiveInWeapon()->GetAttachment( iAttachment, vecPosition, eyeAngles );
		else
			GetAttachment( iAttachment, vecPosition, eyeAngles );

		AngleVectors( eyeAngles, &vecForward, &vecRight, &vecUp );
	}
	// Usamos el método original 
	else
	{
		EyeVectors( &vecForward, &vecRight, &vecUp );
	}

	// Debemos usar la luz realisitica (con sombras en tiempo real)
	if ( flashlight_realistic.GetBool() )
	{
		// El efecto no ha sido creado
		if ( !m_nThirdFlashlight )
		{
			m_nThirdFlashlight = new CFlashlightEffect( index );

			// Creación fallida
			if ( !m_nThirdFlashlight )
			{
				Warning("[C_IN_Player::AddEntity] Ha ocurrido un problema al crear la luz de %s \n", GetPlayerName());
				return;
			}

			// La encendemos.
			m_nThirdFlashlight->Init();
			m_nThirdFlashlight->SetFOV( 30.0f );
			m_nThirdFlashlight->TurnOn();
		}

		// Actualizamos la posición
		m_nThirdFlashlight->Update( vecPosition, vecForward, vecRight, vecUp );
	}
	
	trace_t tr;
	UTIL_TraceLine( vecPosition, vecPosition + (vecForward * 200), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		// El destello no ha sido creado
		if ( !m_nFlashlightBeam )
		{
			BeamInfo_t beamInfo;
			beamInfo.m_nType		= TE_BEAMPOINTS;
			beamInfo.m_vecStart		= tr.startpos;
			beamInfo.m_vecEnd		= tr.endpos;
			beamInfo.m_pszModelName = "sprites/glow01.vmt"; 
			beamInfo.m_pszHaloName	= "sprites/glow01.vmt";
			beamInfo.m_flHaloScale	= 3.0;
			beamInfo.m_flWidth		= 8.0f;
			beamInfo.m_flEndWidth	= 35.0f;
			beamInfo.m_flFadeLength = 300.0f;
			beamInfo.m_flAmplitude	= 0;
			beamInfo.m_flBrightness = 60.0;
			beamInfo.m_flSpeed		= 0.0f;
			beamInfo.m_nStartFrame	= 0.0;
			beamInfo.m_flFrameRate	= 0.0;
			beamInfo.m_flRed		= 255.0;
			beamInfo.m_flGreen		= 255.0;
			beamInfo.m_flBlue		= 255.0;
			beamInfo.m_nSegments	= 8;
			beamInfo.m_bRenderable	= true;
			beamInfo.m_flLife		= 0.5;
			beamInfo.m_nFlags		= FBEAM_FOREVER | FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;

			m_nFlashlightBeam = beams->CreateBeamPoints( beamInfo );

			// Creación fallida
			if ( !m_nFlashlightBeam )
			{
				Warning("[C_IN_Player::AddEntity] Ha ocurrido un problema al crear el destello de %s \n", GetPlayerName());
				return;
			}
		}

	// Actualizamos la posición
	BeamInfo_t beamInfo;
	beamInfo.m_vecStart = tr.startpos;
	beamInfo.m_vecEnd	= tr.endpos;
	beamInfo.m_flRed	= 255.0;
	beamInfo.m_flGreen	= 255.0;
	beamInfo.m_flBlue	= 255.0;

	beams->UpdateBeamInfo( m_nFlashlightBeam, beamInfo );
}

//=========================================================
// Actualiza la linterna del jugador
//=========================================================
void C_IN_Player::UpdateFlashlight()
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	int iSsPlayer = GET_ACTIVE_SPLITSCREEN_SLOT();

	C_BasePlayer *pPlayer = this;

	// Si estamos en modo espectador usemos la Linterna del Jugador a quien vemos
	if ( !IsAlive() && GetObserverMode() == OBS_MODE_IN_EYE )
	{
		pPlayer = ToBasePlayer( GetObserverTarget() );
	}

	if ( !pPlayer )
		return;

	// Ya no tienes la linterna encendida
	if ( !IsEffectActive( EF_DIMLIGHT ) )
	{
		// Los efectos siguen activos
		if ( m_nFlashlight )
		{
			// Apagamos y eliminamos
			m_nFlashlight->TurnOff();
			delete m_nFlashlight;
			m_nFlashlight = NULL;
		}

		return;
	}

	// El efecto no ha sido creado
	if ( !m_nFlashlight )
	{
		m_nFlashlight = new CFlashlightEffect( pPlayer->index );

		// Creación fallida
		if ( !m_nFlashlight )
		{
			Warning("[C_IN_Player::UpdateFlashlight] Ha ocurrido un problema al crear la luz de %s \n", GetPlayerName());
			return;
		}

		// La encendemos
		m_nFlashlight->TurnOn();
	}

	m_nFlashlight->Init();

	// Ubicación, origen y destino de la luz.
	Vector vecForward, vecRight, vecUp;
	Vector vecPosition = EyePosition();

	// ¿Desde la boquilla del arma?
	bool bFromWeapon = ( flashlight_weapon.GetBool() && GetActiveInWeapon() );

	// Acoplamiento de donde vendra la luz
	int iAttachment = -1;

	//
	// Estamos en tercera persona
	//
	if ( ::input->CAM_IsThirdPerson() )
	{
		// La luz debe venir de la boquilla de nuestra arma
		if ( bFromWeapon )
		{
			iAttachment = GetActiveInWeapon()->LookupAttachment( "muzzle" );
		}

		// De otra forma ¿de nuestros ojos?
		else
		{
			iAttachment = LookupAttachment( "eyes" );
		}

		// Podemos trabajar con el acoplamiento
		if ( iAttachment >= 0 )
		{
			// Ubicación del origen deseado
			QAngle eyeAngles = EyeAngles();

			if ( bFromWeapon )
				GetActiveInWeapon()->GetAttachment( iAttachment, vecPosition, eyeAngles );
			else
				GetAttachment( iAttachment, vecPosition, eyeAngles );

			AngleVectors( eyeAngles, &vecForward, &vecRight, &vecUp );
		}
		// Usamos el método original 
		else
		{
			EyeVectors( &vecForward, &vecRight, &vecUp );
		}
	}
	//
	// Estamos en primera persona
	//
	else
	{
		// La luz debe venir de la boquilla de nuestra arma
		if ( bFromWeapon && GetViewModel() )
		{
			iAttachment = GetViewModel()->LookupAttachment( "muzzle" );
		}

		// Podemos trabajar con el acoplamiento
		if ( iAttachment >= 0 )
		{
			// Ubicación del origen deseado
			QAngle eyeAngles = EyeAngles();

			GetViewModel()->GetAttachment( iAttachment, vecPosition, eyeAngles );
			AngleVectors( eyeAngles, &vecForward, &vecRight, &vecUp );
		}

		// Usamos el método original 
		else
		{
			EyeVectors( &vecForward, &vecRight, &vecUp );
		}
	}

	//int minus = 5;
	//vecForward.x -= minus;

	m_nFlashlight->Update( vecPosition, vecForward, vecRight, vecUp );
}

//=========================================================
// Libera el destello de la linterna.
//=========================================================
void C_IN_Player::ReleaseFlashlight()
{
	if ( m_nFlashlightBeam )
	{
		m_nFlashlightBeam->flags	= 0;
		m_nFlashlightBeam->die		= gpGlobals->curtime - 1;

		//delete m_nFlashlightBeam;
		m_nFlashlightBeam = NULL;
	}

	if ( m_nThirdFlashlight )
	{
		m_nThirdFlashlight->TurnOff();
		delete m_nThirdFlashlight;
		m_nThirdFlashlight = NULL;
	}
}

//=========================================================
//=========================================================
bool C_IN_Player::CreateLightEffects()
{
	dlight_t *dl;

	// Is this for player flashlights only, if so move to linkplayers?
	if ( index == render->GetViewEntity() )
		return false;

	if ( IsEffectActive(EF_BRIGHTLIGHT) )
	{
		dl = effects->CL_AllocDlight ( index );
		dl->origin = GetAbsOrigin();
		dl->origin[2] += 16;
		dl->color.r = dl->color.g = dl->color.b = 250;
		dl->radius = random->RandomFloat(400,431);
		dl->die = gpGlobals->curtime + 0.001;
	}

	return true;
}