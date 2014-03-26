//==== InfoSmart. Todos los derechos reservados .===========//

#include "cbase.h"

#include "director.h"
#include "director_music.h"

#ifdef APOCALYPSE
	#include "ap_director.h"
#endif

#include "soundent.h"
#include "engine/IEngineSound.h"

#include "tier0/memdbgon.h"

#ifndef CUSTOM_DIRECTOR_MUSIC
	// Nuestro ayudante.
	CDirectorMusic g_DirMusic;
	CDirectorMusic *DirectorMusic = &g_DirMusic;
#endif

//=========================================================
// Comandos de consola
//=========================================================

ConVar director_music_boss( "director_music_boss", "Director.Boss", FCVAR_SERVER | FCVAR_ARCHIVE );
ConVar director_music_climax( "director_music_climax", "Director.Climax", FCVAR_SERVER | FCVAR_ARCHIVE );
ConVar director_music_gameover( "director_music_gameover", "Director.Gameover", FCVAR_SERVER | FCVAR_ARCHIVE );


#define MUSIC_BOSS		director_music_boss.GetString()
#define MUSIC_CLIMAX	director_music_climax.GetString()
#define MUSIC_GAMEOVER	director_music_gameover.GetString()

//=========================================================
// Guardado de objetos necesarios en caché
//=========================================================
void CDirectorMusic::Precache()
{
	CBaseEntity::PrecacheScriptSound( MUSIC_BOSS );
	CBaseEntity::PrecacheScriptSound( MUSIC_CLIMAX );
	CBaseEntity::PrecacheScriptSound( MUSIC_GAMEOVER );
}

//=========================================================
// Inicializa al Director de Música
//=========================================================
void CDirectorMusic::Init()
{
	Precache();

	m_nBossMusic		= new EnvMusic( MUSIC_BOSS );
	m_nClimaxMusic		= new EnvMusic( MUSIC_CLIMAX );
	m_nGameoverMusic	= new EnvMusic( MUSIC_GAMEOVER );
}

//=========================================================
//=========================================================
void CDirectorMusic::OnNewMap()
{
}

//=========================================================
//=========================================================
void CDirectorMusic::Think()
{
	m_nGameoverMusic->Update();
}

//=========================================================
// Para de reproducir la música
//=========================================================
void CDirectorMusic::Stop()
{
	if ( !m_nBossMusic )
		return;

	m_nBossMusic->Fadeout();
	m_nClimaxMusic->Fadeout();
}

//=========================================================
// Apaga al Director de Música
//=========================================================
void CDirectorMusic::Shutdown()
{
	Stop();

	delete m_nGameoverMusic;
	delete m_nBossMusic;
	delete m_nClimaxMusic;
}

//=========================================================
// Procesa la música
//=========================================================
void CDirectorMusic::Update()
{
	//
	// PARTIDA TERMINADA
	//
	if ( Director->Is(GAMEOVER) )
		m_nGameoverMusic->Play();
	else
		m_nGameoverMusic->Fadeout( 2.0f );

	//
	// CLIMAX
	//
	if ( Director->Is(CLIMAX) )
		m_nClimaxMusic->Play();
	else
		m_nClimaxMusic->Fadeout();

	//
	// JEFE
	//
	if ( Director->Is(BOSS) )
		m_nBossMusic->Play();
	else
		m_nBossMusic->Fadeout();
}