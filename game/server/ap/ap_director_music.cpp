//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"

#include "ap_director.h"
#include "ap_director_music.h"

#include "director_manager.h"
#include "in_gamerules.h"

#include "players_manager.h"

// Nuestro ayudante.
CAP_DirectorMusic g_DirectorMusic;
CAP_DirectorMusic *DirectorMusic = &g_DirectorMusic;

//=========================================================
// Comandos de consola
//=========================================================

//=========================================================
// Guardado de objetos necesarios en caché
//=========================================================
void CAP_DirectorMusic::Precache()
{
	BaseClass::Precache();

	CBaseEntity::PrecacheScriptSound("Director.Angry.Low");
	CBaseEntity::PrecacheScriptSound("Director.Angry.Medium");
	CBaseEntity::PrecacheScriptSound("Director.Angry.High");
	CBaseEntity::PrecacheScriptSound("Director.Angry.Crazy");
	CBaseEntity::PrecacheScriptSound("Director.Angry.Sleep");

	CBaseEntity::PrecacheScriptSound("Director.Ambient.Surprise");
	CBaseEntity::PrecacheScriptSound("Director.Ambient.TooClose");

	CBaseEntity::PrecacheScriptSound("Director.Ambient.Terror");

	CBaseEntity::PrecacheScriptSound("Director.Combat.Danger");
	CBaseEntity::PrecacheScriptSound("Director.Combat.Background");
}

//=========================================================
// Inicializa al Director de Música
//=========================================================
void CAP_DirectorMusic::Init()
{
	BaseClass::Init();
	DirectorMusic = this;

	//m_nPanicDanger		= new EnvMusic("Director.Combat.Danger");
	//m_nPanicBackground	= new EnvMusic("Director.Combat.Background");

	// Reproducimos música de ambiente en 15s
	m_nNextAmbientMusic.Start( 15 );
	m_nNextEffectsMusic.Start( 15 );

	m_iLastVisible = 0;
}

//=========================================================
//=========================================================
void CAP_DirectorMusic::OnNewMap()
{
	BaseClass::OnNewMap();

	m_nAngry	= CreateLayerSound( "Director.Angry.Low", LAYER_LOW );
	m_nSurprise	= CreateLayerSound( "Director.Ambient.Surprise", LAYER_LOW );
	m_nTooClose	= CreateLayerSound( "Director.Ambient.TooClose", LAYER_LOW );
	m_nTerror	= CreateLayerSound( "Director.Ambient.Terror", LAYER_LOW );
}

//=========================================================
//=========================================================
void CAP_DirectorMusic::Think()
{
	BaseClass::Think();

	/*m_nAngry->Update();
	m_nSurprise->Update();
	m_nTooClose->Update();
	m_nTerror->Update();*/

	//m_nPanicBackground->Update();
	//m_nPanicDanger->Update();
}

//=========================================================
// Para de reproducir la música
//=========================================================
void CAP_DirectorMusic::Stop()
{
	BaseClass::Stop();

	if ( !m_nAngry )
		return;

	m_nAngry->Stop();
	m_nSurprise->Stop();
	m_nTooClose->Stop();
	m_nTerror->Stop();

	//m_nPanicDanger->Fadeout();
	//m_nPanicBackground->Fadeout();
}

//=========================================================
// Apaga al Director de Música
//=========================================================
void CAP_DirectorMusic::Shutdown()
{
	BaseClass::Shutdown();

	UTIL_Remove( m_nAngry );
	UTIL_Remove( m_nSurprise );
	UTIL_Remove( m_nTooClose );
	UTIL_Remove( m_nTerror );

	//delete m_nPanicDanger;
	//delete m_nPanicBackground;
}

//=========================================================
// Procesa la música
//=========================================================
void CAP_DirectorMusic::Update()
{
	BaseClass::Update();

	if ( InRules->IsGameMode(GAME_MODE_SURVIVAL) )
		UpdateSurvival();
	else
		UpdateNormal();
}

