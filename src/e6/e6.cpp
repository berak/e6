#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <map>
#include <vector>

#include "e6_impl.h"
#include "e6_sys.h"
#include "version.h"


namespace e6
{

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// strings
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////

	const char *toString( uint value )
	{
		static char str[256];
		sprintf( str, "%u", value );
		return str;
	}

	const char *toString( const float * p, uint siz )
	{
		static char str[256];
		switch( siz )
		{
			case 1:
				sprintf( str, "%3.3f", p[0] );
				break;
			case 2:
				sprintf( str, "%3.3f %3.3f", p[0],p[1] );
				break;
			case 3:
				sprintf( str, "%3.3f %3.3f %3.3f", p[0],p[1],p[2] );
				break;
			case 4:
				sprintf( str, "%3.3f %3.3f %3.3f %3.3f", p[0],p[1],p[2],p[3] );
				break;
		}
		return str;
	}

	const char *toString( float value )
	{
		return  toString( &value, 1 );
	}

	const char *toString( const char * value )
	{
		return  value;
	}

	float stringToFloat( const char *s )
	{
		float v;
		sscanf( s, "%f", &v );
		return v;
	}
	int stringToInt( const char *s )
	{
		int v;
		sscanf( s, "%i", &v );
		return v;
	}



	//////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Dll
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////

	struct Dll
	{
		char moduleName[300];
		e6::ClassInfo * cls;
		e6::ModVersion mv;
		void *handle;

		Dll::Dll()
			: cls(0)
        	, handle(0)
	    {
			$X1("new()");
			mv.modVersion = "";
			mv.e6Version = "";
		}

		Dll::~Dll()
		{
			$X1("del()");
			sys::dll::close( handle );
		}

		bool Dll::open( const char *name, const char *symbolName="getClassInfo" )
		{
			do // once 
			{ 
				handle = sys::dll::open( name );
				if ( ! handle )
				{
					sys::alert( __FUNCTION__, "mod not loaded(%s)\n    on path(%s)", name, sys::getCurrentDir() );
					return 0;
				}
	
				GETCLASSINFO ci = (GETCLASSINFO)sys::dll::getSymbol( handle, symbolName );
				if ( ! ci )
				{
					sys::alert( __FUNCTION__, "symbolName not found (%s) :: (%s)", name, symbolName );
					break;
				}

				if ( ! ci( &cls ) )
				{
					sys::alert( __FUNCTION__, "class not created %s :: %s", name, symbolName );
					break;
				}

				return true;

			} while(0); // once;

			sys::dll::close( handle );
			handle = 0;
			return 0;
		}

		e6::ClassInfo * Dll::getClassInfo() const
		{
			//$();
			return cls;
		}
		const e6::ModVersion * Dll::getVersion() const
		{
			//$();
			return &mv;
		}

	}; // Dll



