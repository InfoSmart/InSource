//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "layersound_manager.h"

#include "tier0/memdbgon.h"

CLayerSoundManager g_LayerSoundManager;
CLayerSoundManager *LayerSoundManager = &g_LayerSoundManager;

#define FOR_EACH_LAYER for ( int l = 1; l < LAYER_MAX_COUNT; ++l )
#define FOR_EACH_SOUND for ( int i = 0; i < m_nAllSounds.Count(); ++i )
#define FOR_EACH_LAYER_SOUND( level ) for ( int i = 0; i < m_nSounds[##level##].Count(); ++i )

//====================================================================
// Comandos
//====================================================================

ConVar layersoundmanager_debug( "cl_layersoundmanager_debug", "0" );

#define IS_SOUND_DEBUG layersoundmanager_debug.GetBool()

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
	// Limpiamos las listas de sonidos
	FOR_EACH_LAYER
	{
		m_nSounds[l].Purge();
	}

	// Separamos cada sonido en una lista de su capa
	FOR_EACH_SOUND
	{
		CLayerSound *pSound = m_nAllSounds[i];

		if ( !pSound )
			continue;

		AddSound( pSound, pSound->GetLayer() );
	}

	LayerLevel maxLayer = LAYER_VERYLOW; 

	FOR_EACH_LAYER
	{
		FOR_EACH_LAYER_SOUND( l )
		{
			CLayerSound *pSound = m_nSounds[l][i];

			if ( !pSound )
				continue;

			// No se esta reproduciendo
			if ( !pSound->IsPlaying() )
				continue;

			// Capa máxima que tiene un sonido reproduciendo
			maxLayer = pSound->GetLayer();
		}
	}

	// Hacemos los cambios necesarios a los sonidos
	UpdateSounds( maxLayer );
}

//====================================================================
//====================================================================
void CLayerSoundManager::UpdateSounds( LayerLevel iMaxLevel )
{
	// Capas inferiores
	for ( int l = LAYER_VERYLOW; l < iMaxLevel; ++l )
	{
		FOR_EACH_LAYER_SOUND( l )
		{
			CLayerSound *pSound = m_nSounds[l][i];

			if ( !pSound )
				continue;

			// Bloqueamos el volumen y lo bajamos al minimo
			if ( !pSound->IsBlocked() && pSound->m_nSoundPatch )
			{
				pSound->SetBlock( true );
				ENVELOPE_CONTROLLER.SoundChangeVolume( pSound->m_nSoundPatch, 0.01f, 0.8f );

				if ( IS_SOUND_DEBUG )
				{
					DevMsg( "[CLayerSoundManager::UpdateSounds] Bloqueando y bajando el volumen del sonido: %s (%i) - Capa maxima: %i \n", pSound->GetSoundName(), pSound->GetLayer(), (int)iMaxLevel );
				}
			}
		}
	}

	// Capas superiores
	for ( int l = iMaxLevel; l < LAYER_MAX_COUNT; ++l )
	{
		FOR_EACH_LAYER_SOUND( l )
		{
			CLayerSound *pSound = m_nSounds[l][i];

			if ( !pSound )
				continue;

			// Restauramos el volumen
			if ( pSound->IsBlocked() )
			{
				pSound->SetBlock( false );
				pSound->SetVolume( pSound->GetVolume() );

				if ( IS_SOUND_DEBUG )
				{
					DevMsg( "[CLayerSoundManager::UpdateSounds] Restaurando sonido: %s (%i) - Capa maxima: %i \n", pSound->GetSoundName(), pSound->GetLayer(), (int)iMaxLevel );
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

	if ( IS_SOUND_DEBUG )
	{
		DevMsg( "[CLayerSoundManager::AddSound] %s \n", pSound->GetSoundName() );
	}
}

//====================================================================
//====================================================================
void CLayerSoundManager::AddSound( CLayerSound *pSound, LayerLevel iLevel )
{
	// Ya esta en la lista
	if ( m_nSounds[iLevel].HasElement(pSound) )
		return;

	m_nSounds[iLevel].AddToTail( pSound );
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