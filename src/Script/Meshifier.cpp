
#include "Script.h"

#include "e6_impl.h"
#include "e6_container.h"
#include "../Core/Core.h"

#include <stdio.h>
#include <string.h>

#include <stdlib.h> // system call & varargs
#include <stdarg.h> // system call & varargs

namespace Meshifier	
{
	struct CInterpreter
		: public e6::Class< e6::CName<Script::Interpreter>, CInterpreter> 
	{
		char _konsole[8192];
		char _kinput[8192];
		bool _selAll;


		CInterpreter()
		{
			memset( _konsole, 0, 8192 );
			memset( _kinput,  0, 8192 );
			_selAll = 0;
			$1("CInterpreter()");
		}
		~CInterpreter()
		{
			$1("~CInterpreter()");
		}

		virtual bool exec( e6::Engine * engine, const char *buffer )// load & run
		{
			Core::World * world  = (Core::World *)engine->createInterface( "Core", "Core.World" );	
			print_konsole("<< Meshifier %s >>\n", __DATE__ );
			E_RELEASE( world );
			return false;
		}
	
		virtual const char * getOutput() const               	// virtual console
		{
			return _konsole;
		}
		virtual void clearOutput()          	// clear virtual console
		{
			_konsole[0]=0;
			return ;
		}
/***
		bool process( Core::Node *node, int what, float x, float y, float z )
		{
				switch( what ) {
					case 0:
						print_konsole( "scale %s %f %f %f\r\n", s->getName(), x,y,z );
						Meshify::scale(node, x,y,z);
						break;
					case 1:
						print_konsole( "rot %s %f %f %f\r\n", s->getName(), x,y,z );
						Meshify::rot(node, x,y,z);
						break;
					case 2:
						print_konsole( "move %s %f %f %f\r\n", s->getName(), x,y,z );
						Meshify::move(node, x,y,z);
						break;
					case 3:
						print_konsole( "yz %s \r\n", s->getName() );
						Meshify::yz(node);
						break;
					case 4:
						print_konsole( "norms %s\r\n", s->getName() );
						Meshify::normals(node);
						break;
					case 5:
						print_konsole( "share %s\r\n", s->getName() );
						Meshify::share(node,_konsole);
						break;
					case 6:
						print_konsole( "world %s\r\n", s->getName() );
						Meshify::world(node,m->getFrame()->getComposite());
						m->getFrame()->reset();
						break;
					default : return 0;
				}
			m->getBox()->clear();
			return 1;
		}
		bool Meshifier::run()
		{
			if ( _kinput[0] == 0 )	return 0;

			int what=0;
			float x=0,y=0,z=0;
			char str[300];
			str[0]=0;
			IScene *scn = engine->activeScene();
			if ( ! scn ) {
				print_konsole("no scene active !\r\n");
				return 0;
			}
			if ( sscanf(_kinput, "lmap %d", &what ) == 1 )
			{
				ILightMapper *lm = createLightMapper();
				lm->setOptions( what, (what!=0), _konsole );
				lm->lightScene( engine, scn );
				delete lm;
				return 1;
			}
			else
			if ( sscanf(_kinput, "sel %s", str ) ) {
				_selAll=0;
				if ( !strcmp(str,"all") )
					return (_selAll=1);
				IBase *base=0;
				const char *t = engine->deepSearch(str, &base);
				if (base){
					mdl = dynamic_cast<IModel*>(base);
					if ( mdl ) {
						print_konsole("selected %s %s!\r\n", t, str);
						return 1;
					}
					else print_konsole("err :%s %s is not Model !\r\n", t, str );
				}
				else print_konsole("err :%s %s not found !\r\n", t, str );
				return 0;
			}
			else
			if ( sscanf(_kinput, "load %s", str ) ) {
				return engine->loadScene(str);
			}
			else
			if ( sscanf(_kinput, "save %s", str ) ) {
				return engine->saveScene(str);
			}
			else
			if ( sscanf(_kinput, "scale %f %f %f", &x,&y,&z ) ) 		what = 0;
			else
			if ( sscanf(_kinput, "rot %f %f %f", &x,&y,&z ) )   		what = 1;
			else
			if ( sscanf(_kinput, "move %f %f %f", &x,&y,&z ) )          what = 2;
			else
			if ( !strncmp(_kinput, "swapyz",6  ) )                      what = 3;
			else
			if ( !strncmp(_kinput, "norms",5  ) )                       what = 4;
			else
			if ( !strncmp(_kinput, "share",5  ) )                       what = 5;
			else
			if ( !strncmp(_kinput, "world",5 ) )						what = 6;
			else {
				print_konsole("could not eval your input ('%s')\r\ntry one of those:\r\n",_kinput);
				print_konsole("load  sc-name -- load a scene\r\n");
				print_konsole("save  sc-name -- save a scene\r\n");
				print_konsole("sel   m-name  -- select model\r\n");
				print_konsole("world         -- transform to world-coords\r\n");
				print_konsole("share         -- try to share verz (active modl)\r\n");
				print_konsole("norms         -- generate shared face-norms(active modl)\r\n");
				print_konsole("swapyz        -- swap active Model's y-z verts\r\n");
				print_konsole("scale x y z   -- scale  active model\r\n");
				print_konsole("rot  x y z    -- rotate active model (world-space)\r\n");
				print_konsole("move x y z    -- move active Model (world-space)\r\n");
				return 0;
			}

			if ( selAll )
			{
				for ( IModel *m = scn->getModel(); m; m=m->next() )
					process( m, what, x,y,z );
				return 1;
			} 
			else 
				return process( mdl, what, x,y,z );
			if ( mdl )
			print_konsole("no model active !\r\n");
			return 0;
		}
***/

		void print_konsole( const char *fmt, ... )
		{
			if ( ! fmt || !fmt[0] ) return;

			static char buffer[2048] = {0};
			buffer[0] = 0;
		    va_list args;
		    va_start( args, fmt );
		    vsprintf( buffer, fmt, args );
		    va_end( args );

			int len = strlen(buffer);
			buffer[len] = 0;

			if ( (strlen(_konsole) + len) > 8190 )
				_konsole[0] = 0; // sorry, we've got to clear the buffer

			strcat( _konsole, buffer );
		}

	};
};



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",		"Meshifier",			Meshifier::CInterpreter::createInterface		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("MyScript 00.000.0001 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

