//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_GAMEUI_HUD_H
#define IN_GAMEUI_HUD_H

#pragma once

#include "vgui_basehtml.h"
#include "weapon_inbase.h"

//====================================================================
// Clase base para crear el HUD en Web
//====================================================================
class CGameHUDWeb : public CBaseHTML
{
public:
	DECLARE_CLASS_SIMPLE( CGameHUDWeb, CBaseHTML );

	CGameHUDWeb( vgui::Panel *parent );
	CGameHUDWeb( vgui::VPANEL parent );

	virtual void Init();

	virtual void OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url );
	virtual void OnMethodCall( Awesomium::WebView* pCaller, unsigned int pObjectID, const Awesomium::WebString& pMethod, const Awesomium::JSArray& args );

	virtual void WebThink();
	virtual void UpdateWeaponSelection();

	virtual int GetHudHiddenBits();

	virtual bool CanPaint();

protected:
	Awesomium::JSObject m_pGameHUDObject;
	int m_iWeaponCount;
};

extern CGameHUDWeb *HUDWeb;

#endif