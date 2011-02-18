#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../Core/Core.h"

#include "bmp/EasyBMP.h"


using e6::uint;
using e6::ClassInfo;


namespace Ebmp
{
	struct CExporter 
		: public e6::Class< Core::Exporter, CExporter >
	{
/*
		void writePixels( FILE * fp, Core::Buffer * pixel, int w, int h )
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
					c[2] = *pix ++;
					c[1] = *pix ++;
					c[0] = *pix ++;
					pix ++;
					fwrite( c, 1, 3, fp );
				}
			}
			pixel->unlock();	
			E_RELEASE( pixel );			
		}
		
				
		uint writeBMP( const char* filename, Core::Texture * tex ) 
		{
			FILE *bmb = fopen(filename, "wb");
			if ( ! bmb ) 
			{
				e6::sys::alert( __FUNCTION__, "could not open %s for writing.\n", filename );
				return 0;
			}
			int width,height;
			short ds = 0;
			int   di = 0;
			int   off = 0;
			short bits = 0;
			fwrite( &ds,     sizeof(short), 1, bmb );
			fwrite( &di,     sizeof(int),   1, bmb );
			fwrite( &ds,     sizeof(short), 1, bmb );
			fwrite( &ds,     sizeof(short), 1, bmb );
			fwrite( &off,    sizeof(int),   1, bmb );
			fwrite( &di,     sizeof(int),   1, bmb );
			fwrite( &width,  sizeof(int),   1, bmb );
			fwrite( &height, sizeof(int),   1, bmb );
			fwrite( &ds,     sizeof(short), 1, bmb );
			fwrite( &bits,   sizeof(short), 1, bmb );
			fwrite( &di,     sizeof(int),   1, bmb );
			fwrite( &di,     sizeof(int),   1, bmb );
			fwrite( &di,     sizeof(int),   1, bmb );
			fwrite( &di,     sizeof(int),   1, bmb );
			fwrite( &di,     sizeof(int),   1, bmb );
			fwrite( &di,     sizeof(int),   1, bmb );

			writePixels(bmb, tex->getLevel(0), width, height);

			fclose(bmb);			
			return 1;
		}
				
*/
		uint saveBMP( const char* filename, Core::Texture * tex ) 
		{
			BMP bmp;
			uint d = 32;
			uint w = tex->width();
			uint h = tex->height();
		    bmp.SetSize( w,h );
		    bmp.SetBitDepth( d );

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
				    RGBApixel p;
					p.Red	= *pix++;
					p.Green	= *pix++;
					p.Blue	= *pix++;
					p.Alpha	= 0xff - *pix++;
                    bmp.SetPixel( j,i, p );
				}
			}
			pixel->unlock();	
			E_RELEASE( pixel );			
		    return bmp.WriteToFile( filename );
		}


		virtual uint save (e6::Engine * e, Core::World * w, const char * path, void * item ) 
		{
			$1(path);
			Core::Texture * tex = (Core::Texture*) item;
			uint r =  saveBMP( path, tex ) ;
			// E_RELEASE( tex );
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
		{	"Core.Exporter",	 "Exportbmp",	Ebmp::CExporter::createInterface, Ebmp::CExporter::classRef	},
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
	mv->modVersion = ( "Exportbmp 00.00.1 " __DATE__ );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
