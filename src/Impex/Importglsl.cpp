#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../Core/Core.h"



using e6::uint;
using e6::ClassInfo;


namespace Importglsl
{
	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{

		virtual uint load(e6::Engine * e, Core::World * w, const char * orig) 
		{
			bool ok = 0;
			{	
				Core::Shader * sha = (Core::Shader*) e->createInterface( "Core", "Core.Shader");
				if ( sha )
				{
					char path[200];
					char name[200];

					e->chopPath( orig, path, name );
					sha->setName( name );
					sha->setPath( orig );
					ok = w->shaders().add( sha->getName(), sha );
					E_RELEASE( sha );
				}
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
		{	"Core.Importer",	 "Importglsl",	Importglsl::CImporter::createInterface,Importglsl::CImporter::classRef	},
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
	mv->modVersion = ( "Importglsl 00.00.1 " __DATE__ );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
