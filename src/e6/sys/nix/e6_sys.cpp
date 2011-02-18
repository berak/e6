#include "e6_sys.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdarg.h>

namespace e6
{
	namespace sys
	{
		bool keyPressed( uint k )
		{
			//~ return ( uint(GetKeyState( k )) == k );
			return 0; // NOT_IMPL
		}

		bool mousePressed( uint k )
		{
			//~ return keyPressed( 1 << k );
			return 0; // NOT_IMPL
		}
		uint mouseX()
		{
			//~ POINT p = {0,0};
			//~ GetCursorPos(&p);
			//~ return p.x;
			return 0; // NOT_IMPL
		}
		uint mouseY()
		{
			//~ POINT p = {0,0};
			//~ GetCursorPos(&p);
			//~ return p.y;
			return 0; // NOT_IMPL
		}

		char fileSeparator()
		{
			return '/';
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
		    static char buffer[512] = {0};
			return getcwd(buffer,511); //".";
		}

		bool execute(const char * code)
		{
			return WinExec( code, 2 );			
		}
		int alert(const char *caption, const char *format, ...)
		{
		    static char buffer[2048] = {0};
		    va_list args;
		    va_start( args, format );
		    vsprintf( buffer, format, args );
		    va_end( args );
		    fprintf( stderr, "%s : %s\n", caption, buffer);   
		    return 1;
		}

		uint getMicroSeconds()
		{
		    struct timeval  ts = {0,0};
		    struct timezone tv = {0,0};
		    gettimeofday( &ts, &tv );
	        return ts.tv_sec *1000000 + ts.tv_usec;
		}

		namespace dll
		{
			void * open( const char * moduleName )
			{
				static char md[255];
				md[0]=0;
				sprintf( md, "%s.so", moduleName ); 
				void *handle = dlopen( md, RTLD_LAZY );
				if ( !handle )
				{
					e6::sys::alert( "dll::open Error no handle!", "(%s)\n", dlerror() );
				}
				return handle;
			}
			void * getSymbol( void * handle, const char * name )
			{
				void *sym = dlsym( handle, name );
				if ( !sym )
				{
					e6::sys::alert( "dll::getSymbol Error no symbol!", "(%s)\n", dlerror() );
				}
				return sym;
			}
			void close( void * handle )
			{
				if ( handle )
					dlclose( handle );
			}
		}; // dll



		WIN32_FIND_DATA lpFindFileData;
		HANDLE hSearch;

		FindFile::FindFile( const char * filePattern ) 	
		{
			hSearch = FindFirstFile( filePattern, &lpFindFileData );
		}


		FindFile::~FindFile()
		{
			if ( isValid() )
			{
				FindClose( hSearch );
			}
		}


		bool FindFile::isValid()
		{
			return hSearch != INVALID_HANDLE_VALUE;
		}


		bool FindFile::isFolder()
		{
			if ( lpFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				return true;
			}

			return false;
		}


		const char * FindFile::getFileName()
		{
			if ( isValid() )
			{
				return lpFindFileData.cFileName;
			}

			return NULL;
		}


		bool FindFile::next()
		{
			if ( isValid() )
			{
				if ( ! FindNextFile( hSearch, &lpFindFileData ) )
				{
					hSearch = INVALID_HANDLE_VALUE;

					return false;
				}

				return true;
			}

			return false;
		}


		
	}; // sys
}; // e6


