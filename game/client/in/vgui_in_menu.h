//==== InfoSmart. Todos los derechos reservados .===========//

#ifndef VGUI_IN_MENU_H
#define VGUI_IN_MENU_H

#include "VAwesomium.h"
#include "vgui_controls/Panel.h"

#include "GameUI/IGameUI.h"

//=========================================================
// CBaseHTML
// >> Clase base para todos los VGUI en HTML
//=========================================================
class CBaseHTML : public VAwesomium
{
public:
	DECLARE_CLASS_SIMPLE( CBaseHTML, VAwesomium );

	CBaseHTML( vgui::VPANEL parent, const char *name, const char *htmlFile );

	virtual void Think();
	virtual void PaintBackground();

	virtual void OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url );
	virtual void OnMethodCall( Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args );
};

namespace vgui
{
	class Panel;
}

//=========================================================
// >> ISMenu
// Interfaz para el Menu
//=========================================================
class ISMenu
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy() = 0;
	virtual void		SetVisible( bool bVisible ) = 0;
	virtual vgui::VPANEL GetPanel() = 0;
};

extern ISMenu *SMenu;

#endif