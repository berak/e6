#include "Dx9.h"

namespace Dx9
{

	namespace SurfaceHelper
	{
		uint saveSurface( LPDIRECT3DSURFACE9 surface, const char *fileName,	RECT  * rect )
		{
			const char * extSupported[] = 
			{
				".bmp",".jpg",".tga",".png",".dds",".ppm",".dib",".hdr",".pfm"
			};
			for ( uint i=0; i<9; i++ )
			{
				if ( strstr( fileName, extSupported[i] ) )
				{
					HRESULT hr = D3DXSaveSurfaceToFile( fileName, (D3DXIMAGE_FILEFORMAT)i , surface, 0, rect );
					return hr == S_OK;
				}
			}
			return 0;
		}
		uint copyToSurface( uint src_w, uint src_h, uint src_bpp, const unsigned char *src_pix, IDirect3DSurface9 *surf )
		{
			D3DLOCKED_RECT lockedRect;
			HRESULT hr;

			hr = surf->LockRect(   
				&lockedRect,
				NULL,             // const RECT *pRect,
				0 //D3DLOCK_DISCARD  // DWORD Flags
			);

			if ( hr != D3D_OK ) 
			{
				LOG_ERROR(hr);
				return 0;
			}

			D3DSURFACE_DESC desc;
			surf->GetDesc( &desc );
	 
			uint dst_w = min(src_w,desc.Width );
			uint dst_h = min(src_h,desc.Height );

			uint src_row  = src_w * src_bpp;

			uint pitch = lockedRect.Pitch;
			unsigned char *dst_pix = (unsigned char *) lockedRect.pBits;

			if ( src_bpp == 3 ) 
			{
				uint r,g,b, a=0xff;
				for ( uint j = 0; j < dst_h; j++ )
				{
					unsigned char * pPixel = dst_pix + j *pitch;
					unsigned char * s = (unsigned char*)src_pix + j * src_row;
					for ( uint i = 0; i < dst_w; i++ )
					{
						b = *s++;
						g = *s++;
						r = *s++;
						*pPixel++ = r;
						*pPixel++ = g;
						*pPixel++ = b;
						*pPixel++ = a;
					}
				}
			}
			else  if ( src_bpp == 4 )
			{
				uint r,g,b, a=0xff;
				for ( uint j = 0; j < dst_h; j++ )
				{
					unsigned char * pPixel = dst_pix + j *pitch;
					unsigned char * s = (unsigned char*)src_pix + j * src_row;
					for ( uint i = 0; i < dst_w; i++ )
					{
						b = *s++;
						g = *s++;
						r = *s++;
						a = *s++;
						*pPixel++ = r;
						*pPixel++ = g;
						*pPixel++ = b;
						*pPixel++ = a;
					}
				}
				//for ( uint j=0, src_scan=0, dst_scan=0; j<(uint)src_h; j++, dst_scan+=pitch, src_scan+=src_row ) 
				//{
		  //          unsigned char *pix_in  = (unsigned char*)src_pix + src_scan;
		  //          unsigned char *pix_out = dst_pix + dst_scan;
				   // memcpy( pix_out, pix_in, src_row ); 
				//}
			}


			hr = surf->UnlockRect();
			E_TRACE(hr);
			return hr == S_OK;
		}

