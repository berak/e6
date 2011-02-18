#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../Core/Core.h"
#include "png.h"



using e6::uint;
using e6::ClassInfo;


namespace Ipng
{
	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{

		uint readPNG( const char* filename, Core::Texture * tex ) 
		{
			FILE * fp;
		    png_byte sig[8];
		    int bit_depth, color_type;
		    double              gamma;
		    png_uint_32 channels, row_bytes;
		    png_byte **row_pointers = 0;
			png_structp png_ptr = 0;
		    png_infop info_ptr = 0;

			
		    // open the PNG input file
		    if (!filename) return 0;
			
		    if (!(fp = fopen(filename, "rb"))) 
			{
				return 0;
			}
		    // first check the eight byte PNG signature
		    fread(sig, 1, 8, fp);
		    if (!png_check_sig(sig, 8)) 
			{ 
				fclose(fp); 
				return 0; 
			}
			
			
			// start back here!!!!
			
		    // create the two png(-info) structures
			
		    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		    if (!png_ptr) { fclose(fp); return 0; }
			
		    info_ptr = png_create_info_struct(png_ptr);
		    if (!info_ptr)
		    {
		        png_destroy_read_struct(&png_ptr, 0, 0);
				fclose(fp);
				return 0;
		    }
			
			// initialize the png structure
		    png_init_io(png_ptr, fp);	

			png_set_sig_bytes(png_ptr, 8);
			
			// read all PNG info up to image data
			png_read_info(png_ptr, info_ptr);
			
			// get width, height, bit-depth and color-type	
			png_uint_32 w, h;
			png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, 0, 0, 0);
			
			// expand images of all color-type and bit-depth to 3x8 bit RGB images
			// let the library process things like alpha, transparency, background
			
			if (bit_depth == 16) png_set_strip_16(png_ptr);
			if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_expand(png_ptr);
			if (bit_depth < 8) png_set_expand(png_ptr);
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_expand(png_ptr);
			if (color_type == PNG_COLOR_TYPE_GRAY ||
				color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
				png_set_gray_to_rgb(png_ptr);
			
			// if required set gamma conversion
			if (png_get_gAMA(png_ptr, info_ptr, &gamma)) png_set_gamma(png_ptr, (double) 2.2, gamma);
			
			// after the transformations have been registered update info_ptr data
			png_read_update_info(png_ptr, info_ptr);
			
			// get again width, height and the new bit-depth and color-type
			png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, 0, 0, 0);
			
			
			// row_bytes is the width x number of channels
			row_bytes = png_get_rowbytes(png_ptr, info_ptr);
			channels = png_get_channels(png_ptr, info_ptr);
				
			// now we can allocate memory to store the image

		    png_byte * img = new png_byte[row_bytes * h];
			
			// and allocate memory for an array of row-pointers

			png_byte ** row = new png_byte * [h];

			
			// set the individual row-pointers to point at the correct offsets
			
			for (unsigned int i = 0; i < h; i++)
				row[i] = img + i * row_bytes;
			
			// now we can go ahead and just read the whole image
			
			png_read_image(png_ptr, row);
			
			// read the additional chunks in the PNG file (not really needed)
			
			png_read_end(png_ptr, NULL);
			


			tex->alloc( 0, w, h, 1 );
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
				png_byte * p = img + ((h-j-1)*row_bytes );
				for ( uint i = 0; i < w; i++ ) 
				{
					*pix++ = 0xff;
					*pix++ = *p++;
					*pix++ = *p++;
					*pix++ = *p++;
				}
			}
			pixel->unlock();	
			E_RELEASE( pixel );			


			delete [] row;
			delete [] img;

		    png_destroy_read_struct(&png_ptr, 0, 0);

		    fclose (fp);
			return 1;
		}

		virtual uint load(e6::Engine * e, Core::World * w, const char * path) 
		{
			$1(path);
			Core::Texture * tex = (Core::Texture*) e->createInterface( "Core", "Core.Texture");
			uint r =  readPNG( path, tex ) ;
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
		{	"Core.Importer",	 "Importpng",	Ipng::CImporter::createInterface, Ipng::CImporter::classRef	},
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
	mv->modVersion = ( "Importpng 00.00.1 " __DATE__ );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
