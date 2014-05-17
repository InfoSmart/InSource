#include "cbase.h"
#include "inputsystem/iinputsystem.h"

#include <VAwesomium.h>
#include <vgui_controls/Controls.h>

#define DEPTH 4

using namespace vgui;
using namespace Awesomium;

int VAwesomium::m_iNumberOfViews = 0;
static Awesomium::WebSession *m_WebSession;

//=========================================================
// Constructor
//=========================================================
VAwesomium::VAwesomium( Panel *parent, const char *panelName ) : Panel( parent, panelName )
{
	m_iNumberOfViews++;
	
	m_iTextureId	= surface()->CreateNewTextureID( true );
	m_WebCore		= WebCore::instance();

	// Aún no existe una instancia de Awesomium
	if ( !m_WebCore )
	{
		// Configuración del navegador
		WebStringArray options;
		options.Push( WSLit("--disable-3d-apis") );
		options.Push( WSLit("--disable-file-system") );
		options.Push( WSLit("--disable-geolocation") );
		options.Push( WSLit("--disable-gl-multisampling") );
		options.Push( WSLit("--disable-glsl-translator") );

		// Configuración del proceso
		WebConfig config;
		config.log_level							= kLogLevel_Verbose;
		config.remote_debugging_port				= 1337;
		//config.additional_options					= options;
		config.reduce_memory_usage_on_navigation	= true;

		// Creamos la instancia
		m_WebCore = WebCore::Initialize( config );
	}

	// Configuración para las sesiones
	WebPreferences m_WebPrefs;
	m_WebPrefs.enable_dart							= false;
	m_WebPrefs.enable_plugins						= false;
	m_WebPrefs.enable_gpu_acceleration				= true;
	m_WebPrefs.enable_web_security					= false;
	m_WebPrefs.allow_scripts_to_open_windows		= false;
	m_WebPrefs.allow_scripts_to_close_windows		= false;
	m_WebPrefs.allow_running_insecure_content		= true;

	// Creamos una sesión
	if ( !m_WebSession )
		m_WebSession = m_WebCore->CreateWebSession( WSLit(VarArgs("%s\\%s", engine->GetGameDirectory(), "sessions")), m_WebPrefs );

	// Creamos una vista
	m_WebView = m_WebCore->CreateWebView( GetTall(), GetWide(), m_WebSession );
	m_WebView->set_js_method_handler( this );
	m_WebView->set_load_listener( this );
	m_WebView->SetTransparent( true );

	SetPaintEnabled( true );
	SetPaintBackgroundEnabled( false );
}

//=========================================================
// Destructor
//=========================================================
VAwesomium::~VAwesomium()
{
	m_iNumberOfViews--;
	
	// Destruimos la vista y la sesión
	if ( m_WebView )
		m_WebView->Destroy();
	
	// Apagamos la instancia/proceso
	if ( m_WebCore && m_iNumberOfViews <= 0 )
	{
		if ( m_WebSession )
			m_WebSession->Release();

		m_WebCore->Shutdown();

		m_WebSession	= NULL;
		m_WebCore		= NULL;
	}

	m_WebView		= NULL;
	m_BitmapSurface = NULL;
}

Awesomium::WebView* VAwesomium::GetWebView()
{
	return m_WebView;
}

void VAwesomium::ExecuteJavaScript(const char *script, const char *frame_xpath)
{
	m_WebView->ExecuteJavascript(WSLit(script), WSLit(frame_xpath));
}

void VAwesomium::Think()
{
	m_WebCore->Update();
}

void VAwesomium::Paint()
{
	BaseClass::Paint();

	m_BitmapSurface = ( BitmapSurface * )m_WebView->surface();

	if ( m_BitmapSurface && m_iNearestPowerWidth + m_iNearestPowerHeight > 0 )
	{
		AllocateViewBuffer();
		DrawBrowserView();
	}
}

