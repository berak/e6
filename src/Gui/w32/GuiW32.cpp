#include "../e6/e6_impl.h"
#include "../Gui.h"

#include <windows.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>



using e6::uint;
using e6::ClassInfo;

HMODULE hDll = 0;


namespace GuiW32
{
	static struct _
	{
		_() { InitCommonControls(); }
	} __;

	// helper:
	inline  
	HINSTANCE _getHinst( HWND hWnd )
	{
		return reinterpret_cast<HINSTANCE> (GetWindowLong(hWnd, GWL_HINSTANCE));
	}  


	template < class Super, class Impl >
    struct CBaseWindow
        : e6::CName< Super, Impl >
    {
        Gui::Window::Resized * cbResize; 
		HWND hwnd;
		HWND parent;
		uint x,y,w,h, id;
		
		CBaseWindow() 
			: cbResize(0)
			, hwnd(0),parent(0)
			, x(0),y(0),w(0),h(0) , id(0)
		{}

		~CBaseWindow()
		{
//			E_RELEASE( cbResize );
		}
			
        virtual void * getWindow()
		{
			printf( __FUNCTION__ " :  %x\n", hwnd );
			return (void*)(hwnd);
		}
        virtual void * getParent()   
		{
			return (void*)(parent = GetParent( hwnd ));
		}
        virtual uint setParent( void * pr )
		{
			printf( __FUNCTION__ " :  %x\n", hwnd );
			parent = (HWND)pr;
			return 1;
		}


        virtual uint setResized( Gui::Window::Resized * cb )
		{
//			E_ADDREF( cb );
//			E_RELEASE( cbResize );
			cbResize = cb;
			return 1;
		}


		HWND init( HWND prnt, int id )
		{   return  hwnd=GetDlgItem(prnt,id); }

		int setText( const char *s )
		{   return  SetWindowText(hwnd,s); }
		int getText( char *s, int l=0xff )
		{   return  GetWindowText(hwnd,s,l); }

		int setPos( int x, int y ) 
		{   return SetWindowPos( hwnd, 0, x,y, 0,0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE ); }
		int setSize( int w, int h ) 
		{   return SetWindowPos( hwnd, 0, 0,0, w,h, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE ); }

		virtual uint setSize( uint x, uint y, uint w, uint h )
		{
			this->x=x;
			this->y=y;
			this->w=w;
			this->h=h;
			uint r = 0;
			if ( hwnd )
				r = SetWindowPos( hwnd, 0, x,y, w,h, SWP_NOZORDER|SWP_NOACTIVATE ); 
			if ( r && cbResize )
				r = cbResize->call( this, x,y,w,h );
			return r;
		}

        virtual uint show()
		{
			if ( ! hwnd )
			{
				hwnd = CreateWindowEx(
					WS_EX_STATICEDGE , "static", getName(), WS_VISIBLE | WS_CHILD,
					x,y,w,h,
					parent, (HMENU)(id), _getHinst(parent), 0 );
			}
			printf( __FUNCTION__ " :  %x\n", hwnd );
			ShowWindow(hwnd,SW_SHOWNORMAL);
			return (hwnd!=0);
		}

        virtual uint hide()
		{
			ShowWindow(hwnd,SW_HIDE);
			return 1;
		}

        virtual uint state()
		{
			return 0; // NOT_IMPL
		}

    };


    struct CStatic
        : CBaseWindow< Gui::Window, CStatic >
    {
	};

    struct CAppWindow
        : CBaseWindow< Gui::Window, CAppWindow >
    {
		Gui::Application * app;

		CAppWindow() : app(0){}
		~CAppWindow() { E_RELEASE(app); }

		void shutdown()
		{
			//if ( app ) app->onExit();
			printf( __FUNCTION__ "\n");
			PostQuitMessage( 0 );
		}

		static LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
		{
			if ( uMsg == WM_CREATE ) {
				CREATESTRUCT *cr = (CREATESTRUCT*)lParam;
				if ( cr )
					SetWindowLong (hWnd, GWL_USERDATA, (long)(cr->lpCreateParams) );
			}
		
			CAppWindow *cwin = (CAppWindow*)(GetWindowLong( hWnd, GWL_USERDATA ));
		
			if ( cwin ) 
				return cwin->message(hWnd,uMsg,wParam,lParam);
		
			return DefWindowProc( hWnd, uMsg, wParam, lParam );
		}

		LRESULT WINAPI message( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
		{
			switch( msg )
			{
				case WM_COMMAND:
				{
					switch( wParam )
					{
						case IDCANCEL:
			printf( __FUNCTION__ " : cancel\n");
							shutdown();
							return 0;
					}
					break;
				}
				case WM_CHAR:
				{
					if ( app ) app->onKey( wParam );					
					switch( wParam )
					{
						case 27:
							shutdown();
							return 0;							
					}
					break;
				}
				case WM_DESTROY:
				{
			printf( __FUNCTION__ " : destroy\n");
					shutdown();
					return 0;
				}
			}

			return DefWindowProc( hWnd, msg, wParam, lParam );
		}

		virtual uint show()
		{
			if ( ! hwnd )
			{
				WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, 
								hDll, NULL, NULL, NULL, NULL, "e6", NULL };
				RegisterClassEx( &wc );

				hwnd = CreateWindow( "e6", "e6", 
									  WS_OVERLAPPEDWINDOW, x,y,w,h,
									  0, NULL, wc.hInstance, NULL );
				assert( hwnd );
			}
			ShowWindow(hwnd,SW_SHOWNORMAL);
			UpdateWindow( hwnd );
			return 1;
		}

        virtual bool setFont(uint size, const char * fName) 
		{
			return 0; // NOT_IMPL
		}

        virtual bool run( Gui::Application * a, int w, int h, const char *title ) 
		{
			app = a;
			E_ADDREF( app );
			if ( ! app->onInit( hwnd ) )
				return 0;

			MSG msg={0}; 
			while( msg.message != WM_QUIT )
			{
				if( PeekMessage(  &msg, 0, 0, 0, PM_REMOVE ) )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
				else
				{
					if ( app ) app->onIdle();
				}
			}
			printf( __FUNCTION__ " finished\n");
			return 1;
		}

    };



	
} // namespace GuiW32



extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Gui.AppWindow", "GuiW32", GuiW32::CAppWindow::createSingleton	},
		{	"Gui.Static", "GuiW32", GuiW32::CStatic::createInterface	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 2; // classses
}



#include "../../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Guiw32 00.000.0000 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

BOOL APIENTRY DllMain( HANDLE hModule, DWORD dwReason, void * lpReserved )
{
	hDll = (HMODULE) hModule;

	switch ( dwReason )
	{
	case DLL_THREAD_ATTACH:
		// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
		//_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_ATTACH:
		//printf( __FUNCTION__ "  :  %x %x\n", hDll, dwReason );
		break;
	case DLL_PROCESS_DETACH:
		hDll = (HMODULE) 0;
		break;
	}
	
	return 1;
}
