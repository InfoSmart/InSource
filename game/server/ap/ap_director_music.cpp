//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"

#include "ap_director.h"
#include "ap_director_music.h"

#include "director_manager.h"
#include "in_gamerules.h"

#include "players_manager.h"

// Nuestro ayudante.
CAP_DirectorMusic g_DirMusic;
CAP_DirectorMusic *DirectorMusic = &g_DirMusic;

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

	CBaseEntity::PrecacheScriptSound("Director.Panic.Escape");
	CBaseEntity::PrecacheScriptSound("Director.Panic.Background");
}

//=========================================================
// Inicializa al Director de Música
//=========================================================
void CAP_DirectorMusic::Init()
{
	BaseClass::Init();

	m_nAngry	= new EnvMusic("Director.Angry.Low");
	m_nSurprise	= new EnvMusic("Director.Ambient.Surprise");
	m_nTooClose	= new EnvMusic("Director.Ambient.TooClose");
	m_nTerror	= new EnvMusic("Director.Ambient.Terror");

	m_nPanic			= new EnvMusic("Director.Panic.Escape");
	m_nPanicBackground	= new EnvMusic("Director.Panic.Background");

	// Reproducimos música de ambiente en 15s
	m_nNextAmbientMusic.Start( 15 );
	m_nNextEffectsMusic.Start( 15 );

	m_iLastVisible = 0;
}

//=========================================================
//=========================================================
void CAP_DirectorMusic::Think()
{
	BaseClass::Think();

	m_nAngry->Update();
	m_nSurprise->Update();
	m_nTooClose->Update();
	m_nTerror->Update();
}

//=========================================================
// Para de reproducir la música
//=========================================================
void CAP_DirectorMusic::Stop()
{
	BaseClass::Stop();

	if ( !m_nAngry )
		return;

	m_nAngry->Fadeout();
	m_nSurprise->Fadeout();
	m_nTooClose->Fadeout();
	m_nTerror->Fadeout();

	m_nPanic->Fadeout();
	m_nPanicBackground->Fadeout();
}

//=========================================================
// Apaga al Director de Música
//=========================================================
void CAP_DirectorMusic::Shutdown()
{
	BaseClass::Shutdown();

	delete m_nAngry;
	delete m_nSurprise;
	delete m_nTooClose;
	delete m_nTerror;

	delete m_nPanic;
	delete m_nPanicBackground;
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
	if ( Director->Is(PANIC) || Director->Is(POST_PANIC) )
	{
		// Reproducimos música de fondo
		m_nPanicBackground->Play();

		int iInDanger = Director->InDangerZone();

		if ( iInDanger > 15 )
			m_nPanicBackground->SetPitch( 110.0f );
		else if ( iInDanger > 8 )
			m_nPanicBackground->SetPitch( 105.0f );
		else if ( iInDanger > 5 )
			m_nPanicBackground->SetPitch( 100.0f );
		else
			m_nPanicBackground->SetPitch( 90.0f );

		/*if ( !m_nPanic->IsPlaying() )
		{
			// Sugerimos ¡¡correr!!
			if ( PlysManager->GetStatus() <= STATS_MED )
			{
				m_nPanic->SetSoundName("Director.Panic.Escape");
				m_nPanic->Play();
			}
			else
			{
				m_nPanic->SetSoundName("Director.Panic.Combat");
				m_nPanic->Play();
			}
		}*/
	}
	else
	{
		m_nPanic->Fadeout();
		m_nPanicBackground->Fadeout();
	}

	UpdateAmbient();
}

//=========================================================
// Procesa la música de ambiente
//=========================================================
void CAP_DirectorMusic::UpdateAmbient()
{
	//
	// No estamos en Relajado ni Dormido, quitamos la música de fondo.
	//
	if ( !Director->Is(RELAXED) && !Director->Is(SLEEP) )
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
		if ( Director->Is(SLEEP) )
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
		Director->OnAngryMusicPlay();

		// Reproducimos nuevamente...
		int iDuration = m_nAngry->GetDuration();
		m_nNextAmbientMusic.Start( RandomInt(iDuration+20, iDuration+60) );
	}	
}