#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../Core/Core.h"



using e6::uint;
using e6::ClassInfo;


namespace Importhlsl
{
	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{
		virtual uint load(e6::Engine * e, Core::World * w, const char * orig) 
		{
			bool ok = 0;
			{
				if ( e6::sys::fileExists(orig) ) 
				{
					Core::Shader * sha = (Core::Shader*) e->createInterface( "Core", "Core.Shader");
					if ( sha )
					{
						char bpath[200];
						char bname[200];
						if ( ! e->chopPath( orig, bpath, bname ) )
						{
							e6::sys::alert( __FUNCTION__, "path(%s) contains no name!",orig );
							return 0;
						}
						sha->setName( bname );
						sha->setPath( orig );

						ok = w->shaders().add( sha->getName(), sha );
						
						if ( ok  ) 
						{
							Core::Shader * test = w->shaders().get( orig );
							if ( test )
								printf( "ok. %s.\n", orig ); 
							E_RELEASE(test);
						}
						E_RELEASE( sha );
					}
				}
			}
			if ( ! ok )
			{
				e6::sys::alert( __FUNCTION__, "could not load hlsl shader '%s'\r\n from path !", orig, e->getPath() );
			}
			return ok;
		}
				
	};
};

using e6::ClassInfo;

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Importer",	 "Importhlsl",	Importhlsl::CImporter::createInterface,Importhlsl::CImporter::classRef	},
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
	mv->modVersion = ( "Importhlsl 00.00.1 " __DATE__ );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
