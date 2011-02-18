#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../Core/Core.h"



using e6::uint;
using e6::ClassInfo;


namespace TexNoize
{
	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{


		int noise()
		{
			static int z=523531;
			int z2 = ((((z+422523)<<7)+0xf44523 )>>3);
			return (z = unsigned (z2) % 255);
		}

		void genPixels( Core::Texture * tex, int w )
		{
			tex->alloc( 0, w, w, 1 );
			Core::Buffer * pixel = tex->getLevel(0);
			unsigned char c[3];
			unsigned char *pix = 0;
			if ( ! pixel->lock( (void**)&pix, 0 ) )
			{
				e6::sys::alert( __FUNCTION__, " could not lock pixels!" );
				return ;
			}
			for ( int j = 0; j < w; j++) 
			{
				for (int i = 0; i < w; i++) 
				{
					*pix ++ = noise();
					*pix ++ = noise();
					*pix ++ = noise();
					*pix ++ = 0xff;
				}
			}
			pixel->unlock();	
			E_RELEASE( pixel );			
		}


		virtual uint load(e6::Engine * e, Core::World * w, const char * path) 
		{
			$1(path);
			Core::Texture * tex = (Core::Texture*) e->createInterface( "Core", "Core.Texture");
			if ( tex )
			{
				tex->setName( path );
				genPixels( tex, 128 );
				w->textures().add( tex->getName(), tex );
			}
			E_RELEASE( tex );
			E_RELEASE( w );
			E_RELEASE( e );
			return 1;
		}
				
	};
};

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Importer",	 "ImportTexNoize",	TexNoize::CImporter::createInterface	},
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
	mv->modVersion = ( "ImportTexNoize 00.00.1 " __DATE__ );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
