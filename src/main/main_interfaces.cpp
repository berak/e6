#include "e6_sys.h"
#include "e6_impl.h"
#include "e6_enums.h"

using e6::uint;

struct InterfaceList 
	: public e6::InterfaceCallback
{
	uint call( const char * interfaceName, const char * moduleName, uint count )
	{
		printf( "\t%-20s%-20s\n", interfaceName, moduleName );
		return 1;
	}

    bool run( e6::Engine * x, const char * iname )
    {
		assert(x);

		x->findInterfaces( iname, *this );
    	return 1;
   	}
};

int main(int argc, char **argv)
{
	char * iname = "*";
	if ( argc>1 ) iname = argv[1];

	InterfaceList ilist;
	e6::Engine * e = e6::CreateEngine();	
	e->loadAllModules();

	ilist.run( e, iname );
	
	E_RELEASE(e);	
	return 0;
}
