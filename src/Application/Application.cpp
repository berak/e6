
#include "Application.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../e6/e6_sys.h"
#include "../e6/sys/w32/e6_thread.h"
#include "../Core/Core.h"
#include "../Script/Script.h"
#include "../TimeLine/TimeLine.h"
#include "../Net/Net.h"

#include "../main/w32/WndClass.h"

#include <map>


using e6::uint;
using e6::ClassInfo;


namespace Application
{

	struct CMain
		: public e6::CName< Application::Main, CMain >
		, e6::Logger
		, Net::Connection
		, w32::CWindow
		, Application::EventHandler
	{

	protected:
		e6::Engine * engine;
		Core::World * world;
		Core::Camera * cam;
		Core::Renderer * renderer;
		Script::Interpreter * script;
		Net::TcpServer * server;
		Net::TcpClient * client;
		TimeLine::TimeLine * timeline;
		float timeNow;
		float timeStep;
		uint frameNo;
		bool running;
		bool scriptLoaded;

		HWND win;

		bool keys[0xff];
		char buf[8012]; // script output, etc
		char scriptCode[1024]; // accumulate events and fire them in idle
		struct EventEntry
		{
			uint ev;
			const char * code;
			EventHandler * handler;			
			EventEntry(uint e=0,const char *c=0,EventHandler * h=0) 
				: ev(e), code(c), handler(h) 
			{}
		};
		typedef std::map< uint, EventEntry > EventMap;
		EventMap events;

		bool handleEvent( EventEntry & entry )
		{
			int nparams = 0;
			float p0=0, p1=0, p2=0;

			switch( entry.ev )
			{
				case ET_NONE:
				{
					return 0;
				}
				case ET_FRAME:
				{
					nparams = 1;
					p0 = this->timeNow;
					break;
				}
				case ET_MOUSE_L:
				{
					int l = e6::sys::keyPressed( VK_LBUTTON );
					if ( !l )
						return 0;

					nparams = 2;
					p0 = e6::sys::relMouseX();
					p1 = e6::sys::relMouseY();
					break;
				}
				case ET_MOUSE_R:
//				case ET_MOUSE_M:
				{
					int r = e6::sys::keyPressed( VK_RBUTTON );
					if ( !r  )
						return 0;

					nparams = 2;
					p0 = e6::sys::relMouseX();
					p1 = e6::sys::relMouseY();
					break;
				}
				default: // key
				{
					if ( entry.ev >= ET_KEY )
					{
						uint k = entry.ev - ET_KEY;
						uint p = 0;
						if ( (k >= '1') && (k <= 'Z') )
						{
							p = keys[k];
						}
						else
						{
							p = e6::sys::keyPressed( k );
						}
						if ( ! p )
							return 0;

						nparams = 1;
						p0 = k;
					}
				}
			}

			if ( entry.code )
			{
				return this->scriptEventHandler( entry.ev, p0, p1, p2, nparams );
			}
			return entry.handler->handleEvent( entry.ev, p0, p1, p2 );
		}

		bool scriptEventHandler( int ev, float p0, float p1, float p2, uint nparams )
		{
			static char * format[] = { "%s();", "%s(%2.3f);", "%s(%2.3f,%2.3f);", "%s(%2.3f,%2.3f,%2.3f);" };
			static char code[120];
			if( nparams < 4 )
			{
				code[0]=0;
				sprintf( code, format[nparams], this->events[ ev ].code );
				return this->script->exec( code, "e6" ); // MULTITHREADING!!!
			}
			return 0;
		}

		//! CWindow  dispatcher:
		virtual LRESULT CALLBACK message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
		{
			switch ( uMsg  ) {
			case WM_CREATE:
				on_create(hWnd);
				return 1;
			case WM_SIZE :
				on_resize( hWnd );
				return 1;
			case WM_DROPFILES:
				onDropFile(hWnd, wParam);
				return 1;
			case WM_LBUTTONUP :
				return 1;
			case WM_RBUTTONDOWN :
			case WM_LBUTTONDOWN :
				return 1;
			case WM_MOUSEMOVE :
				return 1;
			case WM_KEYDOWN:
				keys[ wParam ] = 1;
				return 1;
			case WM_KEYUP:
				keys[ wParam ] = 0;
				return 1;
			case WM_COMMAND:
				on_command( hWnd, uMsg, wParam, lParam );
				return 1;
			case WM_NOTIFY:
    			return on_notify( hWnd, uMsg, wParam, lParam) ;
			case WM_DESTROY:
				on_destroy();
				return 0;
			}
			return DefWindowProc( hWnd, uMsg, wParam, lParam );
		}


		bool on_key(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
		{
			return 0;
		}
		bool on_command(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
		{
			return 0;
		}
		bool on_notify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
		{
			return 0;
		}
		bool on_create( HWND hWnd )
		{
			this->win = hWnd;
			DragAcceptFiles(hWnd, TRUE); 

			HINSTANCE hinst = GetModuleHandle(0);
			char ico[200];
			sprintf( ico, "%s\\bin\\ui\\e6.ico", engine->getPath() );
			uint err = SetClassLong( hWnd, GCL_HICON, (LONG)LoadIcon( hinst,ico )  ); 
			return 1;
		}
		bool on_resize(HWND hWnd )
		{
			return 0;
		}
		bool onDropFile(HWND hWnd, WPARAM wParam )
	    {
			char  f[300];
			HDROP drop = (HDROP)wParam;      
			DragQueryFile( drop, 0, f, sizeof(f) ); 
			bool ok = loadResource( f );
			DragFinish( drop );
			return ok;
		}
		void on_destroy() 
		{
			running = 0;
			Sleep(200);
			//doRender = 0;
			//saveIni( "e6.ini" );		
			//if ( script ) script->exec( "cleanup();", this->getName() );
			PostQuitMessage(0);
		}


		uint on_idle()
		{
			this->timeNow += this->timeStep;
			this->frameNo ++;

			if ( ! running ) return 0; 

			if ( this->timeline )
			{
				this->timeline->update( this->timeNow );
			}

			EventMap::iterator it = this->events.begin();
			for ( it; it != this->events.end(); it++ )
			{
				//if ( it->second )
				{
					this->handleEvent( it->second );
				}
			}
			//if ( this->scriptCode[0] )
			//{
			//	this->script->exec( this->scriptCode, "idle" );
			//	this->scriptCode[0] = 0;
			//}

			if ( this->renderer )
			{
				this->renderScene();
			}

			Sleep( 30 );

			return 1;
		}

		void mainloop()
		{
			MSG msg={0}; 
			while( msg.message != WM_QUIT )
			{
				if( PeekMessage(  &msg, 0, 0, 0, PM_REMOVE ) )
				{
				//#ifdef DOPROF
				//	static Profile p("Main::message");
				//	p.start();
				//#endif
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				//#ifdef DOPROF
				//	p.stop();
				//#endif
				}
				else
				{
				//#ifdef DOPROF
				//	static Profile p("Main::idle");
				//	p.start();
				//#endif
					on_idle();
				//#ifdef DOPROF
				//	p.stop();
				//#endif
				}
			}
			//~ printf( __FUNCTION__ " finished\n");
		}
	


		void moveCam( float x, float y, float z )
		{
			if (this->cam)
			{
    			// move in camera space (so we can fly 'forward'):
				e6::float4x4 ct;
				this->cam->getWorldMatrix(ct);
				//ct.m01 = 0;
				//ct.m10 = 0;
				//ct.m21 = 0;
				//ct.m12 = 0;
				this->cam->addPos( ct.mul3x3( float3(x,y,z) ) );
		   }
		}

		void rotCam( float x, float y, float z )
		{
			if (this->cam)
			{
				// rotate in world space:
				//e6::float4x4 ct;
				//this->cam->getWorldMatrix(ct);
				this->cam->addRot( ( e6::float3(x,y,z) ) );
			}
		}

	public:

		CMain() 
			: engine(0) 
			, world(0) 
			, script(0)
			, renderer(0)
			, server(0)
			, client(0)
			, timeline(0)
			, cam(0)
			, timeNow(0)
			, timeStep(0.04f)
			, frameNo(0)
			, win(0)
			, running(0)
			, scriptLoaded(0)
		{
			memset( keys, 0, 0xff*sizeof(bool) );
		}

		~CMain() 
		{
			E_RELEASE( cam );
			E_RELEASE( server );
			E_RELEASE( client );
			E_RELEASE( renderer );
			E_RELEASE( timeline );
			E_RELEASE( script );
			E_RELEASE( world );
			E_RELEASE( engine );
		}

	
		virtual bool handleEvent( int ev, float p0, float p1, float p2 ) 
		{
			if ( ! running ) return 0; 
			float dx = 0.1f;
			float dr = 0.005f;
			//printf( "event( %2x, %2.2f, %2.2f, %2.2f );\n", ev, p0, p1, p2 );
			if ( ev == ET_KEY + VK_ESCAPE )		{	CMain::on_destroy();			}
			if ( ev == ET_KEY + VK_F1 )			{	this->newWorld();				}
			if ( ev == ET_KEY + 'W' )			{	this->moveCam( 0, 0, -dx );		}
			if ( ev == ET_KEY + 'S' )			{	this->moveCam( 0, 0, dx );		}
			if ( ev == ET_KEY + 'A' )			{	this->moveCam( -dx, 0, 0 );		}
			if ( ev == ET_KEY + 'D' )			{	this->moveCam( dx, 0, 0 );		}
			if ( ev == ET_KEY + 'R' )			{	this->moveCam( 0, dx, 0 );		}
			if ( ev == ET_KEY + 'F' )			{	this->moveCam( 0, -dx, 0 );		}
			if ( ev == ET_KEY + 'Q' )			{	this->rotCam( 0, dr, 0 );		}
			if ( ev == ET_KEY + 'E' )			{	this->rotCam( 0, -dr, 0 );		}
			if ( ev == ET_KEY + 'Y' )			{	this->rotCam( dr, 0, 0 );		}
			if ( ev == ET_KEY + 'C' )			{	this->rotCam( -dr, 0, 0 );		}
			if ( ev == ET_KEY + 'I' )			{	
				this->cam->setPos( float3(0, 0, 10) );
				this->cam->setRot( float3(0, 0, 0) );		
			}
			//if ( ev == ET_MOUSE_L )				{	this->moveCam( 0, 0, -dx );	 }
			//if ( ev == ET_MOUSE_R )				{	this->moveCam( 0, 0, dx );	 }
			//if ( ev == ET_MOUSE_R || ev == ET_MOUSE_L )	{
			if ( ev == ET_MOUSE_R )	{
				float yy = 0.02f * (p0 - 0.5);
				float xx = 0.02f * (p1 - 0.5);
				this->rotCam( -xx, -yy, 0 );		
			}

			return 1;
		}
			
		virtual bool init( e6::Engine * engine ) 
		{
			this->buf[0] = 0;

			this->engine = engine;
			E_ADDREF( this->engine );

			if ( ! this->timeline )
			{
				this->timeline = (TimeLine::TimeLine*) engine->createInterface( "TimeLine", "TimeLine.TimeLine" );
			}

			if ( ! this->world )
			{
				this->world = (Core::World*) engine->createInterface( "Core", "Core.World" );
			}
			this->newWorld();

			this->bindCppEvent( ET_KEY + VK_ESCAPE, this ); //VK_ESCAPE
			this->bindCppEvent( ET_KEY + VK_F1, this ); //w
			this->bindCppEvent( ET_KEY + 'W', this ); //w
			this->bindCppEvent( ET_KEY + 'A', this ); //w
			this->bindCppEvent( ET_KEY + 'S', this ); //w
			this->bindCppEvent( ET_KEY + 'D', this ); //w
			this->bindCppEvent( ET_KEY + 'Q', this ); //w
			this->bindCppEvent( ET_KEY + 'E', this ); //w
			this->bindCppEvent( ET_KEY + 'Y', this ); //w
			this->bindCppEvent( ET_KEY + 'C', this ); //w
			this->bindCppEvent( ET_KEY + 'R', this ); //w
			this->bindCppEvent( ET_KEY + 'F', this ); //w
			this->bindCppEvent( ET_KEY + 'I', this ); //w
			//this->bindCppEvent( ET_MOUSE_L, this ); //w
			this->bindCppEvent( ET_MOUSE_R, this ); //w

			return (1); 
		}

		virtual bool bindCppEvent( int ev, EventHandler * evh ) 
		{
			if ( ! evh )
			{
			}
			EventEntry e( ev, 0, evh );
			events.insert( std::make_pair( (EventType)ev, e ) );
			return 1;
		}

		virtual bool bindScriptEvent( int ev, const char * script ) 
		{
			EventEntry e( ev, script, 0 );
			events.insert( std::make_pair( (EventType)ev, e ) );
			return 1;
		}

		virtual bool run( void * window ) 
		{
			this->win = w32::CWindow::create( "e6","e6", 60,60, 600,400, WS_OVERLAPPEDWINDOW, 0, (HWND)window, GetModuleHandle(0) );

			if ( this->renderer )
			{
				running = this->renderer->init( this->win, 400,400 );
			}

			if ( ! running )
			{
				return false;
			}

			this->mainloop();
			return 1; 
		}

		virtual bool shutdown() 
		{
			running = 0; 
			return 0; //NOT_IMPL
		}

		virtual bool newWorld() 
		{
			if ( this->script )
			{
				this->script->exec( "cleanup();\r\n", this->getName() );
			}
			this->world->clear();
			this->setDefCam();
			return 1;
		}

		virtual e6::Engine * getEngine() const
		{
			if ( engine )
			{
				E_ADDREF( engine );
				return engine;
			}
			return 0; 
		}

		virtual bool loadResource( const char* s )
		{
			if ( ! s || !s[0] )
				return false;

			if ( script )
			{
				char path[200], ext[10];
				engine->chopExt( s, path, ext );
				if ( ! strcmp( ext, script->getFileExtension() ) )
				{
					char sep = e6::sys::fileSeparator();
					char name[200], absPath[200];
					engine->chopPath( s, path, name );
					sprintf( absPath, "%s%c%res%c%s%c%s", engine->getPath(),sep, sep, ext, sep, name );

					const char * code = e6::sys::loadFile(absPath, false);
					if ( code )
					{
						printf( "loaded script '%s'.\r\n", absPath );
						return script->exec( code, this->getName() );
					}
					else
					{
						printf( "\r\n ERROR : script '%s' not loaded !\r\n", s );
					}
					return false; // not loaded.
				} 
			}
		
			return world->load( engine, s );
		}


		virtual bool loadRenderer( const char* path )
		{
			if ( ! path || !path[0] )
				return 0;

			//char name[200], path[200], ext[10];
			//engine->chopPath( s, path, name );
			//engine->chopExt( name, path, ext );
			Core::Renderer * r2d2  = (Core::Renderer *)engine->createInterface( path, "Core.Renderer" );	
			if ( r2d2 )
			{		
				E_RELEASE( this->renderer );
				this->renderer = r2d2;

			//	renderScene();

				return 1;
			}
			return 0;
		}

		
		virtual bool printLog( const char * str ) 
		{
			//strcat( this->buf, str );
			printf( "$> %s\n",str);
			strcat( this->buf, str );
			return 1;
		}


		virtual uint handle( Net::Socket & socket )  
		{
			if ( ! running ) return 0; 

			if ( ! this->script ) 
			{
				return 0;
			}

			char buffer[8012];
			uint r = 0;

			this->buf[0] = 0; // clear output
			buffer[0] = 0;
			r = socket.read( buffer, 8012 );
			if ( r>0 )
			{
				buffer[r] = 0;

				r = this->script->exec( buffer, this->getName() );
				//strcat( this->scriptCode, buffer );
				//printf( "<:%s:>\r\n", buffer );		
			}
			else // client disconnected
			{
				printf( "Client disconnected.\n" );
				return 0;
			}

			Sleep(3);
			uint nBytes = strlen( this->buf );
			if ( nBytes )
			{
				socket.write( this->buf, nBytes );
				this->buf[0] = 0;
			}
			else
			{
				// send newline so the protocol won't hang:
				socket.write( ".\r\n\0", 3 );
			}
			return r;
		}


		virtual Core::Camera * getCamera() const 
		{
			E_ADDREF( this->cam );
			return cam;
		}
		
		
		virtual Core::World * getWorld() const 
		{
			E_ADDREF( this->world );
			return world;
		}

		
		virtual bool startScriptServer( int port )
		{
			E_RELEASE( this->server );

			this->server  = (Net::TcpServer *)engine->createInterface( "Net", "Net.TcpServer" );	

			return this->server->start( port, *this );
		}


		virtual bool stopScriptServer()
		{
			E_RELEASE( this->server );
			return 1;
		}

		
		virtual bool loadScriptEngine( const char * s )
		{
			if ( ! s || !s[0] )
				return 0;

			char name[200], path[200], ext[10];
			char * message = "Loaded";
			bool ok = true;

			engine->chopPath( s, path, name );
			engine->chopExt( name, path, ext );

			E_RELEASE( this->script );

			this->script  = (Script::Interpreter *)engine->createInterface( name, "Script.Interpreter" );	
			if ( ! this->script )
			{
				message = "Could not load interpreter ";
				ok = false;
			}

			if ( ok )
			{
				ok = this->script->setup( this->engine );
				if ( ok  )
				{
					this->script->setErrlog( *this );
					this->script->setOutlog( *this );
				}
				else
				{
					message= "Could not start interpreter ";
					E_RELEASE(this->script);	
				}
			}

			//setStatus( message, 0 );
			//setStatus( name, 1 );
			return ok;
		}


		virtual bool setCamera( const char* name ) 
		{
			Core::Camera * camera = (Core::Camera *)this->world->findRecursive( name );
			if ( camera )
			{
				E_ADDREF( camera );
				E_RELEASE( cam );
				cam = camera;
				return 1;
			}
			return 0;
		}


		void setDefCam()
		{
			E_RELEASE( cam ) ;

			cam = (Core::Camera *)engine->createInterface( "Core", "Core.Camera" );
			cam->setName( "DefCam" );

			Core::Node * root = world->getRoot();
			cam->setPos( e6::float3(0,0,40) );
			root->link( cam );
			root->synchronize();
			E_RELEASE( root );

			//if ( ! mover )
			//	mover = (Physics::Mover *)engine->createInterface( "Physics", "Physics.Mover" );
			//moving = cam;
			//E_ADDREF( moving );	
		}


		void renderToTex( Core::RenderToTexture * rt )
		{
			//static Profile _p("Main::renderToTexture");
			//_p.start();

			Core::Texture* tex = rt->getRenderTarget();
			if ( tex )
			{
				renderer->end3D();
				renderer->setRenderTarget( tex );
				renderer->begin3D();
			}
			E_RELEASE( tex );
			
			uint nl = 0;
			renderFirstCam( rt );
			renderLight( rt, nl );

			for ( uint i=0; i<rt->numChildren(); i++ )
			{
				Core::Node * n = rt->getChild(i);
				renderNode( n );
				E_RELEASE(n);
			}

			if ( rt )
			{
				// reset rendertarget to backbuffer:
				renderer->end3D();
				renderer->setRenderTarget( 0 );
				renderer->begin3D();
				renderer->setCamera( cam );
			}
			//_p.stop();
		}


		bool renderFirstCam( Core::Node * node )
		{
			Core::Camera * cc = (Core::Camera*)node->cast("Core.Camera");
			if ( cc && cc->getVisibility() )
			{
				renderer->setCamera( cc );
				E_RELEASE( cc );
				return 1;
			}
			for ( uint i=0; i<node->numChildren(); i++ )
			{
				Core::Node * n = node->getChild(i);
				bool ok = renderFirstCam( n );
				E_RELEASE(n);
				if ( ok ) break;
			}
			E_RELEASE( cc );
			return 0;
		}


		void renderNode( Core::Node * node )
		{
			bool rendered = 0;
			//Core::Camera * cc = 0;
			if ( ! node->getVisibility() )
			{
				return;
			}
			if ( ! rendered )
			{
				// try mesh
				Core::Mesh * mesh = (Core::Mesh*)node->cast("Core.Mesh");
				if ( mesh )
				{
					rendered = renderer->setMesh( mesh );
					E_RELEASE( mesh );
				}
			}
			if ( ! rendered )
			{
				// try RenderToTexture
				Core::RenderToTexture * rt = (Core::RenderToTexture*)node->cast("Core.RenderToTexture");
				if ( rt )
				{
					// render children into rt:
					renderToTex( rt );
					E_RELEASE( rt );
					return;
				}
			}

			for ( uint i=0; i<node->numChildren(); i++ )
			{
				Core::Node * n = node->getChild(i);
				renderNode( n );
				E_RELEASE(n);
			}
		}


		void renderLight( Core::Node * node, uint & ln )
		{
			Core::Light * light = (Core::Light*)node->cast("Core.Light");
			if ( light && light->getVisibility() )
			{
				renderer->setLight( ln++, light );
			}
			for ( uint i=0; i<node->numChildren(); i++ )
			{
				Core::Node * n = node->getChild(i);
				renderLight( n, ln );
				E_RELEASE(n);
			}
			E_RELEASE( light );
		}

		void renderScene()
		{
			if ( ! running ) return; 
			if ( ! renderer ) return ;

			Core::Node * root  = world->getRoot();	
			root->synchronize();
			e6::float4 ambient(0.2f,0.2f,0.2f,1.0f);
			float c[4] = { timeNow, sinf(timeNow), cosf(timeNow), timeStep };
			renderer->setVertexShaderConstant( e6::VS_TIME, c );
			renderer->setVertexShaderConstant( e6::VS_LIGHT_COLOR_AMBIENT, ambient );

			renderer->clear();
			renderer->begin3D();
			renderer->setCamera( cam );

			uint nLights=0;
			renderLight(root,nLights);

			renderNode(root);

			renderer->end3D();
			renderer->swapBuffers();
			E_RELEASE(root);	
		}

	
	}; // CMain

}; //Application



extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Application.Main",	 "Application",	Application::CMain::createInterface, Application::CMain::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Application 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
