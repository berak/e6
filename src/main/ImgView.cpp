

#include "ImgView.h"
#include "../Image.h"

#include <string.h>
#include <stdio.h>


Image img;




ImgView::ImgView(const char *fn)
{
	img.load((char*)fn);
    img.swapRGB();
}
ImgView::ImgView(int w, int h, int d, unsigned char *p)
{

    img.init(w,h,d,p);
    //img.topDown();
    img.swapRGB();
}

ImgView::~ImgView()
{
    if ( img.data ) delete []img.data;

}


void ImgView::mouse(HWND hWnd) {
    POINT p;
    RECT  r;
    GetCursorPos( &p );
    ScreenToClient( GetActiveWindow(), &p );
    GetClientRect( GetActiveWindow(), &r );
    p.y = r.bottom - p.y;
    int sx = p.x * img.width / (r.right-r.left);
    int sy = p.y * img.height / (r.bottom-r.top);

    unsigned char cr=0,cg=0,cb=0;
    img.getPixel( sx,sy, cr,cg,cb );
    char txt[180];
    sprintf( txt, "%s (%d %d) (%d %d %d)\n", "pixel", sx, sy, cb,cg,cr );
    SetWindowText( hWnd, txt );
}

BOOL ImgView::message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    int hoff = 20; // sysmenu
    switch (uMsg) {
    case WM_INITDIALOG :
        hwnd = hWnd;
        setClsIcon(hWnd, 499 );
        if ( ! img.data ) break;
        if ( img.width < 32 )
            MoveWindow( hWnd, 60,60, img.width*16, hoff + img.height*16, 1 );
        else if ( img.width < 64 )
            MoveWindow( hWnd, 60,60, img.width*4, hoff + img.height*4, 1 );
        else if ( img.width < 256 )
            MoveWindow( hWnd, 60,60, img.width*2, hoff + img.height*2, 1 );
        else
            MoveWindow( hWnd, 60,60, img.width, hoff + img.height, 1 );
        break;
    case WM_LBUTTONDOWN:
        mouse(hWnd);
        break;
    case WM_SIZE :
    case WM_PAINT :
        if ( img.data ) 
            display( hWnd );
        break;
    case WM_COMMAND :
	    if (uMsg == WM_COMMAND && wParam == IDCANCEL)
            return EndDialog( hWnd, wParam );
        break;
    }
	return FALSE;
}

void ImgView::display( HWND hwnd ) {
    if ( ! img.data ) return;
    int biz= sizeof(BITMAPINFOHEADER);
    BITMAPINFOHEADER bi;
    bi.biSize     = biz;
    bi.biWidth    = img.width;
    bi.biHeight   = img.height;
    bi.biPlanes   = 1;
    bi.biBitCount = img.bpp * 8;
    bi.biSizeImage = img.byteSize();
    bi.biCompression = BI_RGB;
    RECT rc;
    HDC hdc = GetDC( hwnd );
    GetClientRect( hwnd, &rc );
    StretchDIBits( hdc,
        rc.left, rc.top, rc.right, rc.bottom,       // dst
        0, 0, img.width, img.height, // src
        img.data, 
        (BITMAPINFO*)&bi, 
        DIB_RGB_COLORS, 
        SRCCOPY );    
    ReleaseDC( hwnd, hdc );    
}



#ifdef STANDALONE
#include "resource.h"
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE previnst, LPSTR cmdline, int cmdshow) 
{
	char *fn = cmdline;
	if (fn && fn[0] ) {
		if ( fn[0] == '\"' ) fn ++; 
		int n=strlen(fn);
		while(n--) {
			if ( fn[n] == ' ' || fn[n] == '\t' || fn[n] == '\"' ) 
				fn[n] = 0;
			else break;
		}   
	} else 
		fn = fileBox(0);

	ImgView iv(fn);
    iv.open( IDD_IMGVIEW, hinst, 0 );
	
	return 0;
}
#endif
