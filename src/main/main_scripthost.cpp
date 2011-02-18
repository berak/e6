#include <stdio.h>
#include <string.h>
#include <conio.h>

#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"

#include "../Net/Net.h"
#include "../Application/Application.h"



void help()
{
	printf( "use : %s [options]\n", __argv[0] );
	printf( "\t options:\n" );
	printf( "\t\t -script [ScriptDll]\n" );
	printf( "\t\t -renderer [RendererDll]\n" );
	printf( "\t\t -port [Tcp port for ScriptServer]\n" );
	printf( "\t\t -function [scriptFunction on frame]\n" );
}

int main(int argc, char *argv[])
{
	bool    ok = 0;
	int   port = 9999;
	char * scr = "ScriptSquirrel";
	char * ren = "RendererDx9";
	char * rsc = 0;
	char * fun = 0;

	for ( int i=1; i<argc; i++ )
	{
		if ( ( ! strcmp( argv[i], "-h" ) ) || ( ! strcmp( argv[i], "/?" ) ) )
		{
			help();
			return 0;
		}
		if ( ! strcmp( argv[i], "-port" ) )
		{
			port = atoi( argv[++i] );
			continue;
		}
		if ( ! strcmp( argv[i], "-script" ) )
		{
			scr = argv[++i];
			continue;
		}
		if ( ! strcmp( argv[i], "-renderer" ) )
		{
			ren = argv[++i];
			continue;
		}
		if ( ! strcmp( argv[i], "-function" ) )
		{
			fun = argv[++i];
			continue;
		}
		rsc = argv[i];
	}


	e6::Engine * engine = e6::CreateEngine();	

	Application::Main * app  = (Application::Main *)engine->createInterface( "Application", "Application.Main" );	

	do  // once
	{
		if ( ! app )
		{
			std::cout <<  "Could not load host " << "Application::Main" << ".\n";
			break;
		}

		if ( ! app->init( engine ) )
		{
			std::cout <<  "Could not init Application " << scr << ".\n";
			break;
		}

		if ( ! app->loadRenderer( ren ) )
		{
			std::cout <<  "Could not load '" << ren << "'.\n";
			break;
		}

		if ( ! app->loadScriptEngine( scr ) )
		{
			std::cout <<  "Could not load interpreter " << scr << ".\n";
			break;
		}

		if ( fun )
		{
			if ( ! app->bindScriptEvent( 1, fun ) )
			{
				std::cout <<  "Could not bind ScriptEvent ( " << fun << " ).\n";
				break;
			}
		}

		if ( ! app->startScriptServer( port ) )
		{
			std::cout <<  "Could not start startScriptServer( " << port << " ) " << scr << ".\n";
			break;
		}

		if ( rsc )
		{
			if ( ! app->loadResource( rsc ) )
			{
				std::cout <<  "Could not load resource '" << rsc << "'.\n";
				break;
			}
		}
		//
		//! if the app was started successful, this will block for the app's lifetime
		//
		if ( ! app->run( 0 ) )
		{
			std::cout <<  "Could not run Application " << scr << ".\n";
			break;
		}
		ok = true;

	} while(0); // once	


	E_RELEASE(app);	
	E_RELEASE(engine);	
	return !ok;
}

