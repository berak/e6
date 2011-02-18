
#include "../../e6/e6_impl.h"
#include "../../e6/e6_math.h"
#include "../../Core/Core.h"
#include "../BaseRenderer.h"

#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>
#include <dxerr.h>


using e6::uint;
using e6::float2;
using e6::float3;
using e6::float4;
using e6::float4x4;
using e6::ClassInfo;

#define COM_ADDREF(x)  if((x)) (x)->AddRef();
#define COM_RELEASE(x) if((x)) (x)->Release(); (x)=0;
#define E_TRACE(hr)    if(hr!=0) printf( __FUNCTION__ " : DXERR %s(%x)\n", DXGetErrorString((hr)), (hr) );


namespace Dx10
{

	struct Shader
	{
		ID3D10VertexShader *shader;

		Shader()
			: shader(0)
		{

		}

		~Shader()
		{

		}

		uint setup( Core::Shader* cvb, ID3D10Device* device )
		{
			static const char * def_vs = 
			"float4 VS( in float3 Pos : SV_POSITION ) : SV_POSITION"
			"{"
			"    return float4(Pos,1);"
			"}\r\n\r\n";

			ID3D10Blob *err = 0;
			ID3D10Blob *blob = 0;
			HRESULT hr = D3D10CompileShader(
							def_vs, //  LPCSTR pSrcData,
							strlen( def_vs ), //  SIZE_T SrcDataLen,
							"default", //  LPCSTR pFileName,
							0, //  CONST D3D10_SHADER_MACRO *pDefines,
							0, //  LPD3D10INCLUDE *pInclude,
							"VS", //  LPCSTR pFunctionName,
							"vs_4_0", // LPCSTR pProfile,
							0, // UINT Flags,
							&blob, // ID3D10Blob **ppShader,
							&err // ID3D10Blob **ppErrorMsgs
							);
			E_TRACE(hr);

			if ( FAILED(hr) )
			{
				printf( __FUNCTION__ " : %s\n", (const char *) err->GetBufferPointer() );
				COM_RELEASE( err );
				COM_RELEASE( blob );
				return 0;
			}
		
			hr = device->CreateVertexShader( blob->GetBufferPointer(), blob->GetBufferSize(), &shader );
			E_TRACE(hr);
			COM_RELEASE( blob );

			return SUCCEEDED(hr);
		}


		static void clear( Shader * b )
		{
			COM_RELEASE(b->shader);
			delete b;
		}
	};

	struct ShaderCache 
		: BaseRenderer::Cache< Core::Shader*, Shader*, 500, Shader >
	{
		ShaderCache() 
		{}

		~ShaderCache() 
		{}		


		Shader * addShader( Core::Shader* cvb, ID3D10Device* device )
		{
			Shader * vb = new Shader;
			vb->setup( cvb, device );
			insert( cvb, vb );
			return vb;
		}

		Shader * get( Core::Shader* cvb, ID3D10Device* device )
		{
			//printf( "%s %s\n", __FUNCTION__, (m?m->getName():"nil"));
			Item * it = find( cvb );
			if ( ! it || ! it->val )
			{
				return addShader(cvb, device);
			}
			it->ttl = 500;
			return it->val;
		}
	};

	struct VBuffer
	{
		ID3D10Buffer		* vb;

		uint stride;
		uint nLayouts;
		D3D10_INPUT_ELEMENT_DESC layout[8];

		VBuffer() : vb(0), stride(0), nLayouts(0) {}

		void setSemantic( char * name, DXGI_FORMAT fmt, uint vSize, uint semanticIndex=0 )
		{
			D3D10_INPUT_ELEMENT_DESC & lo = layout[nLayouts];
			lo.SemanticName = name;
			lo.SemanticIndex = semanticIndex;
			lo.Format = fmt;
			lo.InputSlot = nLayouts;
			lo.AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT; // stride;
			lo.InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
			lo.InstanceDataStepRate = 0;
			nLayouts++;
			stride += vSize;
		}
		