//=========================================================
// Procesa la música en el Gameplay Survival
//=========================================================
void CAP_DirectorMusic::UpdateSurvival()
{
	// Música de suspenso
	if ( m_nNextAmbientMusic.IsElapsed() )
	{
		m_nTerror->Play();

		int iDuration = m_nTerror->GetDuration();
		m_nNextAmbientMusic.Start( RandomInt(iDuration+10, iDuration+30) );
	}
}

//=========================================================
// Procesa la música en el Gameplay Normal
//=========================================================
void CAP_DirectorMusic::UpdateNormal()
{
	//
	// Estamos en un evento de pánico
	//
	/*if ( Director->IsStatus(ST_COMBAT) )
	{
		int iInDanger	= Director->InDangerZone();
		float flVol		= 0.0f;
		
		if ( iInDanger > 8 )
		{
			flVol = 0.5f;
		}
		else if ( iInDanger > 5 )
		{
			flVol = 0.3f;
		}
		else if ( iInDanger > 3 )
		{
			flVol = 0.1f;
		}

		// Reproducimos música de fondo
		// Entre más infectados tengamos cerca menor será su volumen
		if ( !tmp_disable_music.GetBool() )
		{
			m_nPanicBackground->Play();
			m_nPanicBackground->SetVolume( (1 - flVol) );
		}

		// Tenemos a varios infectados cerca, empezamos a reproducirlo
		if ( flVol >= 0.1f )
		{
			m_nPanicDanger->Play();
			m_nPanicDanger->SetVolume( flVol );
		}
		else
		{
			m_nPanicDanger->Fadeout();
		}
	}
	else
	{
		m_nPanicDanger->Fadeout();
		m_nPanicBackground->Fadeout();
	}*/

	UpdateAmbient();
}

//=========================================================
// Procesa la música de ambiente
//=========================================================
void CAP_DirectorMusic::UpdateAmbient()
{
	//
	// No estamos en Normal, quitamos la música de fondo.
	//
	if ( !Director->IsStatus(ST_NORMAL) )
	{
		m_nAngry->Fadeout();
		m_nSurprise->Fadeout();
		m_nTooClose->Fadeout();

		return;
	}

	//
	// Música por eventos:
	// - Estar cerca de varios infectados
	// - De repente ver varios infectados
	//
	if ( m_nNextEffectsMusic.IsElapsed() )
	{
		// Reproducimos nuevamente en 10 a 20s
		m_nNextEffectsMusic.Start( RandomInt(10, 20) );

		// Tenemos a varios infectados cerca
		if ( Director->InDangerZone() >= 5 )
		{
			m_nTooClose->Play();
			return;
		}

		int iActualVisible = Director->ChildsVisibles();

		// De repente estamos viendo muchos infectados
		if ( iActualVisible > m_iLastVisible && iActualVisible >= 5 )
		{
			m_nSurprise->Play();
			m_iLastVisible = iActualVisible;
			return;
		}

		m_iLastVisible = iActualVisible;
	}

	//
	// Música de Ambiente
	// Música dependiendo del nivel de enojo del Director
	//
	if ( m_nNextAmbientMusic.IsElapsed() )
	{
		if ( Director->IsPhase(PH_RELAX) )
		{
			m_nAngry->SetSoundName("Director.Angry.Sleep");
		}
		else
		{
			switch ( Director->m_iAngry )
			{
				case ANGRY_LOW:
					m_nAngry->SetSoundName("Director.Angry.Low");
				break;

				case ANGRY_MEDIUM:
					m_nAngry->SetSoundName("Director.Angry.Medium");
				break;

				case ANGRY_HIGH:
					m_nAngry->SetSoundName("Director.Angry.High");
				break;

				case ANGRY_CRAZY:
					m_nAngry->SetSoundName("Director.Angry.Crazy");
				break;
			}
		}

		m_nAngry->Play();

		// Reproducimos nuevamente...
		int iDuration = m_nAngry->GetDuration();
		m_nNextAmbientMusic.Start( RandomInt(iDuration+10, iDuration+30) );
	}	
}