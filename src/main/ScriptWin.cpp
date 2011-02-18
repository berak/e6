#include "../main/ScriptWin.h"
#include "resource.h"

#include <stdio.h>


//
// this one does nothing,
// all events are posted 
// to the main window !
//



ScriptWin::~ScriptWin() {}
ScriptWin::ScriptWin() {hwnd=code=out=parent=0;}

HWND ScriptWin::create( HWND prnt, const char *imgstrip )  
{ 
    HINSTANCE hinst = w32::getHinst(prnt);
    parent = prnt;
	hwnd = CWindow::create( "script","script", 0,30,400,600, WS_CHILD|WS_VISIBLE, 0, prnt, hinst, 0, WS_EX_STATICEDGE );
	//hwnd = CWindow::create( "script","script", 0,0,400,300, WS_OVERLAPPEDWINDOW|WS_SYSMENU|WS_BORDER|WS_CAPTION, 0, prnt, hinst, 0, WS_EX_STATICEDGE );
	/*hwnd = CreateWindowEx(
            WS_EX_STATICEDGE,
            "static","",
            WS_CHILD|WS_VISIBLE,
            0,30,200,400, 
            prnt, 0,
            hinst, 0 );
        tool.create( hWnd, ID_TOOL, IDB_BITMAP1  );

        tool.add( 0, IDM_SCRIPT_RUN, 10, TBSTYLE_BUTTON );
        tool.add( 0, IDM_SCRIPT_LOAD, 6, TBSTYLE_BUTTON );
        tool.add( 0, IDM_SCRIPT_SAVE, 7, TBSTYLE_BUTTON );
    */

    code = CreateWindowEx(
            WS_EX_STATICEDGE,
            "edit", "",
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_WANTRETURN,
            2,30,580,100,
            hwnd, (HMENU)IDC_EDIT1,
            hinst,
            0 );
    out = CreateWindowEx(
            WS_EX_STATICEDGE,
            "edit", "",
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_READONLY|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL,
            2,134,580,340,
            hwnd, (HMENU)IDC_EDIT2,
            hinst,
            0 );
	HFONT hFont = (HFONT) GetStockObject( ANSI_VAR_FONT ); 
	SendMessage( code, WM_SETFONT, (WPARAM) hFont, 1 );
	SendMessage( out, WM_SETFONT, (WPARAM) hFont, 1 );

	tool.create( hwnd, 1978, (char*)imgstrip, 16,16 );
    tool.add( 0, IDM_SCRIPT_RUN, 10, TBSTYLE_BUTTON );
    tool.add( 0, IDM_SCRIPT_CLEAR, 17, TBSTYLE_BUTTON );
    tool.add( 0, 0,0,0 );
    tool.add( 0, IDM_SCRIPT_LOAD, 6, TBSTYLE_BUTTON );
    tool.add( 0, IDM_SCRIPT_SAVE, 7, TBSTYLE_BUTTON );
    tool.add( 0, 0,0,0 );
    tool.add( 0, IDM_FILE_SCRIPTENGINE, 16, TBSTYLE_BUTTON );
    tool.add( 0, IDM_SCRIPT_VERSION, 18, TBSTYLE_BUTTON );

    w32::setClsIcon( hwnd, ICO0 );

	on_resize(hwnd);

	return hwnd;
}



LRESULT CALLBACK ScriptWin::message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	switch ( uMsg  ) {
	case WM_CREATE:
		on_create( hWnd );
		return 1; 
	case WM_SIZE :
		on_resize( hWnd );
		return 1;
	case WM_DESTROY:
		on_destroy( hWnd );
		return 1;
	case WM_NOTIFY:
		on_notify( hWnd, uMsg, wParam, lParam );
		return 1;
	case WM_COMMAND:
		switch ( LOWORD(wParam)  ) {
			case IDM_SCRIPT_RUN:
			case IDM_SCRIPT_CLEAR:
			case IDM_SCRIPT_VERSION:
			case IDM_SCRIPT_LOAD:
			case IDM_SCRIPT_SAVE:
			case IDM_FILE_SCRIPTENGINE: 
				PostMessage( parent, uMsg, wParam, lParam );
				return 1;
		}
		break;
//        case WM_LBUTTONDOWN :
//            SetCapture(hWnd);
//            return 1;
	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

void ScriptWin::on_destroy( HWND hWnd ) { hwnd=code=out=parent=0;}
void ScriptWin::on_create( HWND hWnd ) {}
void ScriptWin::on_resize( HWND hWnd ) 
{
//	MessageBox(hWnd,"kkk","resize",0);
	RECT rc,rtb;
	GetClientRect(hWnd, &rc);
	tool.update(0,0);
	GetWindowRect(tool.hwnd, &rtb);
	int th   = rtb.bottom - rtb.top;
	int w    = rc.right  - rc.left;
	int h    = rc.bottom - rc.top - th;
	int hin  = h * 3 / 10;
	int hout = h * 7 / 10; 
	MoveWindow( code, 0, th, w, hin, 1 );
	MoveWindow( out , 0, th + hin, w, hout, 1 );
}

void ScriptWin::on_notify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{	
	static char *tt[] = {
		"Run Script",
		"Clear Output",
		"Load Script from File",
		"Load ScriptEngine",
		"Print ScriptVersion",
		0            
	};
	TOOLTIPTEXT *pd = (TOOLTIPTEXT *)lParam;
	if ( (int)pd->hdr.code  == TTN_NEEDTEXT ) {
		switch( pd->hdr.idFrom ) {
			case IDM_SCRIPT_RUN:			pd->lpszText = tt[0];       break;
			case IDM_SCRIPT_CLEAR:			pd->lpszText = tt[1];       break;
			case IDM_SCRIPT_LOAD:			pd->lpszText = tt[2];       break;
			case IDM_FILE_SCRIPTENGINE:     pd->lpszText = tt[3];       break;
			case IDM_SCRIPT_VERSION:	    pd->lpszText = tt[4];       break;
		}
	}
}

// e6::Logger
bool ScriptWin::printLog( const char * s )  
{
	char wt[8024];
	GetWindowText( out, wt, 8024 );
	if ( strlen(wt) + strlen(s) < 8023 )
		strcat(wt,s);
	else
		strcpy(wt,s);

	SetWindowText( out, wt );
	printf( s );

	return 1;
}
