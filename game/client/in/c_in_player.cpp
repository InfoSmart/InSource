//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

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
#include "in_buttons.h"

#include "toolframework/itoolframework.h"
#include "toolframework_client.h"

#include "tier0/memdbgon.h"

#ifdef CIN_Player
	#undef CIN_Player
#endif

//====================================================================
// Comandos
//====================================================================

static ConVar playermodel( "cl_playermodel", "models/player/group01/female_01.mdl", FCVAR_ARCHIVE, "" );
static ConVar playermodel_view( "cl_playermodel_view", "0", FCVAR_CHEAT, "" );

static ConVar firstperson_ragdoll( "cl_firstperson_ragdoll", "0", FCVAR_CHEAT, "" );

static ConVar flashlight( "cl_flashlight_dev_error", "1" );
static ConVar flashlight_fov( "cl_flashlight_fov", "50.0", FCVAR_CHEAT );
static ConVar flashlight_far( "cl_flashlight_far", "750.0", FCVAR_CHEAT );
static ConVar flashlight_linear( "cl_flashlight_linear", "250.0", FCVAR_CHEAT );

static ConVar flashlightbeam_haloscale( "cl_flashlightbeam_haloscale", "3.0" );
static ConVar flashlightbeam_endwidth( "cl_flashlightbeam_endwidth", "10.0" );
static ConVar flashlightbeam_fadelength( "cl_flashlightbeam_fadelength", "300.0" );
static ConVar flashlightbeam_brightness( "cl_flashlightbeam_brightness", "20.0" );
static ConVar flashlightbeam_segments( "cl_flashlightbeam_segments", "10" );

#define PLAYERMODEL_VIEW	playermodel_view.GetBool()
#define PLAYERMODEL			playermodel.GetString()
#define FIRSTPERSON_RAGDOLL firstperson_ragdoll.GetBool()

//====================================================================
// Información y Red
//====================================================================

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
	RecvPropEHandle( RECVINFO( m_nRagdoll ) ),
END_RECV_TABLE()

