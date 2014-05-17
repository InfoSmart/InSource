//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"

#include "director.h"
#include "director_music.h"

#include "soundent.h"
#include "engine/IEngineSound.h"

#include "tier0/memdbgon.h"

// Nuestro ayudante.
#ifndef CUSTOM_DIRECTOR_MUSIC
	CDirectorMusic g_DirectorMusic;
	CDirectorMusic *DirectorMusic = &g_DirectorMusic;
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
}

//=========================================================
//=========================================================
void CDirectorMusic::OnNewMap()
{
	m_nBossMusic		= CreateLayerSound( MUSIC_BOSS, LAYER_HIGH, true );
	m_nClimaxMusic		= CreateLayerSound( MUSIC_CLIMAX, LAYER_HIGH, true );
	m_nGameoverMusic	= CreateLayerSound( MUSIC_GAMEOVER, LAYER_TOP );
}

//=========================================================
//=========================================================
void CDirectorMusic::Think()
{
	//m_nGameoverMusic->Update();
}

//=========================================================
// Para de reproducir la música
//=========================================================
void CDirectorMusic::Stop()
{
	if ( !m_nBossMusic )
		return;

	m_nGameoverMusic->Stop();
	m_nBossMusic->Stop();
	m_nClimaxMusic->Stop();
}

//=========================================================
// Apaga al Director de Música
//=========================================================
void CDirectorMusic::Shutdown()
{
	Stop();

	UTIL_Remove( m_nGameoverMusic );
	UTIL_Remove( m_nBossMusic );
	UTIL_Remove( m_nClimaxMusic );
}

//=========================================================
// Procesa la música
//=========================================================
void CDirectorMusic::Update()
{
	//
	// PARTIDA TERMINADA
	//
	if ( Director->IsStatus(ST_GAMEOVER) )
		m_nGameoverMusic->Play();
	else
		m_nGameoverMusic->Fadeout();

	//
	// CLIMAX
	//
	if ( Director->IsStatus(ST_FINALE) )
		m_nClimaxMusic->Play();
	else
		m_nClimaxMusic->Fadeout();

	//
	// JEFE
	//
	if ( Director->IsStatus(ST_BOSS) )
		m_nBossMusic->Play();
	else
		m_nBossMusic->Fadeout();
}