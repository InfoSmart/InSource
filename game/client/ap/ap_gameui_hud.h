//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_GAMEUI_HUD_H
#define AP_GAMEUI_HUD_H

#pragma once

#include "in_gameui_hud.h"

//====================================================================
// CHudPanel
//
// >>
//====================================================================
class CAP_GameHUD : public CGameHUDWeb
{
public:
	DECLARE_CLASS_SIMPLE( CAP_GameHUD, CGameHUDWeb );

	CAP_GameHUD( vgui::VPANEL parent );
	virtual void Init();

	virtual void WebThink();
	virtual void UpdateInventory();

	virtual void OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url );
	virtual void OnMethodCall( Awesomium::WebView* pCaller, unsigned int pObjectID, const Awesomium::WebString& pMethod, const Awesomium::JSArray& args );

protected:
	int m_iItemsCount;
};

#endif