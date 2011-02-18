#include "Dx9.h"
#include <vector>

#include "../../Core/Frustum.h"
namespace Dx9
{
	using Core::Mesh;
	using Core::Texture;
	using Core::Camera;
	using Core::Light;
	using Core::Shader;
	using Core::Mesh;
	
	void getFVF( uint mesh_format, uint & fvf, uint & siz )
	{
		fvf = D3DFVF_XYZ;
		siz = 3*sizeof(float);
		int stage = 0;
		if ( mesh_format & e6::VF_NOR )
		{
			fvf |= D3DFVF_NORMAL;
			siz += 3*sizeof(float);
		}
		if ( mesh_format & e6::VF_COL )
		{
			fvf |= D3DFVF_DIFFUSE;
			siz += 1*sizeof(uint);
		}
		if ( mesh_format & e6::VF_UV0 )
		{
			fvf |= (stage+1) << D3DFVF_TEXCOUNT_SHIFT;
			fvf |= D3DFVF_TEXCOORDSIZE2(stage);
			siz += 2*sizeof(float);
			stage ++;
		}
		if ( mesh_format & e6::VF_UV1 )
		{
			fvf |= (stage+1) << D3DFVF_TEXCOUNT_SHIFT;
			fvf |= D3DFVF_TEXCOORDSIZE2(stage);
			siz += 2*sizeof(float);
			stage ++;
		}
		if ( mesh_format & e6::VF_TAN )
		{
			fvf |= (stage+1) << D3DFVF_TEXCOUNT_SHIFT;
			fvf |= D3DFVF_TEXCOORDSIZE3(stage);
			siz += 3*sizeof(float);
			stage ++;
		}
	}

