#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../Core/Core.h"



using e6::uint;


int main(int argc, char **argv)
{
	char * in = argv[1];
	e6::Engine * e = e6::CreateEngine();	
	Core::World * world = (Core::World*) e->createInterface( "Core", "Core.World" );
	world->load( e, in );

	char p[300], s[300], ex[30]; 
	e->chopExt( in, p, ex );
	const char * r = e->getResPath();
	sprintf( s, "%s/e6/%s.e6", r, p );
	printf( ".. saving to %s.\n", s );
	world->save( e, s );
	E_RELEASE(world);	
	E_RELEASE(e);	
	return 0;
}
