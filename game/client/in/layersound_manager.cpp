//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "layersound_manager.h"

#include "tier0/memdbgon.h"

CLayerSoundManager g_LayerSoundManager;
CLayerSoundManager *LayerSoundManager = &g_LayerSoundManager;

#define FOR_EACH_SOUND for ( int i = 0; i < m_nAllSounds.Count(); ++i )
//#define FOR_EACH_LAYER_SOUND( level ) for ( int i = 0; i < m_nSounds[##level##].Count(); ++i )

//====================================================================
// Comandos
//====================================================================

ConVar layersoundmanager_debug( "cl_layersoundmanager_debug", "1" );

#define LAYER_SOUND_DEBUG layersoundmanager_debug.GetBool()

//====================================================================
// [Evento] Se ha cargado un mapa
//====================================================================
void CLayerSoundManager::LevelInitPreEntity()
{
	// Limpiamos la lista
	m_nAllSounds.Purge();
}

//====================================================================
// Pensamiento
//====================================================================
void CLayerSoundManager::Update( float frametime )
{
	float flMaxLayer = LAYER_VERYLOW; 

	// Obtenemos el sonido con más prioridad que se reproduce ahora mismo
	FOR_EACH_SOUND
	{
		CLayerSound *pSound = m_nAllSounds[i];

		if ( !pSound )
			continue;

		// No se esta reproduciendo
		if ( !pSound->IsPlaying() )
			continue;

		if ( pSound->GetLevel() <= flMaxLayer )
			continue;

		// Capa máxima que tiene un sonido reproduciendo
		flMaxLayer = pSound->GetLevel();
	}

	// Hacemos los cambios necesarios a los sonidos
	UpdateSounds( flMaxLayer );
}

//====================================================================
//====================================================================
void CLayerSoundManager::UpdateSounds( float flMaxLevel )
{
	FOR_EACH_SOUND
	{
		CLayerSound *pSound = m_nAllSounds[i];

		if ( !pSound )
			continue;

		if ( !pSound->IsPlaying() )
			continue;

		// Menor prioridad
		if ( pSound->GetLevel() < flMaxLevel )
		{
			// Bloqueamos el volumen y lo bajamos al minimo
			if ( !pSound->IsBlocked() && pSound->m_nSoundPatch )
			{
				pSound->SetBlock( true );
				ENVELOPE_CONTROLLER.SoundChangeVolume( pSound->m_nSoundPatch, 0.01f, 0.8f );

				if ( LAYER_SOUND_DEBUG )
				{
					DevMsg( "[CLayerSoundManager::UpdateSounds] Bloqueando y bajando el volumen del sonido: %s (%f) - Capa maxima: %f \n", pSound->GetSoundName(), pSound->GetLevel(), flMaxLevel );
				}
			}
		}

		// Mayor prioridad
		if ( pSound->GetLevel() >= flMaxLevel )
		{
			// Restauramos el volumen
			if ( pSound->IsBlocked() )
			{
				pSound->SetBlock( false );
				pSound->SetVolume( pSound->GetVolume() );

				if ( LAYER_SOUND_DEBUG )
				{
					DevMsg( "[CLayerSoundManager::UpdateSounds] Restaurando sonido: %s (%f) - Capa maxima: %f \n", pSound->GetSoundName(), pSound->GetLevel(), flMaxLevel );
				}
			}
		}
	}
}

//====================================================================
//====================================================================
void CLayerSoundManager::AddSound( CLayerSound *pSound )
{
	// Ya esta en la lista
	if ( m_nAllSounds.HasElement(pSound) )
		return;

	m_nAllSounds.AddToTail( pSound );
}

//====================================================================
//====================================================================
void CLayerSoundManager::RemoveSound( CLayerSound *pSound )
{
	// No esta en la lista
	if ( !m_nAllSounds.HasElement(pSound) )
		return;

	m_nAllSounds.FindAndRemove( pSound );
}