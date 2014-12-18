//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "c_nb.h"

#undef CNextBot

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//====================================================================
// Información y Red
//====================================================================

IMPLEMENT_CLIENTCLASS_DT( C_NextBot, DT_NextBot, CNextBot )

	RecvPropBool( RECVINFO(m_bIsRunning) )

END_RECV_TABLE()

//====================================================================
// Constructor
//====================================================================
C_NextBot::C_NextBot()
{
	//m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;
}

//====================================================================
// Destructor
//====================================================================
C_NextBot::~C_NextBot()
{
	// Liberamos el Anim State
	if ( m_nBotAnimState )
		m_nBotAnimState->Release();
}

//====================================================================
//====================================================================
void C_NextBot::Spawn()
{
	BaseClass::Spawn();

	// Animation State
	m_nBotAnimState	= CreateNextBotAnimationState( this ); 

	// 
	m_nBlinkTimer.Invalidate();
}

//====================================================================
//====================================================================
void C_NextBot::UpdateClientSideAnimation()
{
	if ( IsDormant() )
		return;

	// Actualizamos las animaciones
	m_nBotAnimState->Update();

	BaseClass::UpdateClientSideAnimation();
}