		uint setup( Core::VertexBuffer * cvb, ID3D10Device* device )
		{
			HRESULT hr = 0;

			Core::VertexFeature * pos = cvb->getFeature( e6::VF_POS );
			Core::VertexFeature * nor = cvb->getFeature( e6::VF_NOR );
			Core::VertexFeature * col = cvb->getFeature( e6::VF_COL );
			Core::VertexFeature * uv0 = cvb->getFeature( e6::VF_UV0 );
			Core::VertexFeature * uv1 = cvb->getFeature( e6::VF_UV1 );
			Core::VertexFeature * tan = cvb->getFeature( e6::VF_TAN );

			// set stride & collect layout
			if ( pos ) 
			{
				setSemantic( "POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 3 );
			}
			if ( nor ) 
			{
				setSemantic( "NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 3 );
			}
			if ( col )
			{
				setSemantic( "COLOR", DXGI_FORMAT_R32_UINT, 1 ); ///PPP???
			}
			if ( uv0 )
			{
				setSemantic( "TEXCOORD0", DXGI_FORMAT_R32G32_FLOAT, 2 );
			}
			if ( uv1 )
			{
				setSemantic( "TEXCOORD1", DXGI_FORMAT_R32G32_FLOAT, 2, 1 );
			}
			if ( tan )
			{
				setSemantic( "TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3 );
			}

			// copy vb to flat float arr:
			uint nVerts = 3 * cvb->numFaces();
			uint numFloats = nVerts * stride;

			float *pVertData = new float[ numFloats ];
			if(!pVertData)
				return E_OUTOFMEMORY;
			float *pV = pVertData;
			float *v  = 0;

			for ( uint i=0; i<nVerts; i++ )
			{
				{
					v = (float*)pos->getElement( i );
					*pV++ = *v++;
					*pV++ = *v++;
					*pV++ = *v++;
				}
				if ( nor )
				{
					v = (float*)nor->getElement( i );
					*pV++ = *v++;
					*pV++ = *v++;
					*pV++ = *v++;
				}
				if ( col )
				{
					*(uint*)pV = *(uint*)uv0->getElement( i );
					pV ++;
				}
				if ( uv0 )
				{
					v = (float*)uv0->getElement( i );
					*pV++ = *v++;
					*pV++ = *v++;
				}
				if ( uv1 )
				{
					v = (float*)uv1->getElement( i );
					*pV++ = *v++;
					*pV++ = *v++;
				}
				if ( tan )
				{
					v = (float*)tan->getElement( i );
					*pV++ = *v++;
					*pV++ = *v++;
					*pV++ = *v++;
				}
			}

			// Create a Vertex Buffer
			D3D10_BUFFER_DESC vbDesc;
			vbDesc.ByteWidth = numFloats*sizeof(float);
			vbDesc.Usage = D3D10_USAGE_DEFAULT;
			vbDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
			vbDesc.CPUAccessFlags = 0;
			vbDesc.MiscFlags = 0;

			D3D10_SUBRESOURCE_DATA vbInitData;
			vbInitData.pSysMem = pVertData;
			vbInitData.SysMemPitch = 0;
			vbInitData.SysMemSlicePitch = 0;

			hr = device->CreateBuffer( &vbDesc, &vbInitData, &vb );
			E_TRACE( hr );

			E_RELEASE( pos );
			E_RELEASE( nor );
			E_RELEASE( col );
			E_RELEASE( uv0 );
			E_RELEASE( uv1 );
			E_RELEASE( tan );
			E_DELETEA( pVertData );

			//// Create our vertex input layout
			//const D3D10_INPUT_ELEMENT_DESC layout[] =
			//{
			//	{ "POSITION",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
			//};

			//device->CreateInputLayout( layout, 1, pVSBuf->GetBufferPointer(), pVSBuf->GetBufferSize(), &g_pVertexLayout );

			return SUCCEEDED(hr);
		}

		static void clear( VBuffer * b )
		{
			COM_RELEASE(b->vb);
			delete b;
		}
	};

	 
	struct VertexCache 
		: BaseRenderer::Cache< Core::VertexBuffer*, VBuffer*, 500, VBuffer >
	{
		VertexCache() 
		{}

		~VertexCache() 
		{}		


		VBuffer * addBuffer( Core::VertexBuffer* cvb, ID3D10Device* device )
		{
			VBuffer * vb = new VBuffer;
			vb->setup( cvb, device );
			insert( cvb, vb );
			return vb;
		}
		VBuffer * get( Core::VertexBuffer* cvb, ID3D10Device* device )
		{
			//printf( "%s %s\n", __FUNCTION__, (m?m->getName():"nil"));
			Item * it = find( cvb );
			if ( ! it || ! it->val )
			{
				return addBuffer(cvb, device);
			}
			it->ttl = 500;
			return it->val;
		}
	};

	 
	struct CRenderer
		: public e6::CName< Core::Renderer, CRenderer >
	{
		ID3D10Device*           device;
		IDXGISwapChain*         swapChain;
		ID3D10RenderTargetView* renderTargetView;
		ID3D10DepthStencilView* depthStencilView;
		ID3D10Texture2D*		backBuffer;

		VertexCache             vertexCache;
		ShaderCache             shaderCache;
		HRESULT hr;

		CRenderer()
			: device(0)
			, swapChain(0)
			, renderTargetView(0)
			, depthStencilView(0)
			, backBuffer(0)
			, hr(0)
		{
			setName( "RendererDx10" );
		}


		virtual ~CRenderer()
		{
			COM_RELEASE( backBuffer ); 
			COM_RELEASE( renderTargetView ); 
			COM_RELEASE( depthStencilView ); 
			COM_RELEASE( swapChain ); 
			COM_RELEASE( device ); 
		}

		virtual uint init( const void *win, uint w, uint h )
		{ 
			if ( ! device )
			{
				vertexCache.clear();
				shaderCache.clear();

				DXGI_SWAP_CHAIN_DESC sd;
				ZeroMemory( &sd, sizeof(sd) );
				sd.BufferCount = 1;
				sd.BufferDesc.Width = w;
				sd.BufferDesc.Height = h;
				sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				sd.BufferDesc.RefreshRate.Numerator = 60;
				sd.BufferDesc.RefreshRate.Denominator = 1;
				sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				sd.OutputWindow = (HWND)win;
				sd.SampleDesc.Count = 1;
				sd.SampleDesc.Quality = 0;
				sd.Windowed = TRUE;

				UINT createDeviceFlags = 0;
			#ifdef _DEBUG
				createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
			#endif

				D3D10_DRIVER_TYPE driverTypes[] = 
				{
					D3D10_DRIVER_TYPE_HARDWARE,
					D3D10_DRIVER_TYPE_REFERENCE,
				};
				uint numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);
				for( uint driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
				{
					hr = D3D10CreateDeviceAndSwapChain( NULL, driverTypes[driverTypeIndex], NULL, createDeviceFlags, 
														D3D10_SDK_VERSION, &sd, &swapChain, &device );
					if( SUCCEEDED( hr ) )
					{
						char *ds[] = { "Hardware", "Software" };
						printf( __FUNCTION__ " : created %s device.\n", ds[driverTypeIndex] );
						break;
					}
				}
				if ( ! device ) 
				{
					printf( __FUNCTION__ " : no device created !\n" );
					return 0;
				}

				// Create a render target view
				hr = swapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), (LPVOID*)&backBuffer );
				E_TRACE(hr);
				if( FAILED(hr) || (!backBuffer) )
				{
					printf( __FUNCTION__ " : no BackBuffer !\n" );
					return 0;
				}

				//hr = device->CreateDepthStencilView( backBuffer, NULL, NULL );
				//if( hr != S_FALSE )
				//{
				//	printf( __FUNCTION__ " : wrong backbuffer params for DepthStencilView !\n" );
				//	return 0;
				//}
				//hr = device->CreateDepthStencilView( backBuffer, NULL, &depthStencilView );
				//E_TRACE(hr);
				//if( FAILED(hr) )
				//{
				//	printf( __FUNCTION__ " : no depth buffer created !\n" );
				//	return 0;
				//}
				//printf( __FUNCTION__ " : depth buffer %x.\n", depthStencilView);

				//D3D10_TEXTURE2D_DESC tdesc;
				//backBuffer->GetDesc( &tdesc );
				//D3D10_RENDER_TARGET_VIEW_DESC rtvdesc;
				//ZeroMemory(&rtvdesc, sizeof(D3D10_RENDER_TARGET_VIEW_DESC));
				//rtvdesc.Format = tdesc.Format;
				//rtvdesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;

				//hr = device->CreateRenderTargetView( backBuffer, &rtvdesc, NULL );
				//hr = device->CreateRenderTargetView( backBuffer, NULL, NULL );
				//E_TRACE(hr);
				//if( hr != S_FALSE )
				//{
				//	printf( __FUNCTION__ " : wrong backbuffer params for render target !\n" );
				//	return 0;
				//}

				hr = device->CreateRenderTargetView( backBuffer, NULL, &renderTargetView );
				E_TRACE(hr);
				if( FAILED(hr) )
				{
					printf( __FUNCTION__ " : DXERR(%x) %s[%s]\n", (hr) , DXGetErrorString((hr)), DXGetErrorDescription((hr)) );
				printf( __FUNCTION__ " : no render target created !\n" );
					return 0;
				}
				printf( __FUNCTION__ " : render target %x.\n", renderTargetView );

				device->OMSetRenderTargets( 1, &renderTargetView, NULL );


				//return 1;
			}

			return reset( win,w,h );
		}

		uint reset( const void * win, uint w, uint h )
		{
			// Setup the viewport
			D3D10_VIEWPORT vp;
			vp.Width    = w;
			vp.Height   = h;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			device->RSSetViewports( 1, &vp );

			return 1;
		}

		virtual void clear()
		{ 
			float ClearColor[4] = { 0.3f, 0.125f, 0.1f, 1.0f }; // red, green, blue, alpha
			device->ClearRenderTargetView( renderTargetView, ClearColor );
			
			////
			//// Clear the depth stencil
			////
			//ID3D10DepthStencilView* pDSV = DXUTGetD3D10DepthStencilView();
			//device->ClearDepthStencilView( pDSV, D3D10_CLEAR_DEPTH, 1.0, 0 );
		}

		virtual void swapBuffers()
		{ 
		    hr = swapChain->Present( 0, 0 );
			E_TRACE(hr);
		}

		virtual uint begin3D()
		{ 
			return 1; //NOT_IMPL
		}
		virtual uint setCamera( Core::Camera * cam )
		{ 
			return 0; //NOT_IMPL
		}
		virtual uint setLight( uint n, Core::Light * light )
		{ 
			return 0; //NOT_IMPL
		}
		virtual uint setMesh( Core::Mesh * mesh )
		{ 
			if ( ! mesh )
				return 0;
			Core::VertexBuffer * cvb = mesh->getBuffer();
			VBuffer * dxvb = vertexCache.get( cvb, device );

			// Set IA parameters
			UINT strides[1] = { dxvb->stride * sizeof(float) };
			UINT offsets[1] = {0};
			ID3D10Buffer* pBuffers[1] = { dxvb->vb };
			device->IASetVertexBuffers( 0, 1, pBuffers, strides, offsets );

			//device->IASetInputLayout( g_pVertexLayout );
			//device->IASetIndexBuffer( g_pIB10, DXGI_FORMAT_R16_UINT, 0 );
			device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST ); 

			// Bind the constant buffer to the device
			// pBuffers[0] = g_pConstantBuffer10;
			// hr = device->VSSetConstantBuffers( 0, 1, pBuffers ); E_TRACE(hr);
			// hr = device->OMSetBlendState(g_pBlendStateNoBlend, 0, 0xffffffff); E_TRACE(hr);
			// hr = device->RSSetState(g_pRasterizerStateNoCull); E_TRACE(hr);

			Core::Shader * cvs = mesh->getVertexShader();
			device->VSSetShader( shaderCache.get( cvs, device )->shader ); 
			E_RELEASE( cvs );

			// device->GSSetShader( NULL ); 
			// device->PSSetShader( NULL );
			// hr = device->DrawIndexed( g_dwNumIndices, 0, 0 ); E_TRACE(hr);


			uint nVerts = mesh->numFaces() * 3;
			device->Draw( nVerts, 0 );		
			E_RELEASE(cvb);
			return 1;
		}
		virtual uint setRenderTarget( Core::Texture * tex )
		{ 
			return 0; //NOT_IMPL
		}
		virtual uint setRenderState( uint key, uint value )
		{ 
			return 0; //NOT_IMPL
		}
		virtual uint setVertexShaderConstant( uint i, const float4 & v )
		{ 
			return 0; //NOT_IMPL
		}
		virtual uint setPixelShaderConstant( uint i, const float4 & v )
		{ 
			return 0; //NOT_IMPL
		}
		virtual uint end3D()
		{ 
			return 1; //NOT_IMPL
		}