		uint copyFromSurface( uint dst_w, uint dst_h, uint dst_bpp, unsigned char *dst_pix, IDirect3DSurface9 *surf )
		{
			D3DLOCKED_RECT lockedRect;
			HRESULT hr;
			hr = surf->LockRect(   
				&lockedRect,
				NULL,             // const RECT *pRect,
				0 //D3DLOCK_READONLY  // DWORD Flags
			);

			if ( hr != S_OK ) 
			{
				LOG_ERROR(hr);
				return 0;
			}

			D3DSURFACE_DESC desc;
			surf->GetDesc( &desc );
	 
			uint src_w = min(dst_w,desc.Width );
			uint src_h = min(dst_h,desc.Height );

			uint dst_row  = dst_w * dst_bpp;

			uint pitch = lockedRect.Pitch;
			unsigned char *src_pix = (unsigned char *) lockedRect.pBits;

			if ( dst_bpp == 3 ) // copy single pixels:
			{
				for ( uint j = 0; j < src_h; j++ )
				{
					unsigned char * d = dst_pix + j * dst_row;
					unsigned char * pPixel = src_pix + j *pitch;
					for ( uint i = 0; i < src_w; i++ )
					{
						*d++ = *pPixel++;
						*d++ = *pPixel++;
						*d++ = *pPixel++;
						pPixel++;
					}
				}
			}
			else  if ( dst_bpp == 4 ) // copy whole lines:
			{
				for ( uint j=0, scan_in=(src_h-1)*pitch, scan_out=0; j<src_h; j++, scan_in-=pitch, scan_out+=dst_row ) 
				{
					unsigned char *pix_in   = src_pix + scan_in;
					unsigned char *pix_out = dst_pix + scan_out;
					memcpy( pix_out, pix_in, dst_row ); 
				}
			}

			hr = surf->UnlockRect();
			E_TRACE(hr);
			return hr == S_OK;
	   }

		uint saveSurfaceBmp( IDirect3DSurface9 *surf, const char *fileName, uint alphaColor )
		{
			D3DLOCKED_RECT lockedRect;
			HRESULT hr;
			hr = surf->LockRect(   
				&lockedRect,
				NULL,             // const RECT *pRect,
				0//D3DLOCK_READONLY  // DWORD Flags
			);

			if ( hr != S_OK ) 
			{
				LOG_ERROR(hr);
				return 0;
			}

			D3DSURFACE_DESC desc;
			hr = surf->GetDesc( &desc );
	 
			uint depth = alphaColor ? 4 : 3;
			uint size = desc.Width * desc.Height * depth;
			uint pitch = lockedRect.Pitch;
			uint numPixel = desc.Width * desc.Height;
			uint lineSize  = desc.Width * depth;
			unsigned char *src_pix = (unsigned char *) lockedRect.pBits;



			// fill BITMAPINFOHEADER
			BITMAPINFOHEADER bmih;
			uint bmihSize = sizeof(BITMAPINFOHEADER);
			memset( &bmih, 0, bmihSize );
			
			bmih.biWidth = desc.Width;
			bmih.biHeight = desc.Height;
			bmih.biBitCount = depth * 8;
			bmih.biSize = size;
			bmih.biCompression = BI_RGB;
			bmih.biClrUsed = 0;
			bmih.biPlanes = 0;

			// fill BITMAPFILEHEADER
			BITMAPFILEHEADER bmfh;
			uint bmfhSize = sizeof(BITMAPFILEHEADER);
			memset( &bmfh, 0, bmfhSize );

			bmfh.bfSize = bmfhSize;
			bmfh.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER)
									+ bmih.biSize + bmih.biClrUsed * sizeof (RGBQUAD); 
			bmfh.bfType = 'MB';
			
			// open file:
			FILE * bmp = fopen( fileName, "wb" );	   
			if ( ! bmp ) 
			{
				hr = surf->UnlockRect();
				return 0;
			}

			// write BITMAPFILEHEADER
			fwrite(  &bmfh, bmfhSize, 1, bmp );

			// write BITMAPINFOHEADER
			fwrite( &bmih, bmihSize, 1, bmp );