// DT_InNonLocalPlayerExclusive
BEGIN_RECV_TABLE_NOBASE( C_IN_Player, DT_InNonLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
END_RECV_TABLE()

// DT_INPlayer
IMPLEMENT_CLIENTCLASS_DT( C_IN_Player, DT_InPlayer, CIN_Player )
	RecvPropDataTable( "inlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_InLocalPlayerExclusive ) ),
	RecvPropDataTable( "innonlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_InNonLocalPlayerExclusive ) ),	

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

	RecvPropBool( RECVINFO(m_bInCombat) ),
	RecvPropBool( RECVINFO(m_bMovementDisabled) ),

	RecvPropInt( RECVINFO(m_iEyeAngleZ) ),

	RecvPropBool( RECVINFO(m_bIncap) ),
	RecvPropBool( RECVINFO(m_bClimbingToHell) ),
	RecvPropBool( RECVINFO(m_bWaitingGroundDeath) ),
	RecvPropInt( RECVINFO(m_iTimesDejected) ),
	RecvPropFloat( RECVINFO(m_flHelpProgress) ),
	RecvPropFloat( RECVINFO(m_flClimbingHold) ),

	RecvPropEHandle( RECVINFO(m_nPlayerInventory) ),
END_RECV_TABLE()

// Predicción
BEGIN_PREDICTION_DATA( C_IN_Player )
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()

//====================================================================
//====================================================================
C_IN_Player::C_IN_Player() : iv_angEyeAngles( "C_IN_Player::iv_angEyeAngles" )
{
	// Iniciamos la variable
	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &iv_angEyeAngles, LATCH_SIMULATION_VAR );

	// Invalidamos el cronometro para el parpadeo
	m_nBlinkTimer.Invalidate();
}

//====================================================================
//====================================================================
C_IN_Player::~C_IN_Player( )
{
	ReleaseFlashlight();

	if ( GetAnimation() )
		GetAnimation()->Release();
}

//====================================================================
//====================================================================
C_IN_Player* C_IN_Player::GetLocalInPlayer()
{
	return ToInPlayer( C_BasePlayer::GetLocalPlayer() );
}

//====================================================================
//====================================================================
void C_IN_Player::Spawn()
{
	BaseClass::Spawn();

	// Creamos el procesador de animaciones
	if ( !GetAnimation() )
		CreateAnimationState();
}

//====================================================================
//====================================================================
void C_IN_Player::PreThink()
{
	QAngle vTempAngles = GetLocalAngles();

	if ( GetLocalPlayer() == this )
		vTempAngles[PITCH] = EyeAngles()[PITCH];
	else
		vTempAngles[PITCH] = m_angEyeAngles[PITCH];

	if ( vTempAngles[YAW] < 0.0f )
		vTempAngles[YAW] += 360.0f;

	SetLocalAngles( vTempAngles );

	// PreThink
	BaseClass::PreThink();
}

//====================================================================
//====================================================================
void C_IN_Player::PostThink()
{
	// PostThink
	BaseClass::PostThink();

	// Mientras sigas vivo
	if ( IsAlive() )
	{
		// Actualizamos la mirada
		UpdateLookAt();

		if ( IsLocalPlayer() )
		{
			UpdatePoseParams();
			UpdateIncapHeadLight();
		}
	}
}

//====================================================================
//====================================================================
bool C_IN_Player::Simulate()
{
	// Mostramos la linterna de otros Jugadores
	UpdatePlayerFlashlight();

	return BaseClass::Simulate();
}

//====================================================================
// Crea el procesador de animaciones
//====================================================================
void C_IN_Player::CreateAnimationState()
{
	m_nAnimation = CreatePlayerAnimationState( this );
}

//====================================================================
// Devuelve si el jugador esta presionando un botón.
//====================================================================
bool C_IN_Player::IsPressingButton( int iButton )
{
	return (m_nButtons & iButton) != 0;
}

//=========================================================
// Devuelve si el jugador ha presionado un botón.
//=========================================================
bool C_IN_Player::IsButtonPressed( int iButton )
{
	return (m_afButtonPressed & iButton) != 0;
}

//=========================================================
// Devuelve si el jugador ha dejado de presionar un botón.
//=========================================================
bool C_IN_Player::IsButtonReleased( int iButton )
{
	return (m_afButtonReleased & iButton) != 0;
}

//=========================================================
//=========================================================
void C_IN_Player::PostDataUpdate( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );

	BaseClass::PostDataUpdate( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Cambios para el Jugador local
		if ( IsLocalPlayer() )
			engine->ClientCmd("exec localplayer;");
	}
}

//=========================================================
//=========================================================
void C_IN_Player::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
		SetNextClientThink( CLIENT_THINK_ALWAYS );

	UpdateVisibility();
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
		return GetAnimation()->GetRenderAngles();
	}
}

