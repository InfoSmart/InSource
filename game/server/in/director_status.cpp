//==== InfoSmart. Todos los derechos reservados .===========//
//
// Inteligencia artificial encargada de la creación de enemigos (hijos)
// además de poder controlar la música, el clima y otros aspectos
// del juego.
//==========================================================//

#include "cbase.h"
#include "director.h"

#include "in_gamerules.h"

#include "director_music.h"
#include "director_manager.h"

#include "players_manager.h"

//=========================================================
// Devuelve si es posible Dormir
//=========================================================
bool CDirector::CanSleep()
{
	return ( Is(SLEEP) || Is(CLIMAX) || Is(BOSS) || m_bBlockAllStatus ) ? false : true;
}

//=========================================================
// Devuelve si es posible Relajarse
//=========================================================
bool CDirector::CanRelax()
{
	return ( Is(RELAXED) || m_bBlockAllStatus ) ? false : true;
}

//=========================================================
// Devuelve si es posible entrar en Pánico
//=========================================================
bool CDirector::CanPanic()
{
	return ( Is(PANIC) || Is(CLIMAX) || m_bBlockAllStatus ) ? false : true;
}

//=========================================================
// Devuelve si es posible entrar en Jefe
//=========================================================
bool CDirector::CanBoss()
{
	return ( Is(BOSS) || Is(CLIMAX) || m_bBlockAllStatus || Is(PANIC) && !m_hLeft4FinishPanic.HasStarted() ) ? false : true;
}

//=========================================================
// Devuelve si es posible entrar en Climax
//=========================================================
bool CDirector::CanClimax()
{
	return ( Is(CLIMAX) ) ? false : true;
}

//=========================================================
// Devuelve si es un evento de pánico infinito
//=========================================================
bool CDirector::IsInfinitePanic()
{
	if ( m_hLeft4FinishPanic.HasStarted() )
		return false;

	return true;
}

//=========================================================
// Activa el estado Dormido
//=========================================================
void CDirector::Sleep()
{
	// No podemos
	if ( !CanSleep() )
		return;

	// No podemos dormir, pasemos a relajado
	if ( !director_allow_sleep.GetBool() )
	{
		Relaxed();
		return;
	}

	// Dormimos
	Set( SLEEP );
	m_pInfoDirector->OnSleep.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//=========================================================
// Activa el estado Relajado
//=========================================================
void CDirector::Relaxed()
{
	// No podemos
	if ( !CanRelax() )
		return;

	// Nos relajamos
	Set( RELAXED );
	m_pInfoDirector->OnRelaxed.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//=========================================================
// Activa el estado de Pánico
//=========================================================
void CDirector::Panic( CBasePlayer *pCaller, int iSeconds, bool bInfinite )
{
	// No podemos
	if ( !CanPanic() )
		return;

	// No es un evento infinito
	if ( !bInfinite )
	{
		if ( iSeconds <= 0 )
			iSeconds = RandomInt( 15, 30 );

		// Tiempo en el que acabaremos el evento
		m_hLeft4FinishPanic.Start( iSeconds );

		ConColorMsg( Color(138, 8, 8), "[Director] Comenzando evento de panico de %i segundos !!! \n", iSeconds );
	}
	else
	{
		m_hLeft4FinishPanic.Invalidate();

		ConColorMsg( Color(138, 8, 8), "[Director] Comenzando evento de panico infinito !!! \n" );
	}

	// Reiniciamos la información del último evento
	m_iLastPanicDuration	= 0;
	m_iLastPanicDeaths		= 0;
	m_iPanicChilds			= 0;

	// ¡Pánico!
	Set( PANIC );
	m_pInfoDirector->OnPanic.FireOutput( m_pInfoDirector, m_pInfoDirector );

	// Un evento más a la cuenta
	++m_iPanicCount;

	// Reiniciamos para el próximo evento
	RestartPanic();
}

//=========================================================
// Activa el estado de Jefe
//=========================================================
void CDirector::Boss()
{
	// No podemos
	if ( !CanBoss() )
		return;

	// Entramos en Jefe
	Set( BOSS );
	m_pInfoDirector->OnBoss.FireOutput( m_pInfoDirector, m_pInfoDirector );
}

//=========================================================
// Activa el estado de Climax
//=========================================================
void CDirector::Climax( bool bMini )
{
	// No podemos
	if ( !CanClimax() )
		return;

	// Entramos en Climax
	Set( CLIMAX );
	m_pInfoDirector->OnClimax.FireOutput( m_pInfoDirector, m_pInfoDirector );
}