	struct CRenderer
		: e6::CName< Core::Renderer, CRenderer >
	{
		D3DPRESENT_PARAMETERS   d3dpp;
		D3DDISPLAYMODE          d3ddm;
		D3DCAPS9                caps;
		LPDIRECT3D9             d3d; 
		LPDIRECT3DDEVICE9       device; 
		LPDIRECT3DSURFACE9      backBuffer;
		IDirect3DBaseTexture9 * dxt_last;
		IDirect3DVertexBuffer9 * vb_last;
		IDirect3DVertexShader9 * vs_last;
		IDirect3DPixelShader9 * ps_last;
		uint fvf_last;
		HRESULT 				hr;

		VertexCache				vertexCache;
		TextureCache			textureCache;
		PixelShaderCache		pixelShaderCache;
		VertexShaderCache		vertexShaderCache;

		std::vector< Core::Mesh* > alphaMeshes;
		
		float4x4 matWorld;
		float4x4 matView;
		float4x4 matProj;
		float4x4 matClip;
		
		bool doCull;
		bool doCCW;
		bool doWire;
		bool doTexture;
		bool doLighting;
		bool doFullScreen;
		bool doClearBackBuffer;
		bool doZTest;
		bool doZWrite;
		bool doAlpha;
		bool doAlphaTest;
		bool deviceLost;

		uint zBufferDepth ;
		uint stateBlendSrc;
		uint stateBlendDst;
		uint shadeMode;
		uint multiSampleType;
		uint multiSampleQuality;
		rgba bgColor;
		uint ttl;

		Core::Frustum frustum;

		CRenderer() 
			: d3d(0)
			, device(0)
			, backBuffer(0)
			, doCull(1)
			, doCCW(0)
			, doWire(0)
			, doTexture(1)
			, bgColor(0xff333377)
			, doLighting(1)
			, doFullScreen(0)
			, doClearBackBuffer(1)
			, zBufferDepth(16)
			, doZWrite(1)
			, doZTest(1)
			, doAlpha(0)
			, doAlphaTest(0)
			, stateBlendSrc( e6::BM_ONE )
			, stateBlendDst( e6::BM_ZERO )
			, multiSampleType(D3DMULTISAMPLE_NONE)
			, multiSampleQuality(1)
			, shadeMode( 1 )
			, vb_last(0)
			, dxt_last(0)
			, vs_last(0)
			, ps_last(0)
			, fvf_last(0)
			, deviceLost(0)
		{

			ZeroMemory( &d3dpp, sizeof(d3dpp) );
			ZeroMemory( &d3ddm, sizeof( D3DDISPLAYMODE ) );
			ZeroMemory( &caps, sizeof( D3DCAPS9 ) );
			matProj.perspective( 60, 1.0f, 1, 100 );
			setName( "RendererDx9");
		}

		~CRenderer() 
		{
			cleanup();
		}
		virtual void *cast( const char * str )
		{
			return _cast(str,"Core.Renderer", "e6.Name");
		}
			

		uint createDevice( const void *win, uint w, uint h, D3DDEVTYPE deviceType )
		{
			hr = d3d->GetDeviceCaps( D3DADAPTER_DEFAULT, deviceType, & caps );
			if( FAILED( hr ) )
			{
				E_TRACE(hr);
				printf( "%s()    Error: GetDeviceCaps\n", __FUNCTION__ );
				return 0;
			}

			// antialias
			uint maxMultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
			uint maxMultiSampleQuality = 1;

			hr = d3d->CheckDeviceMultiSampleType( 
				D3DADAPTER_DEFAULT,
				deviceType,
				d3ddm.Format,
				doFullScreen,
				(D3DMULTISAMPLE_TYPE) maxMultiSampleType,
				(DWORD *) &maxMultiSampleQuality
				);
			if ( FAILED(hr))
			{
				E_TRACE(hr);
				multiSampleType  = D3DMULTISAMPLE_NONE;
				multiSampleQuality = 1;
				return 0;
			}
			else
			{
				if (multiSampleQuality < maxMultiSampleQuality-1 )
				{
					multiSampleQuality = maxMultiSampleQuality-1 ;
					multiSampleType    = maxMultiSampleType;
				}
				printf( "%s : setting multisampling quality to %d / %d !\n", __FUNCTION__,  multiSampleQuality, maxMultiSampleQuality-1 );
			}

			init_pp(win,w,h);

			#ifdef _DEBUG
			uint behaviour = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
			#else
			uint behaviour = D3DCREATE_HARDWARE_VERTEXPROCESSING;
			#endif

			if( FAILED( hr=d3d->CreateDevice( D3DADAPTER_DEFAULT, deviceType,
											(HWND)win, behaviour,
											&d3dpp, &device ) ) )
			{
				LOG_ERROR(hr);
				return 0;
			}
			return 1;
		}

		virtual uint init( const void *win, uint w, uint h )
		{
			if ( ! device )
			{
				if( NULL == ( d3d = Direct3DCreate9( D3D_SDK_VERSION ) ) )
				{
					printf( __FUNCTION__ " ERROR :  no D3D!\n" );
					return 0;
				}

				hr = d3d->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ); E_TRACE(hr);
				if( FAILED( hr ) )
				{
					E_TRACE(hr);
					printf( "%s()    Error: GetAdapterDisplayMode\n", __FUNCTION__ );
					return 0;
				}

				if ( ! createDevice( win, w,h, D3DDEVTYPE_HAL ) )
				{
					printf( "trying software renderer..\n");
					if ( ! createDevice( win, w,h, D3DDEVTYPE_REF ) )
					{
						COM_RELEASE( d3d );
						return 0;
					}
				}

				restore();

				vertexCache.setDevice( device );
				textureCache.setDevice( device );
				pixelShaderCache.setDevice( device );
				vertexShaderCache.setDevice( device );
				return 1;
			} 

			return reset(win,w,h);
		}