//=========================================================
// Actualiza las animaciones en el cliente
//=========================================================
void C_IN_Player::UpdateClientSideAnimation()
{
	// Actualizamos la animación
	GetAnimation()->Update();

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
// Ajusta la vista y angulos a los ojos de una entidad
//=========================================================
bool C_IN_Player::GetEntityEyesView( C_BaseAnimating *pEntity, Vector& eyeOrigin, QAngle& eyeAngles, int secureDistance )
{
	// Obtenemos el origen de los ojos de la entidad
	pEntity->GetAttachment( pEntity->LookupAttachment("eyes"), eyeOrigin, eyeAngles );

	Vector vecForward; 
	AngleVectors( eyeAngles, &vecForward );

	// Verificamos si la nueva vista no choca contra alguna pared
	trace_t tr;
	UTIL_TraceLine( eyeOrigin, eyeOrigin + ( vecForward * secureDistance ), MASK_ALL, pEntity, COLLISION_GROUP_NONE, &tr );

	// La vista no choca, todo bien
	if ( !(tr.fraction < 1) || (tr.endpos.DistTo(eyeOrigin) > 30) )
		return true;

	return false;
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
		if ( FIRSTPERSON_RAGDOLL )
		{
			if ( !pRagdoll )
				pRagdoll = this;

			// Ajustamos la vista a los ojos del cadaver
			if ( GetEntityEyesView(pRagdoll, eyeOrigin, eyeAngles) )
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

	// Estamos incapacitados
	// Iván: Desactivado hasta poder limitar el MouseInput del Jugador
	if ( /*IsDejected() ||*/ PLAYERMODEL_VIEW )
	{
		// Ajustamos la vista a nuestros ojos (del modelo)
		if ( GetEntityEyesView(this, eyeOrigin, eyeAngles) )
			return;

		eyeAngles.z = 0;
	}

	BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );
}

//====================================================================
//====================================================================
void C_IN_Player::UpdateLookAt()
{
	// ¡Parpadeamos!
	if ( m_nBlinkTimer.IsElapsed() )
	{
		m_blinktoggle = !m_blinktoggle;
		m_nBlinkTimer.Start( RandomFloat(1.5f, 4.0f) );
	}

	Vector vecForward;
	AngleVectors( GetLocalAngles(), &vecForward );

	m_viewtarget  = GetAbsOrigin() + vecForward * 512;
}

//====================================================================
// [Evento] Un nuevo modelo
//====================================================================
CStudioHdr *C_IN_Player::OnNewModel()
{
	CStudioHdr *pHDR = BaseClass::OnNewModel();
	InitializePoseParams();

	// Reset the players animation states, gestures
	if ( GetAnimation() )
		GetAnimation()->OnNewModel();

	return pHDR;
}
//=========================================================
//=========================================================
void C_IN_Player::InitializePoseParams()
{
	if ( !IsAlive() )
		return;

	CStudioHdr *pHDR = GetModelPtr();

	if ( !pHDR )
		return;

	for ( int i = 0; i < pHDR->GetNumPoseParameters() ; i++ )
		SetPoseParameter( pHDR, i, 0.0 );
}

//=========================================================
// Procesa los efectos "PostProcessing"
//=========================================================
void C_IN_Player::DoPostProcessingEffects( PostProcessParameters_t &params )
{
	// El Jugador esta incapacitado
	if ( IsIncap() )
	{
		float flHealth		= GetHealth();
		int i				= ( 5 + round(flHealth) );
		float flVignetteEnd = ( flHealth / i );
		float flContrast	= -2.5f + ( flHealth / 10 );

		if ( flVignetteEnd < 0.6f )
			flVignetteEnd = 0.6f;

		if ( flContrast > 3.0f )
			flContrast = 3.0f;

		// Estas trepando por tu vida
		if ( IsClimbingIncap() )
			flContrast += 5.0f;

		params.m_flParameters[ PPPN_LOCAL_CONTRAST_STRENGTH ]	= flContrast;
		params.m_flParameters[ PPPN_VIGNETTE_START ]			= 0.1f;
		params.m_flParameters[ PPPN_VIGNETTE_END ]				= flVignetteEnd;
		params.m_flParameters[ PPPN_VIGNETTE_BLUR_STRENGTH ]	= 0.9f;
	}
}

//=========================================================
//=========================================================
bool C_IN_Player::ShouldDrawLocalPlayer()
{
	if ( m_lifeState == LIFE_DYING )
		return true;

	if ( PLAYERMODEL_VIEW )
		return true;

	return BaseClass::ShouldDrawLocalPlayer();
}

//=========================================================
//=========================================================
bool C_IN_Player::ShouldDraw()
{
	if ( m_lifeState == LIFE_DYING )
		return true;

	if ( PLAYERMODEL_VIEW )
		return true;
	
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
	GetAnimation()->DoAnimationEvent( pEvent, nData );
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
IRagdoll* C_IN_Player::GetRepresentativeRagdoll() const
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

	return SHADOWS_RENDER_TO_DEPTH_TEXTURE;
	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

//=========================================================
// Devuelve si el jugador puede recibir luz de las linternas.
//=========================================================
bool C_IN_Player::ShouldReceiveProjectedTextures( int iFlags )
{
	if ( IsEffectActive(EF_NODRAW) )
		 return false;

	return true;
}

//=========================================================
// Actualiza y muestra la linterna otro Jugador
//=========================================================
void C_IN_Player::UpdatePlayerFlashlight()
{
	if ( this == C_IN_Player::GetLocalInPlayer() )
		return;

	// @TODO
	if ( flashlight.GetBool() )
		return;

	// Linterna apagada
	if ( !IsEffectActive(EF_DIMLIGHT) )
	{
		ReleaseFlashlight();
		return;
	}

	// Ubicación, origen y destino de la luz.
	Vector vecPosition, vecForward, vecRight, vecUp;

	// Obtenemos la ubicación para la linterna
	GetFlashlightPosition( this, vecPosition, vecForward, vecRight, vecUp );

	ConVarRef flashlight_realistic("sv_in_flashlight_realistic");

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
				Warning("[C_IN_Player::UpdatePlayerFlashlight] Ha ocurrido un problema al crear la luz de %s \n", GetPlayerName());
				return;
			}

			// La encendemos.
			m_nThirdFlashlight->Init();
			m_nThirdFlashlight->SetFOV( 30.0f );
			m_nThirdFlashlight->SetFar( GetFlashlightFarZ() );
			m_nThirdFlashlight->SetBright( GetFlashlightLinearAtten() );
			m_nThirdFlashlight->TurnOn();
		}

		// Actualizamos la posición
		m_nThirdFlashlight->Update( vecPosition, vecForward, vecRight, vecUp );
	}
	
	trace_t tr;
	UTIL_TraceLine( vecPosition, vecPosition + (vecForward * 100), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	BeamInfo_t beamInfo;

	// El destello no ha sido creado
	if ( !m_nFlashlightBeam )
	{	
		beamInfo.m_nType		= TE_BEAMPOINTS;
		beamInfo.m_vecStart		= tr.startpos;
		beamInfo.m_vecEnd		= tr.endpos;
		beamInfo.m_pszModelName = "sprites/glow_test02.vmt"; 
		beamInfo.m_pszHaloName	= "sprites/light_glow03.vmt";
		beamInfo.m_flHaloScale	= flashlightbeam_haloscale.GetFloat();
		beamInfo.m_flWidth		= 13.0f;
		beamInfo.m_flEndWidth	= flashlightbeam_endwidth.GetFloat();
		beamInfo.m_flFadeLength = flashlightbeam_fadelength.GetFloat();
		beamInfo.m_flAmplitude	= 0;
		beamInfo.m_flBrightness = flashlightbeam_brightness.GetFloat();
		beamInfo.m_flSpeed		= 0.0f;
		beamInfo.m_nStartFrame	= 0.0;
		beamInfo.m_flFrameRate	= 0.0;
		beamInfo.m_flRed		= 255.0;
		beamInfo.m_flGreen		= 255.0;
		beamInfo.m_flBlue		= 255.0;
		beamInfo.m_nSegments	= flashlightbeam_segments.GetFloat();
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
	beamInfo.m_vecStart		= tr.startpos;
	beamInfo.m_vecEnd		= tr.endpos;
	beamInfo.m_flRed		= 255.0;
	beamInfo.m_flGreen		= 255.0;
	beamInfo.m_flBlue		= 255.0;
	beams->UpdateBeamInfo( m_nFlashlightBeam, beamInfo );
}

//=========================================================
// Actualiza y muestra la linterna del jugador
//=========================================================
void C_IN_Player::UpdateFlashlight()
{
	C_IN_Player *pPlayer = this;

	// Si estamos en modo espectador usemos la Linterna del Jugador a quien vemos
	if ( !IsAlive() && GetObserverMode() == OBS_MODE_IN_EYE )
		pPlayer = ToInPlayer( GetObserverTarget() );

	// No hay Jugador
	if ( !pPlayer )
		return;

	// EF_DIMLIGHT = Linterna
	bool bIsOn = pPlayer->IsEffectActive( EF_DIMLIGHT );

	// El Jugador ha muerto, no mostrar la linterna
	if ( !pPlayer->IsAlive() || pPlayer->GetViewEntity() )
		bIsOn = false;

	// La linterna no ha sido creada
	if ( !m_nFlashlight )
	{
		m_nFlashlight = new CFlashlightEffect( pPlayer->index, GetFlashlightTextureName() );

		// No hemos podido crearla
		if ( !m_nFlashlight )
		{
			Warning( "[C_IN_Player::UpdateFlashlight] Ha ocurrido un problema al crear la luz de %s \n", pPlayer->GetPlayerName() );
			return;
		}
	}

	// Linterna apagada
	if ( !bIsOn )
	{
		// La apagamos
		m_nFlashlight->TurnOff();
		return;
	}
	else
	{
		// La encendemos
		m_nFlashlight->TurnOn();
	}

	// Configuramos la linterna
	m_nFlashlight->Init();
	m_nFlashlight->SetFOV( GetFlashlightFOV() );
	m_nFlashlight->SetFar( GetFlashlightFarZ() );
	m_nFlashlight->SetBright( GetFlashlightLinearAtten() );

	// Obtenemos la posición donde estará la linterna
	GetFlashlightPosition( pPlayer, m_vecFlashlightOrigin, m_vecFlashlightForward, m_vecFlashlightRight, m_vecFlashlightUp );

	// Actualizamos la linterna
	m_nFlashlight->Update( m_vecFlashlightOrigin, m_vecFlashlightForward, m_vecFlashlightRight, m_vecFlashlightUp );
}

//=========================================================
//=========================================================
const char *C_IN_Player::GetFlashlightTextureName() const
{
	return "effects/flashlight001_improved";
}

float C_IN_Player::GetFlashlightFOV() const
{
	return flashlight_fov.GetFloat();
}

float C_IN_Player::GetFlashlightFarZ()
{
	return flashlight_far.GetFloat();
}

float C_IN_Player::GetFlashlightLinearAtten()
{
	return flashlight_linear.GetFloat();
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

		delete m_nFlashlightBeam;
		m_nFlashlightBeam = NULL;
	}

	if ( m_nThirdFlashlight )
	{
		m_nThirdFlashlight->TurnOff();
		delete m_nThirdFlashlight;
		m_nThirdFlashlight = NULL;
	}
}