			// write rgb pixels:
			if ( alphaColor == 0 ) // copy single pixels:
			{
				char * surfPixel = (char *) lockedRect.pBits;
				char * d_in = surfPixel;
				char * buf = new char[ lineSize ];
				char * d_out = buf;
				for ( uint j = 0; j < desc.Height; j++ )
				{
					d_in = surfPixel + j * pitch;
					char * d_out = buf;

					for ( uint i = 0; i < desc.Width; i++ )
					{                  
						*d_out ++ = *d_in++;
						*d_out ++ = *d_in++;
						*d_out ++ = *d_in++;
						d_in++; // skip 4.th channel
					}

					fwrite( buf, lineSize, 1, bmp );
				}
				delete [] buf;
			}
			else
			// write rgba pixels, make every alphaColor - pixel tranparent white:
			{
				uint * surfPixel = (uint *) lockedRect.pBits;
				uint * d_in = surfPixel;
				uint * buf = new uint [ desc.Width ];
				for ( uint j = 0; j < desc.Height; j++ )
				{
					uint * d_out = buf;
					d_in = surfPixel + j * pitch;

					for ( uint i = 0; i < desc.Width; i++ )
					{
						if ( *d_in == alphaColor ) 
						{
							*d_out = 0x00ffffff; // transparent white.
						}
						else
						{
							*d_out = *d_in;
						}

						d_out++, d_in++;
					}

					fwrite( buf, lineSize, 1, bmp );
				}
				delete [] buf;
			}

			fclose( bmp );

