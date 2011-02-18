#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../Core/Core.h"



using e6::uint;
using e6::ClassInfo;


namespace Importdds
{
	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{
		virtual uint load(e6::Engine * e, Core::World * w, const char * orig) 
		{
			bool ok = 0;
			{	
				Core::Texture * tex = (Core::Texture*) e->createInterface( "Core", "Core.Texture");
				if ( tex )
				{
					char bpath[200];
					char bname[200];

					if ( ! e->chopPath( orig, bpath, bname ) )
					{
						e6::sys::alert( __FUNCTION__, "path(%s) contains no name!",orig );
						return 0;
					}
					tex->setName( bname );
					tex->setPath( orig );
					ok = w->textures().add( tex->getName(), tex );
					E_RELEASE( tex );
				}
			}
			//E_RELEASE( w );
			//E_RELEASE( e );
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
		{	"Core.Importer",	 "Importdds",	Importdds::CImporter::createInterface,Importdds::CImporter::classRef	},
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
	mv->modVersion = ( "Importdds 00.00.1 " __DATE__ );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