		virtual uint get( uint what )
		{ 
			switch( what )
			{
				//case e6::RF_CULL: return doCull; 
				//case e6::RF_CCW: return doCCW; 
				//case e6::RF_SHADE:  return shadeMode; 
				//case e6::RF_WIRE: return doWire; 
				//case e6::RF_TEXTURE: return doTexture; 
				//case e6::RF_LIGHTING: return doLighting; 
				//case e6::RF_ALPHA: return doAlpha; 
				//case e6::RF_ALPHA_TEST: return doAlphaTest; 
				//case e6::RF_ZWRITE: return doZWrite; 
				//case e6::RF_ZTEST: return doZTest; 
				//case e6::RF_ZDEPTH: return zBufferDepth; 
				//case e6::RF_SRCBLEND: return stateBlendSrc; 
				//case e6::RF_DSTBLEND: return stateBlendDst; 
				//case e6::RF_CLEAR_COLOR: return bgColor; 
				//case e6::RF_CLEAR_BUFFER: return doClearBackBuffer;
				//case e6::RF_NUM_HW_TEX: return textureCache.cache.size(); 
				case e6::RF_NUM_HW_VB : return vertexCache.cache.size(); 
				case e6::RF_NUM_HW_VSH: return shaderCache.cache.size(); 
				//case e6::RF_NUM_HW_PSH: return pixelShaderCache.cache.size(); 
			}
			return 0; 
		}

		virtual uint captureFrontBuffer( const char * fileName )
		{ 
			return 0; //NOT_IMPL
		}

	}; // CRenderer

}; // namespace Dx10

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Renderer",	 "RendererDx10",	Dx10::CRenderer::createInterface, Dx10::CRenderer::classRef	},
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
	mv->modVersion = ("RendererDx10 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