	////////////////////////////////////////////////////////////////////////////////////////////////////
	////
	//// Engine
	////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	struct CEngine 
		: Class< Engine, CEngine >
	{
		typedef std::map< const char *, Dll*, StrLess >			ModMap;
		typedef ModMap::iterator ModIter;
		ModMap mods;
		char e6Path[300];

		CEngine() 
		{
			//$X1("new()");

			bool ok = findE6Path();
			if ( ! ok ) 
			{
				e6::sys::alert( __FUNCTION__, " WARNING : Could not resolve e6 base path !\r\n" );
			}
			else
			{
				printf( __FUNCTION__, " Started on base path '%s' !\r\n" , e6Path );
			}
		}

		bool findE6Path()
		{
			char up[300];
			char name[300];
			char path[300];
			// this must be in the 'bin' folder.
			const char * e6_dll = "_dllCache";

			const char * here = sys::getCurrentDir();
			strcpy( e6Path, here );
			//printf( "here is %s , - ", here );

			// we're in e6?
			if ( _find(path, e6_dll, here, "bin" ) )
			{
				//printf( "we're in e6.\n");
				return true;
			}
			// we're in e6/bin?
			if ( _find(path, e6_dll, here ) )
			{
				this->chopPath( here, up, name );
				strcpy( e6Path, up );
				//printf( "we're in e6/bin.\n");
				return true;
			}
			// we're in e6/some_folder?
			if ( _find(path, e6_dll, here, "..", "bin" ) )
			{
				this->chopPath( here, up, name );
				strcpy( e6Path, up );
				//printf( "we're in e6/*somewhere.\n");
				return true;
			}

			// we're in e6/bin/some_folder?
			sprintf( path, "%s%c..", here, sys::fileSeparator() );
			if ( _find(path, e6_dll, path  ) )
			{
				this->chopPath( here, up, name );
				this->chopPath( up, up, name );
				strcpy( e6Path, up );
				//printf( "we're in e6/bin/somewhere.\n");
				return true;
			}

			printf( "we're in nowhere.\n");
			return false;
		}

		bool _find( char *resultPath, const char *f, const char * p1, const char *p2=0, const char *p3=0 )
		{
			char tmp[300];
			char path[300];
			tmp[0]=0;
			path[0]=0;
			char sep = sys::fileSeparator();
			if ( p2 && p3 )
				sprintf(path,"%s%c%s%c%s", p1, sep, p2, sep,p3 );
			else 
			if ( p2 )
				sprintf(path,"%s%c%s", p1, sep, p2 );
			else 
				sprintf(path,"%s", p1 );

			sprintf(tmp,"%s%c%s",path,sep,f);
			bool e = sys::fileExists(tmp);
			if ( e )
				strcpy( resultPath, path );
			//fprintf( stderr, __FUNCTION__ "(%i) %s\n", e, path );
			return e;
		}

		virtual ~CEngine() 
		{
			$X1("del()");

			for ( ModIter it = mods.begin(); it != mods.end(); ++it )
			{
				const char * s = it->first;
				Dll * d  = it->second;
				ClassInfo *info = d->getClassInfo();
				char b[300];
				char b1[1024];
				bool alive=0;
				b[0] = b1[0] = 0;
				while ( info->iname )
				{
					if ( info->count() )
					{
						alive=1;
						sprintf( b, "%s %i instances of %s alive !\n", info->cname, info->count(), info->iname );
						strcat( b1, b );
					}
					++info;
				}
				if ( alive )
					sys::alert("ref trouble !", b1 );
				E_DELETE( d );
			}
		}


		Dll * loadDll( const char *moduleName )
		{
			E_ASSERT( moduleName );

			char binPath[300];
			sprintf( binPath, "%s%cbin", e6Path, sys::fileSeparator() ); 
			const char * oldPath = sys::getCurrentDir();
			sys::setCurrentDir( binPath );

			$X1("load (" << moduleName << ")" );
			Dll * dll = new Dll;
			if ( ! dll->open( moduleName ) )
			{
				delete dll;
				sys::setCurrentDir( oldPath );
				return 0;
			}
	
			strcpy( dll->moduleName, moduleName );
			mods.insert( std::make_pair( dll->moduleName, dll ) );

			// reset
			sys::setCurrentDir( oldPath );

			return dll;
		}

		uint unloadDll( const char *moduleName )
		{
			$X1("unload (" << moduleName << ")" );
			ModIter it = mods.find( moduleName );
			if ( it != mods.end() )
			{
				Dll* h = it->second;
				E_DELETE( h );
				return 1;
			}
			return 0;
		}

	
		void showRegistry()
		{
			std::cerr << "Registry (" << mods.size() << " modules) : "<< e6_version <<"\n";
			for ( ModIter it = mods.begin(); it != mods.end(); ++it )
			{
				const char * s = it->first;
				Dll * d  = it->second;
				showDll( d, s );
			}
		}
		void showDll(Dll * d, const char * s)
		{
			const ModVersion *mv = d->getVersion();
			std::cerr << "< " << mv->modVersion << " : " << mv->e6Version << " >\n";

			ClassInfo *info = d->getClassInfo();
			while ( info && info->iname )
			{
				std::cerr << "\t" << info->cname << (strlen(info->cname)<8?"\t\t":"") ;
				std::cerr << "\t" << info->iname << (strlen(info->iname)<8?"\t\t":"") ;
				std::cerr << "\t" << info->count() << "\n";
				++ info;
			}
		}


		virtual uint findInterfaces( const char * interfaceName, InterfaceCallback & finder )
		{
			uint n = 0;
			for ( ModIter it = mods.begin(); it != mods.end(); ++it )
			{
				const char * s = it->first;
				Dll * d  = it->second;
				ClassInfo *info = d->getClassInfo();
				while ( info && info->iname )
				{
					if ( ! strcmp( interfaceName, "*" ) ) // selected 'All'
					{
						uint r = finder.call( info->iname, info->cname, info->count() );
						n ++;
					}
					else
					if ( ! strcmp( interfaceName, info->iname ) )
					{
						uint r = finder.call( info->iname, info->cname, info->count() );
						n ++;
					}
					++ info;
				}
			}
			return n;
		}

		virtual uint loadAllModules() 
		{
			uint n=0;

			char binPath[300];
			sprintf( binPath, "%s%cbin", e6Path, sys::fileSeparator() ); 
			const char * oldPath = sys::getCurrentDir();
			sys::setCurrentDir( binPath );
			FILE * f = fopen("_dllCache","rb");
			if ( ! f ) return 0;
			char buf[512];
			while ( fgets( buf,512,f ) )
			{
				int z=(int)strlen(buf)-2;
				buf[z]=0;
				n += (loadDll( buf )!=0);
			}
			sys::setCurrentDir( oldPath );
			return n;
		}


		virtual uint cleanupModules() 
		{
			std::vector<ModIter> undead;
			for ( ModIter it = mods.begin(); it != mods.end(); ++it )
			{
				uint n = 0;
				const char * s = it->first;
				Dll * d  = it->second;
				ClassInfo *info = d->getClassInfo();
				while ( info && info->iname )
				{
					n += info->count();				
					++info;
				}
				if ( n==0 )
				{
					printf( "undead : %s\n", s );
					undead.push_back( it );
				}
			}
			for ( uint i=0; i<undead.size(); i++ )
			{
				ModIter it = undead[i];
				printf( "kill : %s\n", it->first );
				Dll * d  = it->second;
				E_DELETE( d );
				mods.erase( it );
			}
			return 1;
		}




				
		virtual void * createInterface( const char * moduleName,  const char * interfaceName )
		{
			E_ASSERT(moduleName);
			E_ASSERT(interfaceName);
			// std::cerr << __FUNCTION__ << " (" << moduleName << ", " << interfaceName << ")\n";
			ModIter it = mods.find( moduleName );
			bool found = 0;
			do // once
			{
				if ( it != mods.end() )
				{
					found = true;
					break;
				}

				if ( ! loadDll ( moduleName ) )
				{
				//	sys::alert( __FUNCTION__,  "mod not found %s :: %s", moduleName, interfaceName );
					break;
				}
				it = mods.find( moduleName );
				if ( it == mods.end() )
				{
					sys::alert( __FUNCTION__,  "mod not inserted %s :: %s", moduleName, interfaceName );
					break;
				}

				found = true;
			} while(0); // do once

			if ( ! found ) 
			{
				return 0;
			}

			Dll* h = it->second;
			ClassInfo * info = h->getClassInfo( );
			if ( ! info )
			{
				sys::alert( __FUNCTION__, "ClassInfo not found %s :: %s", moduleName, interfaceName );
				return 0;
			}
	
			while ( info && info->cname )
			{
				if ( ! strcmp( info->iname, interfaceName ) )
				{
					return info->create();
				}
				++ info;
			}
	
			sys::alert( __FUNCTION__, "could not create %s :: %s", moduleName, interfaceName );
			return 0;
		}

		virtual	uint error( uint eCode, const char *format, ... )
		{
			static char buf[4044];
			va_list args;
			va_start( args, format );
			vsprintf( buf, format, args );
			va_end( args );
			sys::alert( "err", buf );
			std::cerr << buf << "\n";
			return eCode;
		}
		virtual const char * getVersion() const
		{
			return e6_version;
		}
		virtual const char * getPath() const
		{
			return e6Path;
		}

		virtual uint chopExt(const char * orig, char *path, char *ext ) const
		{
			path[0] = ext[0] = 0;

			size_t n = strlen(orig);

			while ( n-- )
			{
				if ( orig[ n ] == '.' )
				{
					strcpy( ext, orig + (n+1) );
					strncpy( path, orig, (n) );
					path[n] = 0;
					break;
				}					
			}
			//printf( "%s : %s : %s : %s\n", __FUNCTION__, orig, path, ext );
			return n;
		}
		virtual uint chopPath(const char * orig, char *path, char *name ) const
		{
			path[0] = name[0] = 0;

			int n = (int)strlen(orig);

			while ( n--  )
			{
				if ( (orig[ n ] == '/') ||  (orig[ n ] == '\\') )
				{
					strcpy( name, orig + (n+1) );
					strncpy( path, orig, (n) );
					path[n] = 0;
					break;
				}		
			}
			if ( n<0 )
			{
				// no path found
				strcpy( name, orig );
			}
			//printf( "%s : %s : %s : %s\n", __FUNCTION__, orig, path, name );
			return 1;
		}


	};

//	extern "C"
//	{
		Engine * CreateEngine()
		{
			return (Engine*)CEngine::createSingleton();

			//typedef void*(*Creator)(void*);

			//Engine * engine = 0;
			//const char * oldPath = sys::getCurrentDir();
			//char binPath[300];
			//sprintf( binPath, "%s%cbin", e6Path, sys::fileSeparator() ); 
			//sys::setCurrentDir( binPath );
		
			//Dll * dll = new Dll;
			//do // once
			//{
			//	dll->handle = sys::dll::open( "Engine" );
			//	if ( ! dll->handle )
			//	{
			//		sys::alert( __FUNCTION__, "mod not loaded(%s)\n    on path(%s)", "Engine", sys::getCurrentDir() );
			//		break;
			//	}

			//	Creator create = (Creator)sys::dll::getSymbol( dll->handle, "CreateTheEngine" );
			//	if ( ! create )
			//	{
			//		sys::alert( __FUNCTION__, "symbolName not found (%s) :: (%s)", "Engine", "CreateEngine" );
			//		break;
			//	}


			//	engine = (Engine*)create(dll);
			//	if ( ! engine )
			//	{
			//		sys::alert( __FUNCTION__, "\r\n    *** Engine not created !! ***\r\n" );
			//		break;
			//	}

			//	//engine->dll = dll;

			//} while ( false ); // once
	
			//sys::setCurrentDir( oldPath );
			//if ( ! engine )
			//{
			//	E_DELETE( dll );
			//}
			//return engine; // break out
		}	
//	};
}

