#include "../../e6_sys.h"
#include <windows.h>
#include <stdio.h>

namespace e6
{
	namespace sys
	{

		bool keyPressed( uint k )
		{
			return ( uint(GetAsyncKeyState( k ))  );
		}

		bool mousePressed( uint k )
		{
			return keyPressed( 1 << k );
		}


		float relMouseX()
		{
			POINT p = {0,0};
			GetCursorPos(&p);
			HWND hwnd = GetActiveWindow();
			RECT r;
			GetWindowRect( hwnd, &r );
			uint w = r.right - r.left;
			float res = float( p.x - r.left ) / float(w);
			if ( res < 0 ) res = 0;
			if ( res > 1.0 ) res = 1.0;
			return res;
		}
		float relMouseY()
		{
			POINT p = {0,0};
			GetCursorPos(&p);
			HWND hwnd = GetActiveWindow();
			RECT r;
			GetWindowRect( hwnd, &r );
			uint h = r.bottom - r.top;
			float res = float( p.y - r.top ) / float(h);
			if ( res < 0 ) res = 0;
			if ( res > 1.0 ) res = 1.0;
			return res;
		}

		uint mouseX()
		{
			POINT p = {0,0};
			GetCursorPos(&p);
			return p.x;
		}
		uint mouseY()
		{
			POINT p = {0,0};
			GetCursorPos(&p);
			return p.y;
		}
		const char * loadFile( const char * path, bool binMode )
		{
			static char buf[0xfffff];
			// buf[0]=0;
			memset(buf,0,0xfffff );
			FILE * f = fopen( path, (binMode?"rb":"r") );
			if ( ! f ) 
			{
				printf( "File '%s' not found!\n", path );
				return 0;
			}
			int r = fread( buf, 1, 0xfffff, f );
			buf[r]=0;
			fclose(f);
			return buf;
		}

		char fileSeparator()
		{
			return '\\';
		}
		bool fileExists(const char * fname)
		{
			FILE *f=fopen(fname,"rb");
			if ( f )
			{
				fclose(f);
				return 1;
			}
			return 0;
		}
		const char * getCurrentDir()
		{
			static char dir[300];
			dir[0]=0;
			GetCurrentDirectory(300,dir);
			return dir;
		}
		bool setCurrentDir(const char * d)
		{
			return ( 0 != SetCurrentDirectory(d) );
		}

		bool execute(const char * code)
		{
			return ( 0 != system( code ) );			
		}
		int alert(const char *caption, const char *format, ...)
		{
		    static char buffer[2048] = {0};
		    va_list args;
		    va_start( args, format );
		    vsprintf( buffer, format, args );
		    va_end( args );
			printf( "%s : %s\n", caption, buffer );
		    int r = MessageBox(GetActiveWindow(), buffer, caption, 1);   
		    return (r & IDOK);
		}

		unsigned int getMicroSeconds()
		{
			static double timeScale = 0;
			if ( ! timeScale )
			{
				LARGE_INTEGER freq;
		 		QueryPerformanceFrequency( &freq );
				timeScale = 1000.0  / freq.QuadPart;
			}
			LARGE_INTEGER now;
			QueryPerformanceCounter( &now );
			return (unsigned int)( now.QuadPart * timeScale );
		}

		namespace dll
		{
			void * open( const char * moduleName )
			{
				static char md[255];
				md[0]=0;
				sprintf( md, "%s.dll", moduleName ); 
				return LoadLibrary( md );
			}
			void * getSymbol( void * handle, const char * symbolName )
			{
				return GetProcAddress( (HMODULE) handle, symbolName );			
			}
			void   close( void * handle )
			{
				if ( handle )
					FreeLibrary( (HMODULE)handle );			
			}
		}
	}
}


