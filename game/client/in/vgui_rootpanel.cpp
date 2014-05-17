#include "cbase.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "vgui_controls/Panel.h"
#include "vgui/IVgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//=========================================================
// >> C_InRootPanel
//=========================================================
class C_InRootPanel : public vgui::Panel
{
public:
	typedef vgui::Panel BaseClass;
	C_InRootPanel( vgui::VPANEL parent, int iSlot );
	
	virtual void OnThink();
	virtual void PaintTraverse( bool Repaint, bool allowForce = true );

protected:
	int	m_nSplitSlot;
};

static C_InRootPanel *g_pRootPanel[ MAX_SPLITSCREEN_PLAYERS ];
static C_InRootPanel *g_pFullscreenRootPanel;

C_InRootPanel::C_InRootPanel( vgui::VPANEL parent, int iSlot ) : BaseClass( NULL, "InSource Root Panel" ), m_nSplitSlot( iSlot )
{
	SetParent( parent );
	SetPaintEnabled( false );
	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );

	// This panel does post child painting
	SetPostChildPaintEnabled( true );

	// Make it screen sized
	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

	// Ask for OnTick messages
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

void C_InRootPanel::OnThink()
{
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( m_nSplitSlot );
	BaseClass::OnThink();
}

void C_InRootPanel::PaintTraverse( bool Repaint, bool allowForce )
{
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( m_nSplitSlot);
	BaseClass::PaintTraverse( Repaint, allowForce );
}

void VGui_GetPanelList( CUtlVector<Panel *> &list )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		list.AddToTail( g_pRootPanel[ i ] );
	}
}

void VGUI_CreateClientDLLRootPanel( void )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		g_pRootPanel[ i ] = new C_InRootPanel( enginevgui->GetPanel( PANEL_CLIENTDLL ), i );
	}

	g_pFullscreenRootPanel = new C_InRootPanel( enginevgui->GetPanel( PANEL_CLIENTDLL ), 0 );
	g_pFullscreenRootPanel->SetZPos( 1 );
}

void VGUI_DestroyClientDLLRootPanel( void )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		delete g_pRootPanel[ i ];
		g_pRootPanel[ i ] = NULL;
	}

	delete g_pFullscreenRootPanel;
	g_pFullscreenRootPanel = NULL;
}

vgui::VPANEL VGui_GetClientDLLRootPanel( void )
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return g_pRootPanel[ GET_ACTIVE_SPLITSCREEN_SLOT() ]->GetVPanel();
}

vgui::Panel *VGui_GetFullscreenRootPanel( void )
{
	return g_pFullscreenRootPanel;
}

vgui::VPANEL VGui_GetFullscreenRootVPANEL( void )
{
	return g_pFullscreenRootPanel->GetVPanel();
}
