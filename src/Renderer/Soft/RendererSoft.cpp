#include "../../e6/e6_impl.h"
#include "../../e6/e6_math.h"
#include "../../Core/Core.h"
#include "../BaseRenderer.h"

#include <time.h>
#include <map>

#include <Windows.h>


using e6::uint;
using e6::rgba;
using e6::ClassInfo;
using e6::float2;
using e6::float3;
using e6::float4;
using e6::float4x4;
using BaseRenderer::CBaseRenderer;
using Core::Camera;
using Core::Light;
using Core::Mesh;



namespace RendererSoft
{
	
	struct Vertex
	{
		float4 pos;
		float3 col;
		float3 nor;
		float2 uv0;
		float2 scr;
	};

	struct CRenderer 
		: BaseRenderer::CBaseRenderer< Core::Renderer , CRenderer >
	{
		typedef BaseRenderer::CBaseRenderer< Core::Renderer , CRenderer > CBaseRendererSoft;

		void * window;
		uint screenW,screenH;
		unsigned char *screen;
		
		CRenderer()
			: screenW(0)
			, screenH(0)
			, window(0)
			, screen(0)
		{
			setName( "RendererSoft");			
			doWire = true;
		}

		~CRenderer()
		{
			E_DELETEA( screen );
		}

		inline	uint numBytes() { return 3 * screenW * screenH; }
		
		virtual uint init( const void *win, uint w, uint h )
		{
			w = (w+3)&(~3);

			if ( screenW != w || screenH != h )
			{
				screenW = w;
				screenH = h;
				E_DELETEA( screen );
				screen = new unsigned char[ numBytes() ];
				assert( screen );
				printf( __FUNCTION__ "  %i %i %i \n", w, h, numBytes() );
			}
			window = (void*)win;
			return 1;
		}

		virtual void clear()
		{
			if ( ! this->doClearBackBuffer ) return ;
			uint n = screenW * screenH;
			unsigned char p[3] = { (bgColor&0xff),((bgColor>>8)&0xff),((bgColor>>16)&0xff) };
			unsigned char * s = screen;			
			//~ printf( __FUNCTION__ "  %i %i %i \n", screenW, screenH, screenW * screenH );
			for ( uint i=0; i<n; i++ )
			{
				*s++ = p[0];
				*s++ = p[1];
				*s++ = p[2];
			}
		}

		virtual void swapBuffers()
		{
			//~ $()
			if ( window )
				blit();
		}
		uint captureFrontBuffer(const char *)
		{
			return 0;
		}
		uint setRenderTarget(Core::Texture *)
		{
			return 0;
		}
		void blit2()
		{
			BITMAPINFOHEADER bmiHeader = {0};
			void *bitBuf = 0;
			HWND hWnd = (HWND)window;
			HDC  hdc  = GetDC(hWnd);
			HDC bmpDc = CreateCompatibleDC(hdc);

			uint bmpWid=(screenW+3)&(~3);
			uint bmpHei=screenH;

			bmiHeader.biSize=sizeof(bmiHeader);
			bmiHeader.biWidth=bmpWid;
			bmiHeader.biHeight=bmpHei;
			bmiHeader.biPlanes=1;
			bmiHeader.biBitCount=24;
			bmiHeader.biCompression=BI_RGB;
			bmiHeader.biSizeImage=0;
			bmiHeader.biXPelsPerMeter=2048;
			bmiHeader.biYPelsPerMeter=2048;
			bmiHeader.biClrUsed=0;
			bmiHeader.biClrImportant=0;

			HBITMAP hBmp=CreateDIBSection(bmpDc,(BITMAPINFO *)&bmiHeader,DIB_RGB_COLORS,&bitBuf,NULL,0);
			assert( bitBuf );
			memcpy( bitBuf, screen, numBytes() );

			HBITMAP hBmpSave=(HBITMAP)SelectObject(bmpDc,hBmp);

			BitBlt(hdc,0,0,bmpWid,bmpHei,bmpDc,0,0,SRCCOPY);

			SelectObject(bmpDc,hBmpSave);

			ReleaseDC( hWnd, hdc );    
			ReleaseDC( hWnd, bmpDc );    
		}
		
		void blit()
		{
			//~ $()
			int biz= sizeof(BITMAPINFOHEADER);
			BITMAPINFOHEADER bi = {0};
			bi.biSize     = biz;
			bi.biWidth    = screenW;
			bi.biHeight   = screenH;
			bi.biPlanes   = 1;
			bi.biBitCount = 3 * 8;
			bi.biSizeImage = 3 * screenW * screenH;
			bi.biCompression = BI_RGB;
			RECT rc;
			HWND hwnd = (HWND)window;
			HDC hdc = GetDC( hwnd );
			GetClientRect( hwnd, &rc );
			uint r = StretchDIBits( hdc,
				rc.left, rc.top, rc.right, rc.bottom,       // dst
				0, 0, screenW, screenH, // src
				screen, 
				(BITMAPINFO*)&bi, 
				DIB_RGB_COLORS, 
				SRCCOPY );    
			ReleaseDC( hwnd, hdc );    
			//~ if ( r ) printf( "%s\n",GetLastError() );
			//~ if ( r ) printf( __FUNCTION__ " Error %x\n", r );
		}
			
		virtual uint begin3D()
		{
			return 0;
		}
		virtual uint setCamera( Camera * cam )
		{
			float fov = cam->getFov();
			float np  = cam->getNearPlane();
			float fp  = cam->getFarPlane();
			//~ projMatrix.perspective( fov, float(screenW)/screenH, np,fp );	
			//~ projMatrix.identity();
			projMatrix.perspective( fov, 1, np,fp );
			//~ printf( "proj\n" );
			//~ e6::print( projMatrix );

			cam->getWorldMatrix( camMatrix );
			viewMatrix = camMatrix.inverse();			
			//~ printf( "view\n" );
			//~ e6::print( viewMatrix );

			clipMatrix = viewMatrix * projMatrix;
			//clipMatrix = projMatrix * viewMatrix;
			//~ printf( "clip\n" );
			//~ e6::print( clipMatrix );
			//frustum.setup( clipMatrix );
			return 1;
		}
		virtual uint setLight( uint n, Light * light )
		{
			return 0;
		}
		virtual uint setMesh( Mesh * mesh )
		{
			Core::VertexFeature * pos = mesh->getFeature( e6::VF_POS );
			Core::VertexFeature * col = mesh->getFeature( e6::VF_COL );
			Core::VertexFeature * nor = mesh->getFeature( e6::VF_NOR );
			Core::VertexFeature * uv0 = mesh->getFeature( e6::VF_UV0 );
			Core::VertexFeature * uv1 = mesh->getFeature( e6::VF_UV1 );

			texture = 0;
			texW = 0;
			texH = 0;
			if ( doTexture && (uv0||uv1) )
			{
				Core::Texture * t = mesh->getTexture(0);
				if ( t ) 
				{
					Core::Buffer * b = t->getLevel(0);
					if ( b ) 
					{
						b->lock( (void**)&texture );
						texW = t->width();
						texH = t->height();
						b->unlock();
						E_RELEASE(b);
					}
					E_RELEASE(t);
				}
			}

			mesh->getWorldMatrix( mdlMatrix );
			//~ float4x4 matWVP = clipMatrix * mdlMatrix;
			float4x4 matWVP = mdlMatrix * clipMatrix ;
			{
				static float4x4 oldMat;
				if ( oldMat != matWVP )
				{
					printf( "WVP\n" );
					e6::print( matWVP );
					oldMat = matWVP;
				}
			}
			float2 scale( screenW/8, screenH/8 );
			float2 cen( screenW/2, screenH/2 );
			float * pp = (float*)pos->getElement(0); 
			float * pu = uv0 ? (float*)uv0->getElement(0) : 0; 
			for ( uint f=0; f<mesh->numFaces(); f++, pp+=9, pu+=6 )
			{
				Vertex v[3];
				uint out = 0;
				for ( uint i=0,j=0,k=0; i<3; i++,j+=3,k+=2 )
				{
					v[i].pos = matWVP * float4(pp[j],pp[j+1],pp[j+2],1);
					float z = v[i].pos[2];
					if ( z >= 1.0f ) 
					{
						// off near plane, clip tri
						out = 3;;
						break;
					}
					float oow = ( 1.0f / (1.0f - z) );
					v[i].uv0 = uv0 ? float2( texW * pu[k], texH * pu[k+1] ) : float2(0,0);				
					v[i].scr = float2( cen.x + scale.x*v[i].pos[0]*oow,  cen.y + scale.y*v[i].pos[1]*oow );
					out += _clip(v[i].scr[0],v[i].scr[1]);
				}
				if ( out == 3 ) continue;
				
				if ( doWire || !texW || !texture )
				{
					_plot_line( v[0].scr[0],  v[0].scr[1],  v[1].scr[0],  v[1].scr[1], 0xffff0000 );
					_plot_line( v[1].scr[0],  v[1].scr[1],  v[2].scr[0],  v[2].scr[1], 0xffff0000 );
					_plot_line( v[2].scr[0],  v[2].scr[1],  v[0].scr[0],  v[0].scr[1], 0xffff0000 );
				}
				else
				{
					triangle( v[0], v[1], v[2] );
					//_plot_triangle_solid( v );
					//drawtpolyperspsubtri( v );
				}
			}
			E_RELEASE(pos);
			E_RELEASE(nor);
			E_RELEASE(col);
			E_RELEASE(uv0);
			E_RELEASE(uv1);
			return 1;
		}
		virtual uint setRenderState( uint key, uint value )
		{
			uint r = CBaseRendererSoft::set(key,value);
			return r;
		}
		virtual uint setVertexShaderConstant( uint i, const float4 & v )
		{
			return 0;
		}
		virtual uint setPixelShaderConstant( uint i, const float4 & v )
		{
			return 0;
		}
		virtual uint end3D()
		{
			return 0;
		}

		virtual uint get( uint what )
		{
			uint r = CBaseRendererSoft::get(what);
			if ( r != ~0 )
				return r;
			return 0;
		}
		
		
		bool _clip( int x1, int y1 )
		{
			if ( x1<0 || x1 >= (int)screenW ) return 1;
			if ( y1<0 || y1 >= (int)screenH ) return 1;
			return 0;
		}

		void _plot_pixel(int x1, int y1, rgba color )
		{
			unsigned char c[3] = { (color&0xff),((color>>8)&0xff),((color>>16)&0xff) };
			unsigned char * s = screen + 3 * ( screenW * y1 + x1 );
			assert( s+2<screen+numBytes() );
			s[0] = c[0];
			s[1] = c[1];
			s[2] = c[2];
		}

		void _plot_line(int x1, int y1, int x2, int y2, rgba color )
		{
			//~ $();
			// goto Bresenham;
			// Variables
			int i, deltax, deltay, numpixels;
			int d, dinc1, dinc2;
			int x, xinc1, xinc2;
			int y, yinc1, yinc2;

			//Calculate deltaX and deltaY
			deltax = abs(x2 - x1);
			deltay = abs(y2 - y1);

			//Init vars
			if(deltax >= deltay) {
				//If x is independent variable
				numpixels = deltax + 1;
				d = (2 * deltay) - deltax;
				dinc1 = deltay << 1;
				dinc2 = (deltay - deltax) << 1;
				xinc1 = 1;
				xinc2 = 1;
				yinc1 = 0;
				yinc2 = 1;
			} else {
				//If y is independant variable
				numpixels = deltay + 1;
				d = (2 * deltax) - deltay;
				dinc1 = deltax << 1;
				dinc2 = (deltax - deltay) << 1;
				xinc1 = 0;
				xinc2 = 1;
				yinc1 = 1;
				yinc2 = 1;
			}

			//Move the right direction
			if(x1 > x2)	{
				xinc1 = -xinc1;
				xinc2 = -xinc2;
			}
			if(y1 > y2)	{
				yinc1 = -yinc1;
				yinc2 = -yinc2;
			}
			x = x1;
			y = y1;

			//Draw the pixels
			for( i=1; i<numpixels; i++ )	{
				if ( ! _clip( x, y ) )
					_plot_pixel( x, y, color );
				if(d < 0) {
					d = d + dinc1;
					x = x + xinc1;
					y = y + yinc1;
				} else {
					d = d + dinc2;
					x = x + xinc2;
					y = y + yinc2;
				}
			}
		}

		struct Edge 
		{
			float x;
			float dx;
			float tu,tv;
			float3 col;

			Edge() 
				: col(1,1,0) 
				, x(0)
				, dx(0)
				, tu(0)
				, tv(0)
			{}
			Edge( int x, float dx )
				: col(0,1,1)
				, x(x)
				, dx(dx)
				, tu(0)
				, tv(0)
			{
			}
		};

		void _plot_scan_line( uint y, Edge & el, Edge & er )
		{
			int Dx = er.x - el.x;
			if (Dx<=0) return;

			float3 dc = er.col - el.col;
			float u = 1.0f / float(Dx);

			unsigned char * scan = screen + 3 * ( screenW * y + int(el.x) );
			assert( scan+2+Dx*3 < screen+numBytes() );

			for ( int x=el.x; x<er.x; x++ )
			{
				// interpol edge colors:
				float3 c = el.col + dc * u;
				*scan++ = (int(c.x*255.0));
				*scan++ = (int(c.y*255.0));
				*scan++ = (int(c.z*255.0));
			}
		}


		inline
		bool _plot_clip_scan_line( float &x0, float &x1 ) 
		{
			// nothing to do
			if ( x0==x1 )
				return 1;

			// right edge left left edge ?! wlong oldel.
			if ( x1<x0 )  
				return 1;

			// left edge off right
			if ( x0>=(int)texW )  
				return 1;

			// right edge off left
			if ( x1<0 )  
				return 1;

			// clip:
			if ( x0<0 )
				x0 = 0;
			if ( x1>=(int)texW )
				x1 = texW - 1;

			return 0; // visible
		}

		//! edges will get updated !
		void _plot_section( float y0, float y1, Edge & el, Edge & er ) 
		{
			if ( y1 < 0 || y1 < y0 || y0 > (float)texW ) 
			{
				return;
			}

			if ( y0 < 0 ) 
			{
				// interpolate clipping edge:
				el.x -= y0 * el.dx;
				er.x -= y0 * er.dx;
				y0  = 0;
			}
			if ( y1 > (float)texW )
			{
				y1 = texW - 1;
			}

			//float dl = (y1-y0)*el.dx;
			//float dr = (y1-y0)*er.dx;
			//_plot_line( el.x, y0, el.x+dl, y1, 0xffffff00 );
			//_plot_line( el.x, y0, er.x, y0, 0xffffff00 );
			//_plot_line( el.x, y1, er.x, y1, 0xffffff00 );
			//_plot_line( er.x, y0, er.x+dr, y1, 0xffffff00 );
			do 
			{
				if ( _plot_clip_scan_line( el.x, er.x ) )
				{
					_plot_scan_line( y0, el, er );
				}
		        
				el.x += el.dx;
				er.x += er.dx;
				y0 ++ ;

			} while ( y0 < y1 );
		}

		void _plot_triangle_solid ( Vertex *v ) 
		{

			// topdown sorted pointers:
			Vertex *a=&v[0]; //!< top y
			Vertex *b=&v[1]; //!< middle
			Vertex *c=&v[2]; //!< bottom y
			Vertex *s;		 // swapper

			// y-sort (topdown a<b<c): 
			if ( c->scr.y < a->scr.y ) {  s=c; c=a; a=s;  } 
			if ( c->scr.y < b->scr.y ) {  s=c; c=b; b=s;  }
			if ( b->scr.y < a->scr.y ) {  s=b; b=a; a=s;  } 

			// y-dist:
			float Y_ba = b->scr.y - a->scr.y;
			float Y_ca = c->scr.y - a->scr.y;
			float Y_cb = c->scr.y - b->scr.y;
			// x-dist:
			float X_ba = b->scr.x - a->scr.x;
			float X_ca = c->scr.x - a->scr.x;
			float X_cb = c->scr.x - b->scr.x;


			float dab = X_ba / Y_ba;
			float dac = X_ca / Y_ca;
			float dbc = X_cb / Y_cb;


			// clip height:
			if ( b->scr.y >= texH ) b->scr.y = texH - 1;
			if ( c->scr.y >= texH ) c->scr.y = texH - 1;

			// start at point a(top): 

			if ( dab > dac ) {
				//    a
				//      b
				//  c
				// left  edge : ac
				Edge ac( a->scr.x, dac );
				// right edge : ab
				Edge ab( a->scr.x, dab );
				_plot_section( a->scr.y, b->scr.y, ac, ab );
		        
				// left  edge : ac ( same as before )
				// right edge : bc
				Edge bc( b->scr.x, dbc );
				_plot_section( b->scr.y, c->scr.y, ac, bc );
			} else {
				//    a
				//  b    
				//      c
				// left  edge : ab
				Edge ab( a->scr.x, dab );
				// right edge : ac
				Edge ac( a->scr.x, dac );
				_plot_section( a->scr.y, b->scr.y, ab, ac );

				// left  edge : bc
				Edge bc( b->scr.x, dbc );
				// right edge : ac ( same )
				_plot_section( b->scr.y, c->scr.y, bc, ac );
			}   

		}




		//
		// http://www.devmaster.net/forums/showthread.php?t=1884
		//
		int iround(float f )
		{
			return (int)f;
		}
		int min3(int a, int b, int c )
		{
			int r = a;
			if (b<r) r=b;
			if (c<r) r=c;
			return r;
		}
		int max3(int a, int b, int c )
		{
			int r = a;
			if (b>r) r=b;
			if (c>r) r=c;
			return r;
		}
		void triangle(const Vertex &v1, const Vertex &v2, const Vertex &v3)
		{
			char* colorBuffer = (char*)screen;
			// 28.4 fixed-point coordinates
			const int Y1 = iround(16.0f * v1.scr.x);
			const int Y2 = iround(16.0f * v2.scr.y);
			const int Y3 = iround(16.0f * v3.scr.y);

			const int X1 = iround(16.0f * v1.scr.x);
			const int X2 = iround(16.0f * v2.scr.x);
			const int X3 = iround(16.0f * v3.scr.x);

			// Deltas
			const int DX12 = X1 - X2;
			const int DX23 = X2 - X3;
			const int DX31 = X3 - X1;

			const int DY12 = Y1 - Y2;
			const int DY23 = Y2 - Y3;
			const int DY31 = Y3 - Y1;

			// Fixed-point deltas
			const int FDX12 = DX12 << 4;
			const int FDX23 = DX23 << 4;
			const int FDX31 = DX31 << 4;

			const int FDY12 = DY12 << 4;
			const int FDY23 = DY23 << 4;
			const int FDY31 = DY31 << 4;

			// Bounding rectangle
			int minx = (min3(X1, X2, X3) + 0xF) >> 4;
			int maxx = (max3(X1, X2, X3) + 0xF) >> 4;
			int miny = (min3(Y1, Y2, Y3) + 0xF) >> 4;
			int maxy = (max3(Y1, Y2, Y3) + 0xF) >> 4;

			// Block size, standard 8x8 (must be power of two)
			const int q = 8;

			// Start in corner of 8x8 block
			minx &= ~(q - 1);
			miny &= ~(q - 1);

//			(char*&)colorBuffer += miny * stride;

			// Half-edge constants
			int C1 = DY12 * X1 - DX12 * Y1;
			int C2 = DY23 * X2 - DX23 * Y2;
			int C3 = DY31 * X3 - DX31 * Y3;

			// Correct for fill convention
			if(DY12 < 0 || (DY12 == 0 && DX12 > 0)) C1++;
			if(DY23 < 0 || (DY23 == 0 && DX23 > 0)) C2++;
			if(DY31 < 0 || (DY31 == 0 && DX31 > 0)) C3++;

			// Loop through blocks
			for(int y = miny; y < maxy; y += q)
			{
				for(int x = minx; x < maxx; x += q)
				{
					// Corners of block
					int x0 = x << 4;
					int x1 = (x + q - 1) << 4;
					int y0 = y << 4;
					int y1 = (y + q - 1) << 4;

					// Evaluate half-space functions
					bool a00 = C1 + DX12 * y0 - DY12 * x0 > 0;
					bool a10 = C1 + DX12 * y0 - DY12 * x1 > 0;
					bool a01 = C1 + DX12 * y1 - DY12 * x0 > 0;
					bool a11 = C1 + DX12 * y1 - DY12 * x1 > 0;
					int a = (a00 << 0) | (a10 << 1) | (a01 << 2) | (a11 << 3);
		    
					bool b00 = C2 + DX23 * y0 - DY23 * x0 > 0;
					bool b10 = C2 + DX23 * y0 - DY23 * x1 > 0;
					bool b01 = C2 + DX23 * y1 - DY23 * x0 > 0;
					bool b11 = C2 + DX23 * y1 - DY23 * x1 > 0;
					int b = (b00 << 0) | (b10 << 1) | (b01 << 2) | (b11 << 3);
		    
					bool c00 = C3 + DX31 * y0 - DY31 * x0 > 0;
					bool c10 = C3 + DX31 * y0 - DY31 * x1 > 0;
					bool c01 = C3 + DX31 * y1 - DY31 * x0 > 0;
					bool c11 = C3 + DX31 * y1 - DY31 * x1 > 0;
					int c = (c00 << 0) | (c10 << 1) | (c01 << 2) | (c11 << 3);

					// Skip block when outside an edge
					if(a == 0x0 || b == 0x0 || c == 0x0) continue;

//					unsigned int *buffer = colorBuffer;

					// Accept whole block when totally covered
					if(a == 0xF && b == 0xF && c == 0xF)
					{
						for(int iy = 0; iy < q; iy++)
						{
							for(int ix = x; ix < x + q; ix++)
							{
								//buffer[ix] = 0x00007F00; // Green
								_plot_pixel( x,y, 0x00007F00); // Green
							}

							//(char*&)buffer += stride;
						}
					}
					else // Partially covered block
					{
						int CY1 = C1 + DX12 * y0 - DY12 * x0;
						int CY2 = C2 + DX23 * y0 - DY23 * x0;
						int CY3 = C3 + DX31 * y0 - DY31 * x0;

						for(int iy = y; iy < y + q; iy++)
						{
							int CX1 = CY1;
							int CX2 = CY2;
							int CX3 = CY3;

							for(int ix = x; ix < x + q; ix++)
							{
								if(CX1 > 0 && CX2 > 0 && CX3 > 0)
								{
								//	buffer[ix] = 0x0000007F;<< // Blue
								_plot_pixel( x,y, 0x0000007F); // Green
								}

								CX1 -= FDY12;
								CX2 -= FDY23;
								CX3 -= FDY31;
							}

							CY1 += FDX12;
							CY2 += FDX23;
							CY3 += FDX31;

							//(char*&)buffer += stride;
						}
					}
				}

//				(char*&)colorBuffer += q * stride;
			}
		}

///////////////////
//		float dizdx, duizdx, dvizdx, dizdy, duizdy, dvizdy;
//		float xa, xb, iza, uiza, viza;
//		float dxdya, dxdyb, dizdya, duizdya, dvizdya;
		unsigned char *texture;
		uint texW, texH;
//			
//		void drawtpolyperspsubtri( Vertex tri[3] )
//		{
//			float x1, y1, x2, y2, x3, y3;
//			float iz1, uiz1, viz1, iz2, uiz2, viz2, iz3, uiz3, viz3;
//			float dxdy1=-1, dxdy2=-1, dxdy3=-1;
//			float tempf;
//			float denom;
//			float dy;
//			int y1i, y2i, y3i;
//			int side;
//
//			// Shift XY coordinate system (+0.5, +0.5) to match the subpixeling
//			//  technique
//
//			x1 = tri[0].scr.x + 0.5;
//			y1 = tri[0].scr.y + 0.5;
//			x2 = tri[1].scr.x + 0.5;
//			y2 = tri[1].scr.y + 0.5;
//			x3 = tri[2].scr.x + 0.5;
//			y3 = tri[2].scr.y + 0.5;
//
//			// Calculate alternative 1/Z, U/Z and V/Z values which will be
//			//  interpolated
//
//			iz1 = texW / tri[0].pos.z;
//			iz2 = 1.0f / tri[1].pos.z;
//			iz3 = 1.0f / tri[2].pos.z;
//			uiz1 = tri[0].uv0.x * iz1;
//			viz1 = tri[0].uv0.y * iz1;
//			uiz2 = tri[1].uv0.x * iz2;
//			viz2 = tri[1].uv0.y * iz2;
//			uiz3 = tri[2].uv0.x * iz3;
//			viz3 = tri[2].uv0.y * iz3;
//
//			// Sort the vertices in ascending Y order
//
//		#define swapfloat(x, y) tempf = x; x = y; y = tempf;
//			if (y1 > y2)
//			{
//				swapfloat(x1, x2);
//				swapfloat(y1, y2);
//				swapfloat(iz1, iz2);
//				swapfloat(uiz1, uiz2);
//				swapfloat(viz1, viz2);
//			}
//			if (y1 > y3)
//			{
//				swapfloat(x1, x3);
//				swapfloat(y1, y3);
//				swapfloat(iz1, iz3);
//				swapfloat(uiz1, uiz3);
//				swapfloat(viz1, viz3);
//			}
//			if (y2 > y3)
//			{
//				swapfloat(x2, x3);
//				swapfloat(y2, y3);
//				swapfloat(iz2, iz3);
//				swapfloat(uiz2, uiz3);
//				swapfloat(viz2, viz3);
//			}
//		#undef swapfloat
//
//			y1i = y1;
//			y2i = y2;
//			y3i = y3;
//
//			// Skip poly if it's too thin to cover any pixels at all
//
//			if ((y1i == y2i && y1i == y3i)
//				|| ((int) x1 == (int) x2 && (int) x1 == (int) x3))
//				return;
//
//			// Calculate horizontal and vertical increments for UV axes (these
//			//  calcs are certainly not optimal, although they're stable
//			//  (handles any dy being 0)
//
//			denom = ((x3 - x1) * (y2 - y1) - (x2 - x1) * (y3 - y1));
//
//			if (!denom)		// Skip poly if it's an infinitely thin line
//				return;	
//
//			denom = 1 / denom;	// Reciprocal for speeding up
//			dizdx = ((iz3 - iz1) * (y2 - y1) - (iz2 - iz1) * (y3 - y1)) * denom;
//			duizdx = ((uiz3 - uiz1) * (y2 - y1) - (uiz2 - uiz1) * (y3 - y1)) * denom;
//			dvizdx = ((viz3 - viz1) * (y2 - y1) - (viz2 - viz1) * (y3 - y1)) * denom;
//			dizdy = ((iz2 - iz1) * (x3 - x1) - (iz3 - iz1) * (x2 - x1)) * denom;
//			duizdy = ((uiz2 - uiz1) * (x3 - x1) - (uiz3 - uiz1) * (x2 - x1)) * denom;
//			dvizdy = ((viz2 - viz1) * (x3 - x1) - (viz3 - viz1) * (x2 - x1)) * denom;
//
//			// Calculate X-slopes along the edges
//
//			if (y2 > y1)
//				dxdy1 = (x2 - x1) / (y2 - y1);
//			if (y3 > y1)
//				dxdy2 = (x3 - x1) / (y3 - y1);
//			if (y3 > y2)
//				dxdy3 = (x3 - x2) / (y3 - y2);
//
//			// Determine which side of the poly the longer edge is on
//
//			side = dxdy2 > dxdy1;
//
//			if (y1 == y2)
//				side = x1 > x2;
//			if (y2 == y3)
//				side = x3 > x2;
//
//			if (!side)	// Longer edge is on the left side
//			{
//				// Calculate slopes along left edge
//
//				dxdya = dxdy2;
//				dizdya = dxdy2 * dizdx + dizdy;
//				duizdya = dxdy2 * duizdx + duizdy;
//				dvizdya = dxdy2 * dvizdx + dvizdy;
//
//				// Perform subpixel pre-stepping along left edge
//
//				dy = 1.0f - (y1 - y1i);
//				xa = x1 + dy * dxdya;
//				iza = iz1 + dy * dizdya;
//				uiza = uiz1 + dy * duizdya;
//				viza = viz1 + dy * dvizdya;
//
//				if (y1i < y2i)	// Draw upper segment if possibly visible
//				{
//					// Set right edge X-slope and perform subpixel pre-
//					//  stepping
//
//					xb = x1 + dy * dxdy1;
//					dxdyb = dxdy1;
//
//					drawtpolyperspsubtriseg(y1i, y2i);
//				}
//				if (y2i < y3i)	// Draw lower segment if possibly visible
//				{
//					// Set right edge X-slope and perform subpixel pre-
//					//  stepping
//
//					xb = x2 + (1.0f - (y2 - y2i)) * dxdy3;
//					dxdyb = dxdy3;
//
//					drawtpolyperspsubtriseg(y2i, y3i);
//				}
//			}
//			else	// Longer edge is on the right side
//			{
//				// Set right edge X-slope and perform subpixel pre-stepping
//
//				dxdyb = dxdy2;
//				dy = 1.0f - (y1 - y1i);
//				xb = x1 + dy * dxdyb;
//
//				if (y1i < y2i)	// Draw upper segment if possibly visible
//				{
//					// Set slopes along left edge and perform subpixel
//					//  pre-stepping
//
//					dxdya = dxdy1;
//					dizdya = dxdy1 * dizdx + dizdy;
//					duizdya = dxdy1 * duizdx + duizdy;
//					dvizdya = dxdy1 * dvizdx + dvizdy;
//					xa = x1 + dy * dxdya;
//					iza = iz1 + dy * dizdya;
//					uiza = uiz1 + dy * duizdya;
//					viza = viz1 + dy * dvizdya;
//
//					drawtpolyperspsubtriseg(y1i, y2i);
//				}
//				if (y2i < y3i)	// Draw lower segment if possibly visible
//				{
//					// Set slopes along left edge and perform subpixel
//					//  pre-stepping
//
//					dxdya = dxdy3;
//					dizdya = dxdy3 * dizdx + dizdy;
//					duizdya = dxdy3 * duizdx + duizdy;
//					dvizdya = dxdy3 * dvizdx + dvizdy;
//					dy = 1.0f - (y2 - y2i);
//					xa = x2 + dy * dxdya;
//					iza = iz2 + dy * dizdya;
//					uiza = uiz2 + dy * duizdya;
//					viza = viz2 + dy * dvizdya;
//
//					drawtpolyperspsubtriseg(y2i, y3i);
//				}
//			}
//		}
//
//		void drawtpolyperspsubtriseg(int y1, int y2)
//		{
//			unsigned char *scr;
//			int x1, x2, ui,vi;
//			float z, u, v, dx;
//			float iz, uiz, viz;
//
//			if ( y1 < 0 )
//				y1 = 0;
//			if ( y2 > (int)screenH-1 )
//				y2 = screenH-1;
//			
//			while (y1 < y2)		// Loop through all lines in the segment
//			{
//				x1 = xa;
//				x2 = xb;
//				if ( x2 > (int)screenW-1 )
//					x2 = screenW -1;
//				// Perform subtexel pre-stepping on 1/Z, U/Z and V/Z
//
//				//~ dx = 1 - (xa - x1);
//				dx = 1.0f - (xa - x1);
//				iz = iza + dx * dizdx;
//				uiz = uiza + dx * duizdx;
//				viz = viza + dx * dvizdx;
//					
//				scr = screen + 3 * (y1 * screenW + x1);
//
//				while (x1++ < x2)	// Draw horizontal line
//				{
//					// Calculate U and V from 1/Z, U/Z and V/Z
//					if ( x1 >= 0 )
//					{
//						z = 1.0f / iz;
//						u = uiz * z;
//						v = viz * z;
//						ui = e6::clamp( int(u), 0, int(texW) );
//						vi = e6::clamp( int(v), 0, int(texH) );
//						// Copy pixel from texture to screen
//						unsigned char * texel = texture + 4*(vi * texW + ui); // skip alpha
//						//~ unsigned char * texel = texture + 4*(int(v) * texW + int(u)); // skip alpha
//						*scr++ = *texel++;
//						*scr++ = *texel++;
//						*scr++ = *texel++;
//						//~ *scr++ = *texel++;
//					}
//					else
//					{
//						scr +=3;
//					}
//					// Step 1/Z, U/Z and V/Z horizontally
//
//					iz += dizdx;
//					uiz += duizdx;
//					viz += dvizdx;
//				}
//
//				// Step along both edges
//
//				xa += dxdya;
//				xb += dxdyb;
//				iza += dizdya;
//				uiza += duizdya;
//				viza += dvizdya;
//
//				y1++;
//			}
//		} //drawtpolyperspsubtriseg
		
	}; // CRenderer

} //namespace RendererSoft



extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Renderer",		"RendererSoft",	RendererSoft::CRenderer::createSingleton, RendererSoft::CRenderer::classRef	},
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
	mv->modVersion = ( "RendererSoft 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