			hr = surf->UnlockRect();
			//E_TRACE(hr);
			return hr == S_OK;

		}
	} //SurfaceHelper

	void TextureCache::setDevice( LPDIRECT3DDEVICE9 dev )
	{
		device = (dev);

		//unsigned char pix[32*32*4];
		//unsigned char *d = pix;
		//for ( int i = 0; i < 32; i++) {
		//	for ( int j = 0; j < 32; j++) {
		//		unsigned char c = ((((i&8)==0)^((j&8)==0)))*255;
		//		*d++ = 0xff;
		//		*d++ = c;
		//		*d++ = c;
		//		*d++ = 0xff;
		//	}
		//}
		//IDirect3DTexture9 * err = _addTex( e6::PF_X8B8G8R8, 32,32, 4, pix  );
		//insert( 0, err,0xffffffff );
	}

    IDirect3DTexture9 * TextureCache::createTexture( uint width, uint height, uint format , uint usage, uint pool  )
	{
        IDirect3DTexture9 * pTexture = 0;

		uint levels = (usage & D3DUSAGE_AUTOGENMIPMAP) ? 0 : 1;

		printf( "%s %i %i %x %x %x %x\n", __FUNCTION__, width, height, levels, usage, format, pool );
		HRESULT hr = device->CreateTexture( width, height, levels, usage, (D3DFORMAT)format, (D3DPOOL)pool, & pTexture, 0 );
        E_TRACE(hr);
		return pTexture;
	}



	D3DFORMAT getPixelFormat( uint format )
    {
		if ( format <= e6::PF_A32B32G32R32F )
        {
            const D3DFORMAT fmt[] = 
            {
                D3DFMT_UNKNOWN, //        PF__NONE                	= 0x00000000,
        // integer:
                D3DFMT_R8G8B8, //        PF__R8G8B8              		= 0x00000001,
                D3DFMT_X8R8G8B8, //		PF__X8R8G8B8  			= 0x00000002,
                D3DFMT_A8R8G8B8, //		PF__A8R8G8B8  			= 0x00000003,
                D3DFMT_A16B16G16R16, //		PF__A16R16G16B16		= 0x00000005,
        // float:
                D3DFMT_R16F, //        PF__R16F           			= 0x00000008,
                D3DFMT_G16R16F, //		PF__G16R16F      			= 0x00000009,
                D3DFMT_A16B16G16R16F, //		PF__A16B16G16R16F	= 0x0000000a,
                D3DFMT_R32F, //		PF__R32F           			= 0x0000000b,
                D3DFMT_G32R32F, //		PF__G32R32F      			= 0x0000000c,
                D3DFMT_A32B32G32R32F, //		PF__A32B32G32R32F	= 0x0000000d,
            };

            return fmt[format];
        }
		return D3DFMT_UNKNOWN;
	}

	IDirect3DTexture9 * TextureCache::_addTex( uint fmt, uint w, uint h, uint c, const unsigned char * pixel, uint _usage )
	{
		uint usage = _usage;
		uint pool = D3DPOOL_MANAGED;

		if ( usage & D3DUSAGE_DYNAMIC ) // The resource will be a render target. D3DUSAGE_RENDERTARGET can only be used with D3DPOOL_DEFAULT. 
		{
			pool = D3DPOOL_DEFAULT;
			
		}
		if ( usage & D3DUSAGE_RENDERTARGET ) // The resource will be a render target. D3DUSAGE_RENDERTARGET can only be used with D3DPOOL_DEFAULT. 
		{
			pool = D3DPOOL_DEFAULT;
			
		}
		else // no usage
		{
			usage |= D3DUSAGE_AUTOGENMIPMAP;
		}

		//printf( "%s %s %x\n", __FUNCTION__, ("nil"), fmt);

		IDirect3DTexture9 * tx = createTexture( w, h, getPixelFormat(fmt), usage, pool );
		if ( ! tx ) 
		{
			printf( __FUNCTION__ " ERROR : could not create tex \n" );
			return 0;
		}
		if ( pixel )
		{
			IDirect3DSurface9 *surf = 0;
			HRESULT hr = tx->GetSurfaceLevel( 0, & surf );
			E_TRACE(hr);

			uint r = SurfaceHelper::copyToSurface( w, h, c, pixel, surf );

			COM_RELEASE( surf );
		}
		return tx;
	}
	IDirect3DCubeTexture9 * TextureCache::_addCubeTex( const char * srcFile, uint _usage )
	{
		uint pool = D3DPOOL_MANAGED;
		if ( _usage & D3DUSAGE_RENDERTARGET ) // The resource will be a render target. D3DUSAGE_RENDERTARGET can only be used with D3DPOOL_DEFAULT. 
			pool = D3DPOOL_DEFAULT;
		uint usage = _usage; // | D3DUSAGE_AUTOGENMIPMAP;
		printf( "%s %s\n", __FUNCTION__, srcFile );

        IDirect3DCubeTexture9 * pTexture = 0;

		uint mipLevels = 0;
        D3DXIMAGE_INFO imgInfo;
        HRESULT hr = D3DXCreateCubeTextureFromFileEx(
            device,						//LPDIRECT3DDEVICE9 pDevice,
            srcFile,						//LPCTSTR pSrcFile,
            D3DX_DEFAULT,		//UINT Size,
            mipLevels,				//UINT MipLevels,
            usage,//_usage &  (D3DUSAGE_DYNAMIC | D3DUSAGE_RENDERTARGET),					// DWORD Usage,
            D3DFMT_FROM_FILE, //_format, // D3DFORMAT Format,
			(D3DPOOL) pool,						//D3DPOOL Pool,
            D3DX_DEFAULT ,		//DWORD Filter,
            D3DX_DEFAULT ,		//DWORD MipFilter,
            0,								//D3DCOLOR ColorKey,
            & imgInfo,				// D3DXIMAGE_INFO
            0,								//PALETTEENTRY *pPalette,
            & pTexture				//LPDIRECT3DCUBETEXTURE9 *ppCubeTexture
        );
        E_TRACE(hr);
        if ( FAILED( hr ) )
		{
			return 0;
		}

		return pTexture;
	}

	IDirect3DVolumeTexture9 * TextureCache::_addVolumeTex( const char * srcFile, uint _usage  )
	{
		HRESULT hr = 0;
        IDirect3DVolumeTexture9 * pTexture = 0;
		uint pool = D3DPOOL_MANAGED;
		if ( _usage & D3DUSAGE_RENDERTARGET ) // The resource will be a render target. D3DUSAGE_RENDERTARGET can only be used with D3DPOOL_DEFAULT. 
			pool = D3DPOOL_DEFAULT;
		uint usage = _usage ; //;| D3DUSAGE_AUTOGENMIPMAP;
		printf( "%s %s\n", __FUNCTION__, srcFile );
        D3DXIMAGE_INFO srcInfo;

		uint levels = 1;
        hr = D3DXCreateVolumeTextureFromFileEx(
            device, //LPDIRECT3DDEVICE9 pDevice,
            srcFile, // LPCTSTR pSrcFile,
            0, //UINT Width,
            0, //UINT Height,
            0, //UINT Depth,
            0, //UINT MipLevels,
            usage, //DWORD Usage,
            D3DFMT_UNKNOWN, //D3DFORMAT Format,
            (D3DPOOL) pool, //D3DPOOL Pool,
            D3DX_DEFAULT , //DWORD Filter,
            D3DX_DEFAULT , //DWORD MipFilter,
            0, //D3DCOLOR ColorKey,
            &srcInfo, //D3DXIMAGE_INFO *pSrcInfo,
            0, //PALETTEENTRY *pPalette,
            & pTexture //LPDIRECT3DVOLUMETEXTURE9 *ppTexture
           );

        E_TRACE(hr);
        if ( FAILED( hr ) )
		{
			return 0;
		}
        return pTexture;
	}



	IDirect3DBaseTexture9 * TextureCache::addTex( Core::Texture * tex )
	{
		if ( ! tex )
		{
			return 0;
		}
		uint usage = 0;
		//if ( strstr( tex->getName(), ".rt" ) )
		//{
		//	usage = D3DUSAGE_RENDERTARGET;
		//}
		IDirect3DBaseTexture9 * tx = 0;

		if ( tex->type() == e6::TU_RENDERTARGET )
		{
			tx = _addTex( tex->format(), tex->width(), tex->height(), 4, 0, D3DUSAGE_RENDERTARGET );
		}
		else
		if ( tex->type() == e6::TU_VIDEO )
		{
			tx = _addTex( tex->format(), tex->width(), tex->height(), 4, 0, D3DUSAGE_DYNAMIC );
			//tx = _addVideoTex( tex->getPath(), usage );
		}
		else
		if ( strstr( tex->getName(), ".cube" ) )
		{
			tx = _addCubeTex( tex->getPath(), usage );
		}
		else
		if ( strstr( tex->getName(), ".3d" ) )
		{
			tx = _addVolumeTex( tex->getPath(), usage );
		}
		else 
		{
			Core::Buffer * cbuf = tex->getLevel(0);
			if ( cbuf )
			{
				unsigned char * pixel = 0;
				cbuf->lock( (void**)&pixel );
				tx = _addTex( tex->format(), tex->width(), tex->height(), 4, pixel, usage );
				cbuf->unlock();
				E_RELEASE( cbuf );
			}
		}

		if ( ! tx ) 
		{
			printf( __FUNCTION__ " ERROR : %s could not create tex\n", (tex?tex->getName():"nil"));
			return 0;
		}
		insert( tex, tx );
		return tx;
	}

	//IDirect3DBaseTexture9 * _addVideoTex( const char * srcFile, uint _usage  )
	//{
	//	IDirect3DBaseTexture9 * tx = 0;
	//	if ( ! tx ) 
	//	{
	//		printf( __FUNCTION__ " ERROR : %s could not create tex\n", srcFile );
	//		return 0;
	//	}
	//	return tx;
	//}



	IDirect3DBaseTexture9 * TextureCache::getBuffer( Core::Texture * tex )
	{
		//~ printf( "%s %s\n", __FUNCTION__, (tex?tex->getName():"nil"));
		//E_ASSERT( cache.size() );

		Item * it = find( tex );
		if ( ! it || ! it->val )
		{
			return addTex(tex);
		}
		it->ttl = 500;
		return it->val;
	}
	
}; // dx9

