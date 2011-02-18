#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../e6/e6_sys.h"
#include "../Core/Core.h"
#include "bmp/EasyBMP.h"



using e6::uint;
using e6::ClassInfo;


namespace Ibmp
{
	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{

/**
		void readPixels( FILE * fp, Core::Buffer * pixel, int w, int h )
		{
			unsigned char c[3];
			unsigned char *pix = 0;
			if ( ! pixel->lock( (void**)&pix, 0 ) )
			{
				e6::sys::alert( __FUNCTION__, " could not lock pixels!" );
				return ;
			}
			for ( int j = 0; j < h; j++) 
			{
				for (int i = 0; i < w; i++) 
				{
					fread( c, 1, 3, fp );
					*pix ++ = c[2];
					*pix ++ = c[1];
					*pix ++ = c[0];
					*pix ++ = 0xff;
				}
			}
			pixel->unlock();	
			E_RELEASE( pixel );			
		}
		
				
		uint readBMP( const char* filename, Core::Texture * tex ) 
		{
			FILE *bmb = fopen(filename, "rb");
			if ( ! bmb ) 
			{
				e6::sys::alert( __FUNCTION__, "could not load : %s\n", filename );
				return 0;
			}
			int width,height;
			short ds = 0;
			int   di = 0;
			int   off = 0;
			short bits = 0;
			fread( &ds,     sizeof(short), 1, bmb );
			fread( &di,     sizeof(int),   1, bmb );
			fread( &ds,     sizeof(short), 1, bmb );
			fread( &ds,     sizeof(short), 1, bmb );
			fread( &off,    sizeof(int),   1, bmb );
			fread( &di,     sizeof(int),   1, bmb );
			fread( &width,  sizeof(int),   1, bmb );
			fread( &height, sizeof(int),   1, bmb );
			fread( &ds,     sizeof(short), 1, bmb );
			fread( &bits,   sizeof(short), 1, bmb );
			fread( &di,     sizeof(int),   1, bmb );
			fread( &di,     sizeof(int),   1, bmb );
			fread( &di,     sizeof(int),   1, bmb );
			fread( &di,     sizeof(int),   1, bmb );
			fread( &di,     sizeof(int),   1, bmb );
			fread( &di,     sizeof(int),   1, bmb );
			assert( bits == 24 );			
			tex->alloc( 0, width, height, 1 );
			readPixels(bmb, tex->getLevel(0), width, height);
			fclose(bmb);			
			return 1;
		}
				
**/
		uint readBMP( const char* filename, Core::Texture * tex ) 
		{
			BMP bmp;
			if ( ! bmp.ReadFromFile(filename) )
			{
				return 0;
			}
			uint w = bmp.TellWidth();
			uint h = bmp.TellHeight();
			uint d = bmp.TellBitDepth();

			// check img diagonal for alpha pixels:
			uint pf = e6::PF_X8B8G8R8;			
			for ( uint i=0,j=0; (j<h && i<w); j++,i++) 
			{
				RGBApixel p = bmp.GetPixel(i,j);
				//if ( p.Alpha != 0xff )
				if ( p.Alpha != 0 )
				{
					pf = e6::PF_A8B8G8R8;
					break;
				}
			}
					
			tex->alloc( pf, w, h, 1 );
			Core::Buffer * pixel = tex->getLevel(0);

			unsigned char *pix = 0;
			if ( ! pixel->lock( (void**)&pix, 0 ) )
			{
				e6::sys::alert( __FUNCTION__, " could not lock pixels!" );
				E_RELEASE( pixel );			
				return 0;
			}
			for ( uint j = 0; j < h; j++ ) 
			{
				for ( uint i = 0; i < w; i++ ) 
				{
				    RGBApixel p = bmp.GetPixel(i,j);
					*pix ++ = p.Red;
					*pix ++ = p.Green;
					*pix ++ = p.Blue;
					*pix ++ = 0xff - p.Alpha;
				}
			}
			pixel->unlock();	
			E_RELEASE( pixel );	
			return 1;
		}

		virtual uint load(e6::Engine * e, Core::World * w, const char * path) 
		{
			$1(path);
			Core::Texture * tex = (Core::Texture*) e->createInterface( "Core", "Core.Texture");
			uint r =  readBMP( path, tex ) ;
			if (r)
			{
				char bpath[200];
				char bname[200];

				if ( ! e->chopPath( path, bpath, bname ) )
				{
					e6::sys::alert( __FUNCTION__, "path(%s) contains no name!",path );
					return 0;
				}
				tex->setName( bname );
				tex->setPath( path );
				w->textures().add( tex->getName(), tex );
			}
			E_RELEASE( tex );
			//E_RELEASE( w );
			//E_RELEASE( e );
			return r;
		}
				
	};
};

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Importer",	 "Importbmp",	Ibmp::CImporter::createInterface, Ibmp::CImporter::classRef	},
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
	mv->modVersion = ( "Importbmp 00.00.1 " __DATE__ );
	mv->e6Version =	e6::e6_version;
	return 1; 
}