void C_IN_Player::UpdateIncapHeadLight()
{
	UpdateFlashlight();

	C_IN_Player *pPlayer = this;

	// Si estamos en modo espectador usemos la Linterna del Jugador a quien vemos
	if ( !IsAlive() && GetObserverMode() == OBS_MODE_IN_EYE )
		pPlayer = ToInPlayer( GetObserverTarget() );

	// No hay Jugador
	if ( !pPlayer )
		return;

	// No hemos creado la linterna
	if ( !m_nHeadLight )
	{
		m_nHeadLight = new CFlashlightEffect( pPlayer->index, "effects/muzzleflash_light" );

		if ( !m_nHeadLight )
			return;
	}

	// No estamos incapacitados
	if ( !IsIncap() )
	{
		m_nHeadLight->TurnOff();
		return;
	}
	else
	{
		m_nHeadLight->TurnOn();
	}

	// Configuramos la linterna
	m_nHeadLight->Init();
	m_nHeadLight->SetFOV( 90.0f );
	m_nHeadLight->SetFar( 180.0f );
	m_nHeadLight->SetBright( 500.0f );

	Vector vecForward, vecRight, vecUp;
	Vector vecSrc = pPlayer->GetAbsOrigin();
	QAngle angle;

	VectorAngles( vecSrc, angle );
	AngleVectors( angle, &vecForward, &vecRight, &vecUp );

	trace_t tr1;
	UTIL_TraceLine( vecSrc, vecSrc + vecUp * 100, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr1 );

	// Actualizamos la linterna
	m_nHeadLight->Update( tr1.endpos, -vecUp, vecRight, vecForward );
}

