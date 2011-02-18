#include "e6_sys.h"
#include "e6_impl.h"
#include "e6_enums.h"
#include "../Core/Core.h"
#include "../Gui/Gui.h"
#include "../TimeLine/TimeLine.h"

#include <Windows.h>


struct MyApp 
	: e6::CName< Gui::Application, MyApp >
{
	e6::Engine * engine;
	Core::Renderer * renderer;	
	Core::World * world;	
	Core::Camera * cam;
	Gui::AppWindow * win;
	
	MyApp()
	{
		engine = e6::CreateEngine();	
		renderer  = (Core::Renderer *)engine->createInterface( "RendererDx9", "Core.Renderer" );	
		world  = (Core::World *)engine->createInterface( "Core", "Core.World" );	
		cam = (Core::Camera *)engine->createInterface( "Core", "Core.Camera" );
		win  = (Gui::AppWindow *)engine->createInterface( "GuiW32", "Gui.AppWindow" );	
	}

	~MyApp()
	{
		printf( __FUNCTION__ "\n");
		fini();
	}

	void fini()
	{
		E_RELEASE(renderer);	
		E_RELEASE(win);	
		E_RELEASE(world);	
		E_RELEASE(cam);	
		E_RELEASE(engine);	
	}

	void run( int argc, char **argv )
	{
		win->setSize( 40,40, 400,400 );
		win->show();

		for ( int i=1; i<argc; i++ )
		{
			world->load( engine, argv[i] );
		}

		Core::Node * root = world->getRoot();
		cam = (Core::Camera *)engine->createInterface( "Core", "Core.Camera" );
		cam->setPos( e6::float3(0,0,40) );
		root->link( cam );
		E_RELEASE( root );

        win->run( this, 400,400, "Cosmo" );
	}

	virtual uint onIdle()
	{
		if ( win && renderer && world )
			render();
		return 1;
	}

	virtual uint onInit( void * w )
	{
		printf( __FUNCTION__ "  :  %x \n", w );
		uint r = renderer->init( w, 400,400 );
		return r;
	}
	virtual uint onExit()
	{
		//fini();
		return 1;
	}
	virtual uint onKey( uint k )
	{
		return 0; // NOT_IMPL
	}
	virtual uint onMouse( uint state, uint x, uint y )
	{
		return 0; // NOT_IMPL
	}
	virtual uint onResize( uint w, uint h )
	{
		return 0; // NOT_IMPL
	}
	void render( Core::Node * node )
	{
		Core::Mesh * mesh = (Core::Mesh*)node->cast("Mesh");
		if ( mesh )
		{
			renderer->setMesh( mesh );
			E_RELEASE( mesh );
			printf(".\n");
		}
		for ( uint i=0; i<node->numChildren(); i++ )
		{
			Core::Node * n = node->getChild(i);
			render( n );
			E_RELEASE(n);
		}
	}

	void render()
	{
		Core::Node * root  = world->getRoot();	
		cam->addRot( e6::float3(0,0.01,0) );
		root->synchronize();

		renderer->clear(0);
		renderer->setCamera( cam );

		renderer->begin3D();
		render(root);
		renderer->end3D();

		renderer->swapBuffers();

		E_RELEASE(root);	
	}
};






int main(int argc, char **argv)
{
	printf("!\n%s\n", argv[0]);

	MyApp * app = new MyApp;
	app->run( argc, argv );
	
	printf( __FUNCTION__ " finished \n");
	E_RELEASE(app);	
	printf("!\n");
	return 0;
}
