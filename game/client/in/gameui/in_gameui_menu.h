//==== InfoSmart 2014. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef GU_MENU_H
#define GU_MENU_H

#pragma once

#include "vgui_basehtml.h"

class CGameUIPanelWeb : public CBaseHTML
{
public:
	DECLARE_CLASS_SIMPLE( CGameUIPanelWeb, CBaseHTML );

	CGameUIPanelWeb( vgui::VPANEL parent );

	virtual bool CanPaint();

	virtual void OnGameUIActivated();
	virtual void OnGameUIHidden();

	virtual void OnDisconnectFromServer( uint8 eSteamLoginFailure );
	virtual void OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog );
	virtual void OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason );

	virtual bool UpdateProgressBar( float progress, const char *statusText );
	virtual bool SetShowProgressText( bool show );
	virtual void SetProgressLevelName( const char *levelName );

public:
	bool m_bIsPaused;
};

#endif