//=========================================================
// Establece en las variables los vectores de la posición
// de la linterna del Jugador
//=========================================================
void C_IN_Player::GetFlashlightPosition( C_IN_Player *pPlayer, Vector &vecPosition, Vector &vecForward, Vector &vecRight, Vector &vecUp, const char *pWeaponAttach )
{
	if ( !pPlayer )
		return;

	ConVarRef flashlight_weapon("sv_in_flashlight_weapon");

	// ¿Desde la boquilla del arma?
	bool bFromWeapon = ( flashlight_weapon.GetBool() && pPlayer->GetActiveInWeapon() );

	// Información de origen de la luz
	int iAttachment				= -1;
	C_BaseAnimating *pParent	= pPlayer;
	const char *pAttachment		= "eyes";

	bool bFromOther = false;

	// La luz debe venir de la boquilla de nuestra arma
	if ( bFromWeapon )
	{
		if ( !C_BasePlayer::IsLocalPlayer(pPlayer) || pPlayer != C_BasePlayer::GetLocalPlayer() )
		{
			pParent		= pPlayer->GetActiveInWeapon();
			bFromOther	= true;
		}
		else
		{
			pParent	= pPlayer->GetViewModel();
		}

		pAttachment = pWeaponAttach;
	}

	if ( !pParent )
		return;

	iAttachment = pParent->LookupAttachment( pAttachment );

	// El acoplamiento existe, obtenemos la ubicación de este
	if ( iAttachment >= 0 )
	{
		QAngle eyeAngles;
		pParent->GetAttachment( iAttachment, vecPosition, eyeAngles );

		// @FIX!
		if ( bFromOther && bFromWeapon )
		{
			AngleVectors( eyeAngles, &vecRight, &vecUp, &vecForward );
		}
		else
		{
			AngleVectors( eyeAngles, &vecForward, &vecRight, &vecUp );
		}		
	}

	// De otra forma la luz se originara desde nuestros ojos
	else
	{
		vecPosition = EyePosition();
		pPlayer->EyeVectors( &vecForward, &vecRight, &vecUp );
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
		dl = effects->CL_AllocDlight( index );
		dl->origin		= GetAbsOrigin();
		dl->origin[2]	+= 16;
		dl->color.r		= dl->color.g = dl->color.b = 250;
		dl->radius		= random->RandomFloat(400,431);
		dl->die			= gpGlobals->curtime + 0.001;
	}

	return true;
}

//=========================================================
// Devuelve el inventario del Jugador
//=========================================================
C_PlayerInventory *C_IN_Player::GetInventory()
{
	if ( !m_nPlayerInventory.Get() )
		return NULL;

	return (C_PlayerInventory *)m_nPlayerInventory.Get();
}

//=========================================================
// Procesa los comandos de movimiento del Jugador
//=========================================================
bool C_IN_Player::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{
	// Angulos de vista
	pCmd->viewangles.z = m_iEyeAngleZ;

	// No moverse.
	if ( m_bMovementDisabled )
	{
		pCmd->forwardmove	= 0;
		pCmd->sidemove		= 0;
		pCmd->upmove		= 0;
	}
	else
	{
		// Giro
		if ( !IsObserver() )
		{
			if ( pCmd->sidemove < 0 )
				pCmd->viewangles.z -= .5f;
			if ( pCmd->sidemove > 0 )
				pCmd->viewangles.z += .5f;
		}
	}

	return BaseClass::CreateMove( flInputSampleTime, pCmd );
}