		void setupZBuffer()
		{
			switch (zBufferDepth)
			{
			case 0: // switch off zbuffer:
				d3dpp.EnableAutoDepthStencil = FALSE;
				doZTest = FALSE;
				doZWrite =  FALSE;
				break;
			case 16:
				d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
				d3dpp.EnableAutoDepthStencil = TRUE;
				d3dpp.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;            
				doZTest = TRUE;
				break;

			default:
				MessageBox( 0, "invalid zbuffer-depth in rdd. defaulting to 24 bit !", "sorry...", 0 );
			case 24:
				d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;//D3DFMT_D16;
				d3dpp.EnableAutoDepthStencil = TRUE;
				d3dpp.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;            
				doZTest = TRUE;
			}
			//~ $();
		}
		uint init_pp(const void * win, int w, int h)
		{
			d3dpp.Windowed			= (! doFullScreen);
			d3dpp.SwapEffect		= D3DSWAPEFFECT_DISCARD;
			d3dpp.BackBufferWidth	= doFullScreen ? d3ddm.Width : w;
			d3dpp.BackBufferHeight	= doFullScreen ? d3ddm.Height : h;
			d3dpp.BackBufferFormat	= d3ddm.Format;
			d3dpp.BackBufferCount	= 1;
			d3dpp.hDeviceWindow		= (HWND) win;
			d3dpp.MultiSampleType	= (D3DMULTISAMPLE_TYPE) multiSampleType;
			d3dpp.MultiSampleQuality = multiSampleQuality;
			//d3dpp.FullScreen_RefreshRateInHz = 0;
			//d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
			//d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
			//d3dpp.Flags |= D3DPRESENTFLAG_DEVICECLIP;
			//d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
			//d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
			//d3dpp.PresentationInterval = D3DPRESENT_DONOTWAIT;
			//d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

			setupZBuffer();
			//~ $();
			return 1;
		}

		void restore()
		{
			// restore render states
			//~ hr = device->SetRenderState( D3DRS_CULLMODE, (doCull? doCCW + 2 : 1)  ); E_TRACE(hr);
			//~ hr = device->SetRenderState( D3DRS_LIGHTING, doLighting  );			E_TRACE(hr);
			hr = device->SetRenderState( D3DRS_ZENABLE, doZTest );				E_TRACE(hr);
			hr = device->SetRenderState( D3DRS_ZWRITEENABLE, doZWrite );		E_TRACE(hr);

			hr = device->SetRenderState( D3DRS_ALPHATESTENABLE, doAlphaTest );	E_TRACE(hr);

			_setRenderState( e6::RF_LIGHTING, doLighting );
			_setRenderState( e6::RF_CULL, doCull );
			_setRenderState( e6::RF_CCW, doCCW );
			_setRenderState( e6::RF_SRCBLEND, stateBlendSrc );
			_setRenderState( e6::RF_SRCBLEND, stateBlendSrc );
			_setRenderState( e6::RF_DSTBLEND, stateBlendDst );
			_setRenderState( e6::RF_SHADE, shadeMode );
			_setRenderState( e6::RF_WIRE, doWire );
			_setRenderState( e6::RF_TEXTURE, doTexture );
			_setRenderState( e6::RF_ALPHA, doAlpha );

			for ( uint i=0; i<8; i++ )
			{
				hr = device->SetSamplerState( i, D3DSAMP_ADDRESSU,  D3DTADDRESS_WRAP );
				hr = device->SetSamplerState( i, D3DSAMP_ADDRESSV,  D3DTADDRESS_WRAP );
				hr = device->SetSamplerState( i, D3DSAMP_MAGFILTER,   D3DTEXF_ANISOTROPIC ); 
				hr = device->SetSamplerState( i, D3DSAMP_MINFILTER,   D3DTEXF_ANISOTROPIC );    
				hr = device->SetSamplerState( i, D3DSAMP_MIPFILTER,   D3DTEXF_ANISOTROPIC );  
			}

			float lc[] = { 1,1,1,1 };
			hr = device->SetVertexShaderConstantF( e6::VS_LIGHT_0_COLOR, lc, 1 );
			float lp[] = { -1,100,10,1 };
			hr = device->SetVertexShaderConstantF( e6::VS_LIGHT_0_POS, lp, 1 );
			//float vp[] = { d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, 0, 0 };
			//hr = device->SetPixelShaderConstantF( e6::PS_RESOLUTION, vp, 1 );

			if ( ! backBuffer )
			{
				hr = device->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer );
				E_TRACE(hr);
				if ( hr ) backBuffer = 0;
			}

