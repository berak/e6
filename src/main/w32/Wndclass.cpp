#include "wndclass.h"


#include <stdio.h>

namespace w32
{

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// CDialog
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	BOOL CALLBACK CDialog::dlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if ( uMsg == WM_INITDIALOG ) // get this-ptr from lparam
			SetWindowLong( hWnd, GWL_USERDATA, lParam );     
		CDialog *dlg = (CDialog *)GetWindowLong(hWnd, GWL_USERDATA);
		if ( ! dlg ) return FALSE;
		return dlg->message(hWnd, uMsg, wParam, lParam);
	}
	
	
	BOOL CDialog::open(int nID, HINSTANCE hInstance, HWND hWndParent)
	{
		return DialogBoxParam(hInstance, MAKEINTRESOURCE(nID), hWndParent, dlgProc, (LPARAM)this); 
	}
	
	BOOL CDialog::message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
	{
		if (uMsg == WM_COMMAND && wParam == IDCANCEL)
			return EndDialog( hWnd, wParam );
		return FALSE;
	}
	
	
	
	
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// CWindow
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	
	
	HWND CWindow::create( const char *cls, const char *title, const int x, const int y, const int w, const int h, int style, const int menu, const HWND parent, const HINSTANCE hi, const int color, const int exStyle ) 
	{
		WNDCLASS wndclass;
	
		if ( ! (GetClassInfo( hi, cls, &wndclass) ) ) {
			memset( &wndclass, 0, sizeof( WNDCLASS ) );
			wndclass.lpszClassName = cls;
			wndclass.hInstance     = hi;
			wndclass.lpfnWndProc   = winProc;
			wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
			wndclass.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
			wndclass.hbrBackground = (HBRUSH)color; 
			//wndclass.lpszMenuName	= (LPCSTR)IDR_MENU1;
			RegisterClass(&wndclass);
		}
		HWND win = CreateWindowEx( exStyle, cls, title, style, x, y, w, h, parent, (HMENU)menu, hi, this );
	//    if ( menu ) ShowMenu( win, menu );
		ShowWindow( win, SW_SHOWNORMAL );
	
		return win;
	}
	
	
	LRESULT CALLBACK CWindow::winProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
	{
		if ( uMsg == WM_CREATE ) {
			CREATESTRUCT *cr = (CREATESTRUCT*)lParam;
			if ( cr )
				SetWindowLong (hWnd, GWL_USERDATA, (long)(cr->lpCreateParams) );
		}
	
		CWindow *cwin = (CWindow*)(GetWindowLong( hWnd, GWL_USERDATA ));
	
		if ( cwin ) 
			return cwin->message(hWnd,uMsg,wParam,lParam);
	
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}
	
	
	LRESULT CALLBACK CWindow::message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
	{
		if ( uMsg == WM_DESTROY ) {
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}
	
	
	
	void CWindow::main() 
	{
		MSG msg;
		while( GetMessage(&msg,0,0,0) ) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	
	
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Helpers
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	int alert( char *caption, char *format, ... ) 
	{
		static char buffer[2048] = {0};
		va_list args;
		va_start( args, format );
		vsprintf( buffer, format, args );
		va_end( args );
		MessageBox(GetActiveWindow(), buffer, caption, 0);   
		return 0;
	}   
	
	
	
	
	char *fileBox( bool SAV, char *startPath, char *filter, HWND parent ) 
	{
		static char _fileName[0xfff] = {0};
		if ( startPath )
			strcpy( _fileName, startPath );

		OPENFILENAME of = {0};
		of.lStructSize = sizeof( OPENFILENAME );
		of.nMaxFile    = 0xfff;
		of.lpstrFile   = _fileName;
		of.lpstrFilter = filter;
		of.nFilterIndex= 1;
		of.hwndOwner   = parent?parent:GetActiveWindow();
		of.hInstance   = getHinst(of.hwndOwner); //GetModuleHandle(__argv[0]);
		of.Flags = OFN_LONGNAMES | ( SAV ? OFN_OVERWRITEPROMPT :  OFN_FILEMUSTEXIST );
		int res  = SAV ? GetSaveFileName( &of ) : GetOpenFileName( &of );
		if ( ! res || CommDlgExtendedError() )
			return 0; // cancelled.
		strcpy( _fileName, of.lpstrFile );
		return _fileName;  
	}
	
	
	
	
	BOOL fullScreen(HWND hWnd, bool on)
	{
		static long oldStyle;
		static RECT oldSize;
		DEVMODE dmSettings = {0};
		if ( ! EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dmSettings) )
			return FALSE;
		if ( on ) {
			oldStyle = GetWindowLong( hWnd, GWL_STYLE );	
			GetWindowRect( hWnd, &oldSize );
			SetWindowLong( hWnd, GWL_STYLE, WS_POPUP );
			SetWindowPos( hWnd,HWND_TOP, 0,0, dmSettings.dmPelsWidth, dmSettings.dmPelsHeight, SWP_SHOWWINDOW );
			return true;
		}
		SetWindowLong( hWnd, GWL_STYLE, oldStyle );
		//SetWindowPos( hWnd,HWND_TOP, oldSize.left, oldSize.top, oldSize.right-oldSize.left, oldSize.bottom-oldSize.top, SWP_SHOWWINDOW );
		SetWindowPos( hWnd,HWND_TOP, 0,0, dmSettings.dmPelsWidth, dmSettings.dmPelsHeight, SWP_SHOWWINDOW ); // ( stupid way to clear the desktop ... )
		SetWindowPos( hWnd,HWND_TOP, oldSize.left, oldSize.top, oldSize.right-oldSize.left, oldSize.bottom-oldSize.top, SWP_SHOWWINDOW );
		// this crashes NT4.
		// UpdateWindow( GetDesktopWindow() );
		return false;
	}
	
	
	
	
};