int VAwesomium::NearestPowerOfTwo(int v)
{
	// http://stackoverflow.com/questions/466204/rounding-off-to-nearest-power-of-2
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

void VAwesomium::AllocateViewBuffer()
{
	//2048 - 1024 - 8388608 
	//DevMsg( "[AllocateViewBuffer] %i - %i - %i \n", m_iNearestPowerWidth, m_iNearestPowerHeight, (m_iNearestPowerWidth * m_iNearestPowerHeight * DEPTH) );

	//
	// FIXME: Esto consume 20fps !!!
	//

	unsigned char* buffer = new unsigned char[m_iNearestPowerWidth * m_iNearestPowerHeight * DEPTH];
	m_BitmapSurface->CopyTo( buffer, m_BitmapSurface->width() * DEPTH, DEPTH, true, false );

	vgui::surface()->DrawSetTextureRGBA( m_iTextureId, buffer, m_BitmapSurface->width(), m_BitmapSurface->height() );

	delete buffer;
	buffer = NULL;
}

void VAwesomium::DrawBrowserView()
{
	//
	// FIXME: Esto consume 10 fps
	//

	vgui::surface()->DrawSetTexture( m_iTextureId );
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );

	vgui::surface()->DrawTexturedSubRect( 0, 0, m_BitmapSurface->width(), m_BitmapSurface->height(), 0.0f, 0.0f, 1, 1 );
}

void VAwesomium::OnCursorMoved( int x, int y )
{
	m_WebView->InjectMouseMove( x, y );
}

void VAwesomium::OnRequestFocus( vgui::VPANEL subFocus, vgui::VPANEL defaultPanel )
{
	BaseClass::OnRequestFocus( subFocus, defaultPanel );
	m_WebView->Focus();
}

void VAwesomium::OnMousePressed(MouseCode code)
{
	MouseButtonHelper( code, false );
}

void VAwesomium::OnMouseReleased(MouseCode code)
{
	MouseButtonHelper( code, true );
}

void VAwesomium::MouseButtonHelper(MouseCode code, bool isUp)
{
	MouseButton mouseButton;

	switch (code)
	{
	case MOUSE_RIGHT:
		mouseButton = kMouseButton_Right;
		break;
	case MOUSE_MIDDLE:
		mouseButton = kMouseButton_Middle;
		break;
	default: // MOUSE_LEFT:
		mouseButton = kMouseButton_Left;
		break;
	}

	isUp ? m_WebView->InjectMouseUp(mouseButton) : m_WebView->InjectMouseDown(mouseButton);
}

void VAwesomium::OnMouseWheeled( int delta )
{
	m_WebView->InjectMouseWheel( delta * WHEEL_DELTA, 0 );
}

void VAwesomium::OnKeyTyped(wchar_t unichar)
{
	WebKeyboardEvent event;

	event.text[0] = unichar;
	event.type = WebKeyboardEvent::kTypeChar;
	m_WebView->InjectKeyboardEvent(event);
}

void VAwesomium::KeyboardButtonHelper(KeyCode code, bool isUp)
{
	WebKeyboardEvent event;

	event.virtual_key_code = inputsystem->ButtonCodeToVirtualKey(code);
	event.type = isUp ? WebKeyboardEvent::kTypeKeyUp : WebKeyboardEvent::kTypeKeyDown;

	m_WebView->InjectKeyboardEvent(event);
}

void VAwesomium::OnKeyCodePressed(KeyCode code)
{
	KeyboardButtonHelper(code, false);
}

void VAwesomium::OnKeyCodeReleased(KeyCode code)
{
	KeyboardButtonHelper(code, true);
}

void VAwesomium::ResizeView()
{
	m_iNearestPowerWidth	= NearestPowerOfTwo( GetWide() );
	m_iNearestPowerHeight	= NearestPowerOfTwo( GetTall() );

	m_WebView->Resize( GetWide(), GetTall() );
}

void VAwesomium::OpenURL(const char *address)
{
	m_WebView->LoadURL(WebURL(WSLit("about:blank")));
	m_WebView->LoadURL(WebURL(WSLit(address)));

	ResizeView();
}

void VAwesomium::PerformLayout()
{
	BaseClass::PerformLayout();
	ResizeView();
}