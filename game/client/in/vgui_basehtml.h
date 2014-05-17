//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef VGUI_BASEHTML_H
#define VGUI_BASEHTML_H

#pragma once

#include "vgui_controls/Panel.h"
#include "VAwesomium.h"

//====================================================================
// >> CBaseHTML
// Clase base para todos los elementos VGUI que utilizan HTML
//====================================================================
class CBaseHTML : public VAwesomium
{
public:
	DECLARE_CLASS_SIMPLE( CBaseHTML, VAwesomium );

	CBaseHTML( vgui::Panel *parent, const char *name, const char *htmlFile );
	CBaseHTML( vgui::VPANEL parent, const char *name, const char *htmlFile );

	virtual void Init( const char *htmlFile );
	virtual void Think();
	virtual void WebThink();

	virtual bool CanPaint() { return true; }
	virtual void Paint();
	virtual void PaintBackground();

	virtual void OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url );
	virtual void OnMethodCall( Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args );

protected:
	Awesomium::JSObject m_pGameObject;
	const char *m_pSteamID;
	float m_flNextThink;
};

#endif