
// 
// script shell,
// loads a dll containing the interpreter
// make:  make -f makefile.eel
// run :  eel [script_dll] [script_file]
//
//
// 9/22/2005 - lua static shell
// 3/5/2006  - dynload interpreter from dll
// 2/8/2006  - try to load file first, then switch to interactive
//
//



#include <stdio.h>
#include <string.h>

#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../Script/Script.h"

//~ #include "Windows.h"


struct DefLogger : e6::Logger 
{
	virtual bool printLog( const char * s )  
	{
		printf( s );
		return 1;
	}
};
struct FileLogger : e6::Logger 
{
	FILE * f;
	FileLogger( char * name )
	{
		f = fopen(name, "wb" );
	}
	~FileLogger()
	{
		fclose(f);
	}
	virtual bool printLog( const char * s )  
	{
		fprintf( f, s );
		fflush( f );
		return 1;
	}
}; 

// eel [-s dll] [-e errlog] [-o outlog] [script] [script] [...]

int main(int argc, char *argv[])
{
	// options
	DefLogger _defLog;
	e6::Logger *le =&_defLog, *lo=&_defLog;
	char *fe = "ScriptSquirrel";
	char *scr[8];
	int nscr = 0;
	bool w;
	bool interactive = 0;
	for ( int i=1; i<argc; i++ )
	{
		if ( argv[i][0] == '-' )
		{
			if ( argv[i][1] == 'w' )
			{
				w = 1;	
			}
			if ( argv[i][1] == 'i' )
			{
				interactive = 1;	
			}
			if ( argv[i][1] == 's' )
			{
				fe = argv[++i];		
			}
			//if ( argv[i][1] == 'e' )
			//{
			//	le = FileLogger( argv[++i] );		
			//}
			//if ( argv[i][1] == 'o' )
			//{
			//	lo = FileLogger( argv[++i] );		
			//}		
			continue;
		}
		scr[nscr++] = argv[i];
	}	

	// setup
	e6::Engine * engine = e6::CreateEngine();	

	Script::Interpreter * speek  = (Script::Interpreter *)engine->createInterface( fe, "Script.Interpreter" );	
	if ( ! speek )
	{
		std::cout <<  "Could not load interpreter " << fe << ".\n";
		E_RELEASE(engine);	
		return 1;
	}
	if ( ! speek->setup( engine ) )
	{
		std::cout <<  "Could not start interpreter " << fe << ".\n";
		E_RELEASE(speek);	
		E_RELEASE(engine);	
		return 1;
	}
	//speek->setErrlog( *le );
	//speek->setOutlog( *lo );

	// load script(s)
	for ( int i=0; i<nscr; i++ )
	{
		const char * s = e6::sys::loadFile(scr[i], false) ;
		if ( ! s )
		{
			std::cout <<  "Could not load script '" << scr[i] << "'.\n";
			continue;
		}
		bool ok = speek->exec(s, scr[i]);
		if ( ! ok )
		{
			std::cout <<  "script '" << scr[i] << "' failed on execution.\n";
			continue;
		}
		std::cout <<  "script '" << scr[i] << "' executed.\n";
	}	
	
	if( interactive || ! nscr	)
	{
		char buf[4300];
		for (;;) 
		{ 
			memset(buf,0,4300);
			printf("\n%s> ", speek->getFileExtension() );
			if ( ! fgets( buf, 4300, stdin ) )		break;
			if ( ! strncmp( buf, "ok.",3 ) )		break;

			if ( ! speek->exec( buf, "konsole" ) )	break;
		}
	}

	E_RELEASE(speek);	
	E_RELEASE(engine);	
	return 0;
}