			dxt_last = 0;
			vb_last = 0;
			vs_last = 0;
			ps_last = 0;
			fvf_last = 0;
		}		

		uint reset(const void *win, int w, int h)
		{
			COM_RELEASE( backBuffer );

			init_pp(win,w,h);
			hr = device->Reset( &d3dpp );		E_TRACE(hr);
			if ( FAILED( hr ) )
			{
				return 0;
			}
			restore();
			return 1;
		}


		virtual void clear()
		{
			uint clearFlag = 0;
			if ( doZTest )
				clearFlag |= D3DCLEAR_ZBUFFER;
			if ( doClearBackBuffer )
				clearFlag |= D3DCLEAR_TARGET;

			if ( clearFlag )
			{
				hr = device->Clear( 0, NULL, clearFlag, bgColor, 1.0f, 0 );
				E_TRACE(hr);
			}
			//~ $();
		}

		virtual void swapBuffers()
		{
			hr = device->Present( NULL, NULL, NULL, NULL );
			E_TRACE(hr);
			if ( hr == D3DERR_DEVICELOST )
			{
				deviceLost = true;
				printf( __FUNCTION__ " : DEVICE LOST !\n" );
				//vertexCache.clear();
				//textureCache.clear();
				//pixelShaderCache.clear();
				//vertexShaderCache.clear();
				// backBuffer ??
				// rendertargets ??
			}
			else
			{
				vertexCache.tick();
				textureCache.tick();
				pixelShaderCache.tick();
				vertexShaderCache.tick();
			}
			//~ $();
		}

		virtual uint begin3D()
		{
			alphaMeshes.clear();

			hr = device->BeginScene();
			E_TRACE(hr);
			//~ $();
			return SUCCEEDED( hr );
		}

		virtual uint setRenderTarget( Texture * tex ) 
		{
			HRESULT hr = S_FALSE;
			IDirect3DSurface9 * target = backBuffer;

			if ( tex && (tex->type() == e6::TU_RENDERTARGET) )
			{
				IDirect3DTexture9 * txdx = (IDirect3DTexture9*)textureCache.getBuffer( tex );
				if ( txdx )
				{
					hr = txdx->GetSurfaceLevel( 0, & target );
					E_TRACE(hr);
					COM_RELEASE( txdx );
					if ( hr != S_OK ) 
					{
						target = backBuffer;
						//return 0;
					}
				}
			}

			hr = device->SetRenderTarget( 0, target );
			if ( (hr==S_OK) && (target != backBuffer) ) // otherwise wait for regular clear
			{
				clear();
			}

			E_TRACE(hr);
			return hr==S_OK; 
		}
		
		virtual uint setCamera( Camera * cam )
		{
			D3DXMatrixPerspectiveFovRH( (D3DXMATRIX*)&matProj, cam->getFov() * e6::deg2rad, 1.0f, cam->getNearPlane(), cam->getFarPlane() );

			float4x4 matCam;
			cam->getWorldMatrix(matCam);
			matView = matCam.inverse();

			hr = device->SetVertexShaderConstantF( e6::VS_MATRIX_INV_VIEW, (float*)&(matView), 4 );
			E_TRACE(hr);
	
			matClip = matView * matProj;

			frustum.setup( matClip );

			return 1;
		}

		
		virtual uint setLight( uint n, Light * light ) 
		{
			if ( doLighting )
			{
				if ( n > 4 ) return 0;
				
				float4 c(light->getColor());
				c[3] = 1;
				hr = device->SetVertexShaderConstantF( e6::VS_LIGHT_0_COLOR+2*n, (float*)&c, 1 );
				E_TRACE(hr);
				float4 d(light->getPos());
				d[3] = 1;
				hr = device->SetVertexShaderConstantF( e6::VS_LIGHT_0_POS+2*n, (float*)&d, 1 );
				E_TRACE(hr);
			}
			return 1;
		}

		
		virtual uint setMesh( Mesh * mesh )
		{
			if ( doTexture )
			if ( meshHasAlpha( mesh ) )
			{
				// later, baby..
				alphaMeshes.push_back( mesh );
				return 1;
			}

			return renderMesh( mesh );
		}


		uint meshHasAlpha( Mesh * mesh )
		{
			bool yes = 0;
			for ( uint i=0; i<4; i++ )
			{
				Core::Texture * tex = mesh->getTexture(i);				
				if ( ! tex ) 
				{
					continue;
				}
				yes = ( tex->format() == e6::PF_A8B8G8R8 );
				E_RELEASE( tex );
				if ( yes ) break;
			}
			return yes;
		}

		uint renderMesh( Mesh * mesh )
		{
			// transformation:
			mesh->getWorldMatrix(matWorld);
			float4x4 matWVP = (matWorld*matClip);
			hr = device->SetVertexShaderConstantF( e6::VS_MATRIX_WORLD, (float*)&matWorld, 4 );
			hr = device->SetVertexShaderConstantF( e6::VS_MATRIX_WORLD_VIEW_PROJ, (float*)&matWVP, 4 );
			E_TRACE(hr);

			if ( this->doCull )
			{
				bool in = frustum.intersectSphere( matWorld.getPos(), mesh->getSphere() );
				if ( ! in ) 
				{
					//printf("Culled %s.\n", mesh->getName() );
					return 0;
				}
			}

			// textures:
			if ( doTexture )
			for ( uint i=0; i<4; i++ )
			{
				Core::Texture * tex = mesh->getTexture(i);				
				if ( ! tex ) 
				{
					//if (i!=0)
						continue;
					// else try to set checker:
				}

				IDirect3DBaseTexture9 * dxt = textureCache.getBuffer( tex );

				if ( tex && (tex->type() == e6::TU_DYNAMIC || tex->type() == e6::TU_VIDEO) )
				{
					Core::Buffer * cbuf = tex->getLevel(0);
					if ( cbuf )
					{
						unsigned char * pixel = 0;
						cbuf->lock( (void**)&pixel );
						if ( pixel )
						{
							IDirect3DSurface9 *surf = 0;
							HRESULT hr = ((IDirect3DTexture9 *)dxt)->GetSurfaceLevel( 0, & surf );
			
							SurfaceHelper::copyToSurface( tex->width(), tex->height(), 4, pixel,surf );

							COM_RELEASE( surf );
							cbuf->unlock();
						}else{printf("!lock\n");}
						E_RELEASE( cbuf );
					}
				}

				if ( dxt_last != dxt )
				{
					hr = device->SetTexture( i, dxt );
					E_TRACE(hr);
				}
				dxt_last = dxt;
				E_RELEASE( tex );
			}


			// VertexShader:
			Core::Shader * vShader = mesh->getVertexShader();
			IDirect3DVertexShader9 * vs = vertexShaderCache.getBuffer( vShader );
			if ( vs_last != vs )
			{
				hr = device->SetVertexShader( vs );
				E_TRACE(hr);
			}
			E_RELEASE( vShader );
			vs_last = vs;

			for ( uint i=0; i<mesh->getNumVertexShaderConstants(); i++ )
			{
				Core::Shader::Constant *c = mesh->getVertexShaderConstant(i);
				device->SetVertexShaderConstantF( c->index, (float*)&(c->value), 1 );
			}

			// PixelShader:
			Core::Shader * pShader = mesh->getPixelShader();
			IDirect3DPixelShader9 * ps = pixelShaderCache.getBuffer( pShader );
			if ( ps_last != ps )
			{
				hr = device->SetPixelShader( ps );
				E_TRACE(hr);
			}
			E_RELEASE( pShader );
			ps_last = ps;

			for ( uint i=0; i<mesh->getNumPixelShaderConstants(); i++ )
			{
				Core::Shader::Constant *c = mesh->getPixelShaderConstant(i);
				device->SetPixelShaderConstantF( c->index, (float*)&(c->value), 1 );
			}

			for ( uint i=0; i<mesh->getNumRenderStates(); i++ )
			{
				Core::Shader::RenderState *rs = mesh->getRenderState(i);
				_setRenderState( rs->index, rs->value );
			}

			// VertexBuffer:
			Core::VertexBuffer * vb = mesh->getBuffer();
			E_ASSERT(vb);
			IDirect3DVertexBuffer9 *vbdx = vertexCache.getBuffer( vb );
			E_ASSERT(vbdx);
			E_RELEASE(vb);

			uint vSiz=0, fvf=0;
			getFVF( mesh->format(), fvf, vSiz );


			if ( (vb_last != vbdx) || (fvf_last != fvf) )
			{
				hr = device->SetStreamSource( 0, vbdx, 0, vSiz );
				E_TRACE(hr);
				hr = device->SetFVF( fvf );
				E_TRACE(hr);
			}
			vb_last = vbdx;
			fvf_last  = fvf;
			
			hr = device->DrawPrimitive( D3DPT_TRIANGLELIST, 0, mesh->numFaces() );
			E_TRACE(hr);
			//~ $();
			return 1;
		}




		virtual uint setRenderState( uint key, uint value )
		{
			switch( key )
			{
				case e6::RF_TEXTURE:
					doTexture = (bool)value;
					break;
				case e6::RF_LIGHTING:
					doLighting = (bool)value;
					break;
				case e6::RF_CULL:
					doCull = (bool)value;
					break;
				case e6::RF_CCW:
					doCCW = (bool)value;
					break;
				case e6::RF_SHADE: 
					shadeMode=value; 
					break;
				case e6::RF_CLEAR_COLOR: 
					bgColor=value; 
					break;
				case e6::RF_CLEAR_BUFFER:
					doClearBackBuffer = (bool)value;
					break;
				case e6::RF_ZTEST:
					doZTest = (bool)value;
					break;
				case e6::RF_ZWRITE:
					doZWrite = (bool)value;
					break;
				case e6::RF_ZDEPTH:
					doZTest = (bool)value;
					setupZBuffer();
					break;
				case e6::RF_ALPHA_TEST:
					doAlphaTest = (bool)value;
					break;
				case e6::RF_ALPHA:
					doAlpha = (value!=0);
					break;
				case e6::RF_SRCBLEND:
					stateBlendSrc = value; 
					break;
				case e6::RF_DSTBLEND:
					stateBlendDst = value; 
					break;
				case e6::RF_WIRE:
					doWire = (bool)value;
					break;
				default: return 0;
			}
			//~ $();
			return _setRenderState( key, value );
		}

		uint _blendMode( uint e6_blend )
		{
			uint mode = 0;
			switch( e6_blend ) 
			{
				case e6::BM_ZERO : 
					mode = D3DBLEND_ZERO; break;
				case e6::BM_ONE: 
					mode = D3DBLEND_ONE; break;		// (1, 1, 1, 1 )  
				case e6::BM_SRCCOLOR: 
					mode = D3DBLEND_SRCCOLOR; break;			// (Rs, Gs, Bs, As )  
				case e6::BM_INVSRCCOLOR: 
					mode = D3DBLEND_INVSRCCOLOR; break;	// (1-Rs, 1-Gs, 1-Bs, 1-As)  
				case e6::BM_SRCALPHA: 
					mode = D3DBLEND_SRCALPHA; break;		// (As, As, As, As )  
				case e6::BM_INVSRCALPHA: 
					mode = D3DBLEND_INVSRCALPHA; break;		// (1-As, 1-As, 1-As, 1-As)  
				case e6::BM_DESTALPHA: 
					mode = D3DBLEND_DESTALPHA; break;		// (Ad, Ad, Ad, Ad )  
				case e6::BM_INVDESTALPHA:  
					mode = D3DBLEND_INVDESTALPHA; break;	// (1-Ad, 1-Ad, 1-Ad, 1-Ad)  
				case e6::BM_DESTCOLOR:  
					mode = D3DBLEND_DESTCOLOR; break;	// (Rd, Gd, Bd, Ad )  
				case e6::BM_INVDESTCOLOR: 
					mode = D3DBLEND_INVDESTCOLOR; break;	// (1-Rd, 1-Gd, 1-Bd, 1-Ad)  
				case e6::BM_SRCALPHASAT:  
					mode = D3DBLEND_SRCALPHASAT; break;		// (f, f, f, 1 ) f = min (As, 1-Ad)  
				default: E_ASSERT(!"unknown type!");  break;
			}
			return mode;
		}

		uint _setRenderState( uint key, uint value )
		{
			E_ASSERT( device );
			switch( key )
			{
				case e6::RF_TEXTURE:
					if ( ! value )
					{
						hr = device->SetTexture( 0, 0 );					E_TRACE(hr);
						hr = device->SetTexture( 1, 0 );					E_TRACE(hr);
						hr = device->SetTexture( 2, 0 );					E_TRACE(hr);
						hr = device->SetTexture( 3, 0 );					E_TRACE(hr);
						E_TRACE(hr);
					}
					break;
				case e6::RF_LIGHTING:
					hr = device->SetRenderState( D3DRS_LIGHTING, value  );									E_TRACE(hr);
					break;
				case e6::RF_CULL:
					hr = device->SetRenderState( D3DRS_CULLMODE, (value? doCCW + 2 : 1)  );					E_TRACE(hr);
					break;
				case e6::RF_CCW:
					hr = device->SetRenderState( D3DRS_CULLMODE, (doCull? value + 2 : 1)  );					E_TRACE(hr);
					break;
				case e6::RF_SHADE: 
					hr = device->SetRenderState( D3DRS_SHADEMODE, (value? D3DSHADE_GOURAUD : D3DSHADE_FLAT)  );	E_TRACE(hr);
					break;
				case e6::RF_ZTEST:
					hr = device->SetRenderState( D3DRS_ZENABLE, value );									E_TRACE(hr);
					break;
				case e6::RF_ZWRITE:
					hr = device->SetRenderState( D3DRS_ZWRITEENABLE, value );								E_TRACE(hr);
					break;
				case e6::RF_ZDEPTH:
					break;
				case e6::RF_ALPHA_TEST:
					device->SetRenderState( D3DRS_ALPHAREF, (DWORD) 0x000000001 );
					//   device->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_ALWAYS );
					hr = device->SetRenderState( D3DRS_ALPHATESTENABLE, value );							E_TRACE(hr);
					break;
				case e6::RF_ALPHA:
					hr = device->SetRenderState(D3DRS_ALPHABLENDENABLE, value );							E_TRACE(hr);
					break;
				case e6::RF_SRCBLEND:
					hr = device->SetRenderState( D3DRS_SRCBLEND, _blendMode(value) );						E_TRACE(hr);
					break;
				case e6::RF_DSTBLEND:
					hr = device->SetRenderState( D3DRS_DESTBLEND, _blendMode(value) );						E_TRACE(hr);
					break;				
				case e6::RF_WIRE:
				{
					if ( value )
					{
						hr = device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );					E_TRACE(hr);
//						hr = device->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
					}
					else
					{
						hr = device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );						E_TRACE(hr);
//						hr = device->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
					}

					break;
				}
				default: return 0;
			}

			//~ $();
			return 1;
		}


		virtual uint setVertexShaderConstant( uint i, const float4 & v )
		{
			E_ASSERT( device );
			hr = device->SetVertexShaderConstantF( i, (float*)&(v), 1 );
			E_TRACE(hr);
			return 1;
		}
		virtual uint setPixelShaderConstant( uint i, const float4 & v ) 
		{
			E_ASSERT( device );
			hr = device->SetPixelShaderConstantF( i, (float*)&(v), 1 );
			E_TRACE(hr);
			return 1;
		}


		virtual uint end3D()
		{
			E_ASSERT( device );

			if ( ! alphaMeshes.empty() ) 
			{		
				//~ hr = device->SetRenderState( D3DRS_ALPHATESTENABLE, 1 );			E_TRACE(hr);
				hr = device->SetRenderState( D3DRS_ALPHABLENDENABLE, 1 );				E_TRACE(hr);
				hr = device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );		E_TRACE(hr);
				hr = device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );	E_TRACE(hr);
				for ( uint i=0; i<alphaMeshes.size(); i++ )
				{
					if ( alphaMeshes[ i ] )
					{
						renderMesh( alphaMeshes[i] );
					}
				}
				alphaMeshes.clear();
				//~ hr = device->SetRenderState( D3DRS_ALPHATESTENABLE, doAlphaTest );	E_TRACE(hr);
				_setRenderState( e6::RF_ALPHA, doAlpha );
				_setRenderState( e6::RF_SRCBLEND, stateBlendSrc );
				_setRenderState( e6::RF_DSTBLEND, stateBlendDst );	
			}

			hr = device->EndScene();			E_TRACE(hr);


			//{ // save rendertarget for debugging:
			//	IDirect3DSurface9 * target = 0;
			//	hr = device->GetRenderTarget( 0, & target );
			//	E_TRACE(hr);
			//	if ( target != backBuffer )
			//	{
			//		SurfaceHelper::saveSurface( target, "screen.tga" );
			//	}
			//}

			//~ $();
			return SUCCEEDED( hr );
		}

		
		virtual uint get( uint what )
		{
			switch( what )
			{
				case e6::RF_CULL: return doCull; 
				case e6::RF_CCW: return doCCW; 
				case e6::RF_SHADE:  return shadeMode; 
				case e6::RF_WIRE: return doWire; 
				case e6::RF_TEXTURE: return doTexture; 
				case e6::RF_LIGHTING: return doLighting; 
				case e6::RF_ALPHA: return doAlpha; 
				case e6::RF_ALPHA_TEST: return doAlphaTest; 
				case e6::RF_ZWRITE: return doZWrite; 
				case e6::RF_ZTEST: return doZTest; 
				case e6::RF_ZDEPTH: return zBufferDepth; 
				case e6::RF_SRCBLEND: return stateBlendSrc; 
				case e6::RF_DSTBLEND: return stateBlendDst; 
				case e6::RF_CLEAR_COLOR: return bgColor; 
				case e6::RF_CLEAR_BUFFER: return doClearBackBuffer;
				case e6::RF_NUM_HW_TEX: return textureCache.cache.size(); 
				case e6::RF_NUM_HW_VB : return vertexCache.cache.size(); 
				case e6::RF_NUM_HW_VSH: return vertexShaderCache.cache.size(); 
				case e6::RF_NUM_HW_PSH: return pixelShaderCache.cache.size(); 
			}
			return 0;
		}
		
		void cleanup()
		{
			alphaMeshes.clear();
			COM_RELEASE( backBuffer );
			COM_RELEASE( device ) ;
			COM_RELEASE( d3d  );
		}

		virtual uint captureFrontBuffer( const char * fileName )
		{
			IDirect3DSurface9 * surface = 0;
			hr = device->CreateOffscreenPlainSurface( 
				d3ddm.Width, 
				d3ddm.Height, 
				//d3dpp.BackBufferWidth, 
				//d3dpp.BackBufferHeight, 
				D3DFMT_A8R8G8B8,
				D3DPOOL_SYSTEMMEM,
				& surface, 0 );

			if ( hr != S_OK ) return 0;

			HWND hwnd = this->d3dpp.hDeviceWindow;
			//HWND hwnd = GetActiveWindow();

			RECT * rect = 0;
			RECT rect0;
			if ( ! doFullScreen )
			{
				GetWindowRect( hwnd, & rect0 );
				//GetClientRect( hwnd, & rect1 );
				rect = & rect0;
			}


			uint swapChain = 0;
			hr = device->GetFrontBufferData( swapChain, surface );
			if ( hr != S_OK ) return 0;
			//HRESULT hr = S_OK;
			uint res = SurfaceHelper::saveSurface( surface, fileName, rect );

			COM_RELEASE( surface );

			return res;
		}


	}; //CRenderer

}; // dx9



using e6::ClassInfo;

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Renderer",	 "RendererDx9",	Dx9::CRenderer::createSingleton, Dx9::CRenderer::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}



#include "../../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Dx9 00.000.0000 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}


