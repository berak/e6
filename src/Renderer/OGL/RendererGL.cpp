

///PPP disabled shaders !!


#include "../../Core/Core.h"
#include "../../Core/Frustum.h"
#include "../../e6/e6_impl.h"
#include "../../e6/e6_math.h"
#include "../../e6/e6_sys.h"
#include "../../e6/e6_enums.h"
#include "../../e6/e6_container.h"
#include "../BaseRenderer.h"

#include "gl_tk.h"
#include "gl_2.h"

#include <Windows.h>
#include "glew.h"
#include "wglew.h"
//#include <GL/gl.h>
//#include <GL/glu.h>
#include <vector>

using e6::rgba;
using e6::uint;
using e6::Class;
using e6::CName;
using e6::ClassInfo;
using e6::float2;
using e6::float3;
using e6::float4;
using e6::float4x4;
using BaseRenderer::CBaseRenderer;
using BaseRenderer::Cache;

 #define CHANGE_STATE(a,b)     if ( (b) ) glEnable((a)); else glDisable((a)); 
//#define CHECK_ERR { char *s=RGL::checkError(); if(s) printf( "\nGLError :%s(%i) : %s\n",__FUNCTION__,__LINE__,s); } 

namespace RGL 
{

	// Texture cache
	struct _ClearTexItem
	{
		static void clear( uint  it ) 
		{
			if ( RGL::texIsResident( it ) )
			{
				printf("gl removeTex %d\n", it );
				RGL::texRelease( it );
			}
		}
	};

	struct TexCache 
		: public BaseRenderer::Cache< Core::Texture *, uint, 500, _ClearTexItem >
	{
		static const uint ttl = 400;

		uint texID( Core::Texture *t )
		{
//			$();
			if ( ! t ) return 1; // texerror
			uint id = 0;

			TexCache::Item * it = TexCache::find(t);
			if ( it )
			{
				it->ttl = ttl;
				id = it->val;
			}
 			if ( id  && glIsTexture(id) ) return id;

			// add to cache:
			if ( t->type() == e6::TU_RENDERTARGET )
			{
				float vp[4];
				glGetFloatv( GL_VIEWPORT, vp );
				CHECK_ERR;
				//id = RGL::createRenderTarget( t->width(), t->height() );
				id = RGL::createRenderTarget( vp[2], vp[3] );
				t->alloc(0,vp[2],vp[3],0);
				CHECK_ERR;
				printf("gl created RT  %d %s\n", id, t->getName() );
			}
			else
			{
				unsigned char * pixel = 0;
				Core::Buffer * b = t->getLevel(0);
				if ( ! b ) return 1;
				b->lock( (void**)&pixel );
				if ( pixel )
				{
					bool doMip = (t->type() == e6::TU_STANDARD);
					id = RGL::texCreate( t->width(), t->height(), 4, pixel, doMip );
				}
				b->unlock();
				E_RELEASE(b);
				printf("gl created tex %d %s\n", id, t->getName() );
				//_tex.push_back( new TexCacheItem(t, id) );
				CHECK_ERR;
			}

			TexCache::insert( t, id, ttl );
			return id ? id : 1;
		}
	};



	// shader cache
	struct _ClearShaderItem 
	{
		static void clear( uint  it ) 
		{
			printf("gl removeShader %d\n", it );
			RGL::removeShader( it );
		}
	};

	struct ShaderCache 
		: public BaseRenderer::Cache< Core::Shader *, uint, 500, _ClearShaderItem >
	{
		static const uint ttl = 400;
		uint shaderID(Core::Shader * sh)
		{
			uint id = 0;
			ShaderCache::Item * it = ShaderCache::find(sh);
			if ( ! it )
			{
				const char * n = sh->getPath();
				if ( strstr( n, "vs_" ) )
					id = RGL::createVertexShader( n );
				else
					id = RGL::createPixelShader( n );

				ShaderCache::insert( sh, id, ttl );
			}
			else
			{
				id = it->val;
				it->ttl = ttl;
			}
			return id;
		}
	};

	//  ProgramCache
	struct _ClearProgramItem 
	{
		static void clear( uint  it ) 
		{
			printf("gl removeProgram %d\n", it );
			RGL::removeProgram( it );
		}
	};

	struct ProgramCache 
		: public BaseRenderer::Cache< Core::Mesh *, uint, 500, _ClearProgramItem >
	{
		static const uint ttl = 400;

		ShaderCache _sha;
		uint defprog;

		ProgramCache()
		{
			// add default prog: id(1)
			//programId(
			defprog = RGL::createProgram( "vs_0.glsl", "ps_0.glsl" );
		}
		
		~ProgramCache()
		{
			RGL::removeProgram(defprog);
		}

		//
		/// tries to get a working program, in this order:
		/// * it's already in the cache.
		/// * try to create one from the mesh shaders
		/// * return defprog
		//
		uint programID(Core::Mesh * mesh)
		{
			uint program = 0;
			ProgramCache::Item * it = ProgramCache::find(mesh);
			if ( ! it )
			{
				uint vs=0, ps=1;
				Core::Shader * cvs = mesh->getVertexShader();
				if ( cvs )
				{
					vs = _sha.shaderID(cvs);				
					CHECK_ERR;
				}
				E_RELEASE( cvs );
				Core::Shader * cps = mesh->getPixelShader();
				if ( cps )
				{
					ps = _sha.shaderID(cps);				
					CHECK_ERR;
				}
				E_RELEASE( cps );

				if ( vs && ps )
				{
					program = RGL::createProgram( vs, ps );
					CHECK_ERR;
					if ( program ) 
					{
						this->insert( mesh, program, 400 );
					}
				}
				if ( ! program )
				{
					printf( "shader err %p %p : %i %i (revert to default.)\n", cvs, cps, vs, ps  );
					program = defprog;
				}
			}
			else
			{
				program = it->val;
				it->ttl = ttl;
			}
			return program;
		}
	};

	//struct Program 
	//{
	//	uint id;
	//	uint vs;
	//	uint ps;
	//	Program( uint i=0, uint v=0, uint p=0 ) : id(i), vs(v), ps(p) {}
	//};

	//struct BlendingState {
	//	bool blend;
	//	int  blend_l;
	//	int  blend_r;

	//	bool depth;
	//	int  depth_m;
	//	int  depth_f;

	//	int  env;

	//	// defaults to DIFFUSE.
	//	BlendingState() 
	//		: blend(0)
	//		, blend_l(GL_SRC_ALPHA)
	//		, blend_r(GL_ONE_MINUS_SRC_ALPHA)
	//		, depth(1)
	//		, depth_m(GL_TRUE)
	//		, depth_f(GL_LESS)
	//		, env(GL_REPLACE)
	//	{ }
	//} _blends[32];


	//void _initBlends() 
	//{
	//	_blends[1].env     = GL_MODULATE;

	//	_blends[2].blend   = true;

	//	_blends[3].env     = GL_MODULATE;
	//	_blends[3].blend   = true;
	//	_blends[3].blend_l = GL_ZERO;
	//	_blends[3].blend_r = GL_SRC_COLOR;
	//	_blends[3].depth_m = GL_FALSE;
	//	_blends[3].depth_f = GL_EQUAL;

	//	_blends[4].blend    = true;
	//	_blends[4].blend_l  = GL_ONE;
	//	_blends[4].blend_r  = GL_ONE;

	//	_blends[5].blend    = true;
	//	_blends[5].blend_l  = GL_DST_COLOR;
	//	_blends[5].blend_r  = GL_ZERO;
	//	_blends[5].depth    = false;

	//	_blends[6].blend    = true;
	//	_blends[6].blend_l  = GL_ZERO;
	//	_blends[6].blend_r  = GL_SRC_COLOR;
	//	_blends[6].depth_m  = GL_FALSE;
	//	_blends[6].depth_f  = GL_EQUAL;
	//}

	//------------- ------------------ ----------------- --------------
	//
	//
	//
	//------------- ------------------ ----------------- --------------

    enum { TTL_MAX=500 };

    //struct TexCacheItem
    //{
    //  	Core::Texture * t;
    //	uint  ttl;	
    //	uint  texID;	

    //	TexCacheItem(Core::Texture *x, uint id) : t(x), ttl(TTL_MAX), texID(id) { E_ADDREF(t); }
    //	~TexCacheItem() {$X(); E_RELEASE(t); }
    //};

	//typedef std::vector< TexCacheItem *> TexCache;




	struct CRenderer 
		: CBaseRenderer< Core::Renderer , CRenderer >
	{
		typedef CBaseRenderer< Core::Renderer , CRenderer > CBaseRendererGL;
		//~ float4x4 projMatrix;
		//~ float4x4 viewMatrix;
		//~ float4x4 clipMatrix;
		//~ float4x4 camMatrix;
		//~ float4x4 mdlMatrix;
		
		//~ bool doCull;
		//~ bool doCCW;
		//~ bool doWire;
		//~ bool doTexture;
		//~ bool doLighting;
		//~ bool doFullScreen;
		//~ bool doClearBackBuffer;
		//~ bool doZTest;
		//~ bool doZWrite;
		//~ bool doAlpha;
		//~ bool doAlphaTest;

		//~ uint zBufferDepth ;
		//~ uint stateBlendSrc;
		//~ uint stateBlendDst;
		//~ uint shadeMode;
		//~ rgba bgColor;
		
		// rendertarget
		uint last_ti;
		uint last_ti_w;
		uint last_ti_h;

		int curProg;

		bool shaderSupported;

		TexCache _tex;
		ProgramCache _pro;

		Core::Frustum frustum;

		CRenderer()
			//~ : doCull(1)
			//~ , doCCW(0)
			//~ , doWire(0)
			//~ , doTexture(1)
			//~ , doLighting(1)
			//~ , doFullScreen(0)
			//~ , doClearBackBuffer(1)
			//~ , doZWrite(0)
			//~ , doZTest(0)
			//~ , doAlpha(0)
			//~ , doAlphaTest(0)
			//~ , zBufferDepth(16)
			//~ , stateBlendSrc( e6::BM_ONE )
			//~ , stateBlendDst( e6::BM_ZERO )
			//~ , shadeMode( 1 )
			//~ , bgColor(0)
			: shaderSupported(0)
			, last_ti(0)
			, last_ti_w(0)
			, last_ti_h(0)
			, curProg(-1)
		{
			setName( "RendererGL");
		}


		~CRenderer()
		{
		}


		virtual void *cast( const char * str )
		{
			return _cast(str,"Core.Renderer", "e6.Name");
		}


		// return pixelformat or 0
		int initMultisample(HWND hWnd,PIXELFORMATDESCRIPTOR & pfd)
		{  
			// See If The String Exists In WGL!
			if ( ! RGL::isExtensionSupported("WGL_ARB_multisample") )
			{
				printf( "! WGL_ARB_multisample !\n" );
				return 0;
			}
			PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
				(PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			if ( ! wglChoosePixelFormatARB )
			{
				printf( "! wglChoosePixelFormatARB() !\n" );
				return 0;
			}
			HDC hDC = GetDC(hWnd);

			int pixelFormat;
			UINT numFormats;
			float fAttributes[] = {0,0};

			int iAttributes[] = { WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
				WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
				WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
				WGL_COLOR_BITS_ARB,24,
				WGL_ALPHA_BITS_ARB,8,
				WGL_DEPTH_BITS_ARB,zBufferDepth,
				WGL_STENCIL_BITS_ARB,0,
				WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
				WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
				WGL_SAMPLES_ARB, 4 ,						// Check For 4x Multisampling
				0,0};

			bool valid = wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);
		 
			if ( ! valid || (numFormats < 1) )
			{
				//  Test For 2 Samples
				iAttributes[19] = 2;
				valid = wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);			
			}

			if ( ! valid || (numFormats < 1) )
			{
				//  Test For 1 Samples
				iAttributes[19] = 1;
				valid = wglChoosePixelFormatARB(hDC,iAttributes,fAttributes,1,&pixelFormat,&numFormats);			
			}
			if ( ! valid || (numFormats < 1) )
			{
				pixelFormat = 0;
			}
			printf( "multisample %d %d %4x\n", valid, iAttributes[19], pixelFormat );
			return  pixelFormat;
		}




		virtual uint init( const void * win, uint w, uint h )
		{
			static bool running = 0;
			#ifndef USE_GLUT
				// release old rendercontext, if any:
				if ( ! win ) 
					{	wglMakeCurrent( 0,0 ); return running=0;     }

				// init rendercontext:
				if ( ! running )
				{
					int colbits   = 24;
					int depthbits = zBufferDepth;
					int stencilbits = 0; //zBufferDepth;

					HDC hdc = GetDC((HWND)win);
					if ( ! hdc )  
						{ printf( "GLrenderer could not get hdc from win %p",win); return 0; }

					PIXELFORMATDESCRIPTOR pfd = {0};
					pfd.nSize        = sizeof (pfd);
					pfd.nVersion     = 1;
					pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
					pfd.iPixelType   = PFD_TYPE_RGBA;
					pfd.cColorBits   = colbits; // 0:let the machine choose
					pfd.cDepthBits   = depthbits;
					pfd.cStencilBits = stencilbits;
					pfd.iLayerType   = PFD_MAIN_PLANE;
					int res = 0;
					int iFormat = ChoosePixelFormat ( hdc, &pfd );
						res = SetPixelFormat( hdc, iFormat, &pfd );
						if ( ! res )
						{
							printf( "Error SetPixelFormat(%4x)\n", iFormat );
							return 0;
						}


					HGLRC rc = wglGetCurrentContext();
					if ( ! rc ) 
					{
						rc = wglCreateContext( hdc );
						if ( ! rc )  { printf( "GLrenderer could not create rendercontext"); return 0; }
					}

					running = wglMakeCurrent( hdc, rc );

					// init extensions:
					uint r = glewInit();
					if ( r )
					{
						printf( "Error glewInit(%d : %s)\n", r, glewGetErrorString(r));
						shaderSupported = 0;
					}
					else
					{
						if (glewIsSupported("GL_VERSION_2_0"))
						{
							shaderSupported = 1;
							printf("glew: Ready for OpenGL 2.0\n");
						}
						else 
							printf("glew: OpenGL 2.0 not supported\n");
					}

					int multisampleFormat = initMultisample((HWND)win, pfd);
					if (multisampleFormat)
					{
						// change pixelformat
						res = SetPixelFormat( hdc, multisampleFormat, &pfd );
						wglDeleteContext( rc );
						rc = wglCreateContext( hdc );
						running = wglMakeCurrent( hdc, rc );
						glEnable(GL_MULTISAMPLE_ARB);
					}

					printf( "SetPixelFormat(%4x / %4x)\n",multisampleFormat, iFormat );
					if ( ! running )
					{
						res = SetPixelFormat( hdc, iFormat, &pfd );
						running = wglMakeCurrent( hdc, rc );
						if ( ! res )
						{
							printf( "Error SetPixelFormat(%4x)\n", iFormat );
							return 0;
						}
					}

					printf(   RGL::driverInfo() );
				}




				if ( ! running )  { printf( "GLrenderer could not activate rendercontext"); return 0; }
				RGL::texParams( GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPLACE );

				uint errID = RGL::texChecker( 16, 4 );
				//_tex.push_back( new TexCacheItem(0, errID) );

				//RECT r;
				//GetClientRect((HWND)win,&r);
				//Machine::log("gl activated %x %x %x (%d %d)(%d %d)\n", win, rc,hdc, r.right, r.bottom, colbits, depthbits);
			#endif

			glViewport(0,0, w,h);
			restore();
				
			CHECK_ERR;
			return running;
		}


		virtual void clear()
		{
//			$(); 
			if ( this->classRef() != 1 )
			{
				printf( __FUNCTION__ " : CLASSREF ERR ! (%x)\n", this->classRef() );
			}
			uint bits = GL_DEPTH_BUFFER_BIT;
			if ( doClearBackBuffer )
			{
				float b = float((bgColor&0xff))/255.0f;
				float g = float((bgColor&0xff00)>>8)/255.0f;
				float r = float((bgColor&0xff0000)>>16)/255.0f;
				glClearColor(r,g,b, 1);
				bits |= GL_COLOR_BUFFER_BIT;
			}
			glClear( bits );//| GL_STENCIL_BUFFER_BIT );			
			CHECK_ERR;
		}

		virtual void swapBuffers()
		{
//			$();
			#ifndef USE_GLUT
				HDC hdc = wglGetCurrentDC();
				SwapBuffers( hdc );
				CHECK_ERR;
			#else
				glutSwapBuffers();
			#endif

			_tex.tick();
			if ( shaderSupported )
			{
///PPP!!				_sha.tick();
				_pro.tick();
			}

			CHECK_ERR;
		}

		uint restore()
		{
			$();
			setRenderState( e6::RF_LIGHTING,   doLighting );
			setRenderState( e6::RF_TEXTURE,   doTexture );
			setRenderState( e6::RF_CULL,   doCull );
			setRenderState( e6::RF_CCW,   doCCW );
			setRenderState( e6::RF_ZTEST,   doZTest );
			setRenderState( e6::RF_WIRE,   doWire );
			setRenderState( e6::RF_ALPHA,   doAlpha );
			RGL::texParams( GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPLACE );
			//~ _setBlend(0);
			CHECK_ERR;
			last_ti   = 0;
			last_ti_w = 0;
			last_ti_h = 0;
			return 1;
		}


		virtual uint begin3D()
		{
			glPushMatrix();
			CHECK_ERR;
			return 1;
		}

		virtual uint setRenderTarget( Core::Texture * tex ) 
		{
			last_ti   = 0;
			last_ti_w = 0;
			last_ti_h = 0;

			if ( tex )
			{
				last_ti   = this->_tex.texID( tex );
				CHECK_ERR;
				float vp[4];
				glGetFloatv( GL_VIEWPORT, vp );
				CHECK_ERR;
				last_ti_w = vp[2]; //tex->width();
				last_ti_h = vp[3]; //tex->height();
				return 1;
			}
			return 0;
		}

		virtual uint setCamera( Core::Camera * cam )
		{
			float fov = cam->getFov();
			float np  = cam->getNearPlane();
			float fp  = cam->getFarPlane();
			RGL::perspective( fov,np,fp, projMatrix );

			cam->getWorldMatrix( camMatrix );
			viewMatrix = camMatrix.inverse();			

			if ( ! shaderSupported )
			{
				glLoadMatrixf( viewMatrix );
			}

			clipMatrix = viewMatrix * projMatrix;
			//clipMatrix = projMatrix * viewMatrix;
			frustum.setup( clipMatrix );
			CHECK_ERR;
			return 1;
		}

		virtual uint setLight( uint n, Core::Light * light ) 
		{
			if ( ! doLighting || n>3 )
				return 0;

			float4x4 lm;
			light->getWorldMatrix(lm);
			float3 p = lm.transpose().getPos();
			glLightfv( GL_LIGHT0+n, GL_POSITION,  (float*)&(p) ); 
			glLightfv( GL_LIGHT0+n, GL_DIFFUSE,   (float*)&(float4(light->getColor()) ) ); 
			//glLightfv( GL_LIGHT0+n, GL_AMBIENT,  afPropertiesAmbient); 
			//glLightfv( GL_LIGHT0+n, GL_SPECULAR, afPropertiesSpecular); 
			glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 1.0); 

			glEnable( GL_LIGHT0+n ); 
			CHECK_ERR;
			return 1;
		}
		

		virtual uint setMesh( Core::Mesh * mesh )
		{
//			$(); 
			float4x4 mdlMatrix;
			mesh->getWorldMatrix( mdlMatrix );

			if ( this->doCull )
			{
				bool in = frustum.intersectSphere( mdlMatrix.getPos(), mesh->getSphere() );
				if ( ! in ) 
				{
					//printf("Culled %s.\n", mesh->getName() );
					return 0;
				}
			}
			//~ if ( doCull && ! frustum.intersectSphere( mdlMatrix.getPos(), mesh->getSphere() ) )
			//~ {
				//~ return 0;
			//~ }

			Core::VertexFeature * pos = mesh->getFeature( e6::VF_POS );
			Core::VertexFeature * col = mesh->getFeature( e6::VF_COL );
			Core::VertexFeature * nor = mesh->getFeature( e6::VF_NOR );
			Core::VertexFeature * uv0 = mesh->getFeature( e6::VF_UV0 );
			Core::VertexFeature * uv1 = mesh->getFeature( e6::VF_UV1 );

			if ( doTexture && (uv0||uv1) )
			{
				for ( uint i=0; i<4; i++ )
				{
					uint tp   = e6::TU_STANDARD;
					uint flag = GL_TEXTURE_2D;
					Core::Texture * t = mesh->getTexture(i);
					if ( t && t->width() && t->height() )
					{
						glActiveTexture( GL_TEXTURE0 + i );
						CHANGE_STATE( GL_TEXTURE_2D, 1 );
						uint id   = _tex.texID( t );
						tp = t->type();
						if ( tp == e6::TU_DYNAMIC || tp == e6::TU_VIDEO )
						{
							Core::Buffer * cbuf = t->getLevel(0);
							if ( cbuf )
							{
								unsigned char * pixel = 0;
								cbuf->lock( (void**)&pixel );
								if ( pixel )
								{
									glBindTexture( flag, id );
									//glTexParameteri( flag, GL_GENERATE_MIPMAP, 0 );
									//glTexSubImage2D( flag, 0, 0,0, t->width(), t->height(),GL_RGBA,GL_UNSIGNED_BYTE, pixel );
							        //glTexImage2D( flag, 0, 4, t->width(), t->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel );
							        gluBuild2DMipmaps( flag,4, t->width(), t->height(), GL_RGBA, GL_UNSIGNED_BYTE, pixel );
									cbuf->unlock();
								} else{printf("!lock\n");}
								E_RELEASE( cbuf );
							}
						}
						else // TU_STANDARD
						{
							glBindTexture( flag, id );
							CHECK_ERR;
						}
						
						E_RELEASE(t);
						continue;
					}
					else // no tex
					if ( i==0 ) 
					{ 
						glBindTexture( flag, 1 ); // errortex only for stage 0
						continue;
					}
					break;

					////if ( t->format() == e6::t
					//if ( tp >= e6::TU_CUBE )
					//{
					//	glEnable( GL_TEXTURE_CUBE_MAP_EXT );
					//	int ci = tp - e6::TU_CUBE;
					//	flag = GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT + ci;
					//}

					//glBindTexture( flag, id );
				}
				//glActiveTexture(GL_TEXTURE0);
			}

			if ( shaderSupported )
			{
				uint program = _pro.programID( mesh );

				if ( program ) 
				{
					curProg = program;
					RGL::bindProgram( program );
					CHECK_ERR;

					//e6::float4x4 mat( clipMatrix * mdlMatrix );
					e6::float4x4 mat( mdlMatrix * clipMatrix );
					RGL::setConstant( program, "mWorldViewProj", mat, 16 );
					float t = 0.001 * e6::sys::getMicroSeconds();
					float4 mv( t,sin(t),cos(t),0.04f  );
					RGL::setConstant( program, "fTime", mv, 4 );
					//float4 re;
					//glGetFloatv(GL_VIEWPORT,re);
					//RGL::setConstant( program, "resolution", re, 4 );
					//RGL::setConstants( program );
				}
			}
			else
			{
				glPushMatrix();
				glMultMatrixf( mdlMatrix );
			}


			Core::Shader::Constant *c = mesh->getVertexShaderConstant(e6::VS_MATERIAL_COLOR_DIFFUSE);
			if ( c )
				glColor4fv( (float*)&(c->value) );
			else
				glColor4f( .4f,.4f,.4f,1 );

			float * p = 0;
			float * pp = (float*)pos->getElement(0); 
			float * pn = nor ? (float*)nor->getElement(0) : 0; 
			float * pu = uv0 ? (float*)uv0->getElement(0) : 0; 
			float * pv = uv1 ? (float*)uv1->getElement(0) : 0; 
			uint  * pc = col ? (uint *)col->getElement(0) : 0; 
			#define V   p=pp; glVertex3f( p[0],p[1],p[2] ); pp+=3;
			#define C   if ( col ) { glColor3ub( (((*pc)>>16)&0xff),(((*pc)>>8)&0xff),(((*pc))&0xff) ); pc+=1; }
			#define N   if ( doLighting && nor ) { p=pn; glNormal3f( p[0],p[1],p[2] ); pn+=3; }
			#define T0  if ( doTexture && uv0 )  { p=pu; glMultiTexCoord2f(GL_TEXTURE0, p[0],p[1] ); pu+=2; }
			#define T1  if ( doTexture && uv1 )  { p=pv; glMultiTexCoord2f(GL_TEXTURE1, p[0],p[1] ); pv+=2; }
			uint nFaces = mesh->numFaces();
			glBegin( GL_TRIANGLES );
			for ( uint i=0; i<nFaces; i++ )
			{
				C;T0;T1;N;V;
				C;T0;T1;N;V;
				C;T0;T1;N;V;
			}
			glEnd();

			if ( showNormals && nor )  
			{ 
				CHANGE_STATE( GL_TEXTURE_2D, 0 );
				CHANGE_STATE( GL_LIGHTING, 0 );
				glColor3ub(0xff, 0xee, 0x22); 
				glBegin( GL_LINES );
				for ( uint i=0; i<nFaces; i++ )
				{
					float3 p0( (float*)pos->getElement(i*3) );
					float3 p1( p0 + 1.5 * float3((float*)nor->getElement(i*3)) );
					glVertex3fv( p0 );
					glVertex3fv( p1 );
				}
				glEnd();
				CHANGE_STATE( GL_TEXTURE_2D, doTexture );
				CHANGE_STATE( GL_LIGHTING, doLighting );
			}

			if ( ! shaderSupported )
			{
				glPopMatrix();
				CHECK_ERR;
			}

			#undef C
			#undef V
			#undef N
			#undef T0
			#undef T1
			E_RELEASE(pos);
			E_RELEASE(nor);
			E_RELEASE(col);
			E_RELEASE(uv0);
			E_RELEASE(uv1);
			return 1; 
		}

		uint _blendMode( uint e6_state )
		{
			uint mode = 0;
			switch( e6_state ) 
			{
				case e6::BM_ZERO : 
					mode = GL_ZERO; break;
				case e6::BM_ONE: 
					mode = GL_ONE; break;		// (1, 1, 1, 1 )  
				case e6::BM_SRCCOLOR: 
					mode = GL_SRC_COLOR; break;			// (Rs, Gs, Bs, As )  
				case e6::BM_INVSRCCOLOR: 
					mode = GL_ONE_MINUS_SRC_COLOR; break;	// (1-Rs, 1-Gs, 1-Bs, 1-As)  
				case e6::BM_SRCALPHA: 
					mode = GL_SRC_ALPHA; break;		// (As, As, As, As )  
				case e6::BM_INVSRCALPHA: 
					mode = GL_ONE_MINUS_SRC_ALPHA; break;		// (1-As, 1-As, 1-As, 1-As)  
				case e6::BM_DESTALPHA: 
					mode = GL_DST_ALPHA; break;		// (Ad, Ad, Ad, Ad )  
				case e6::BM_INVDESTALPHA:  
					mode = GL_ONE_MINUS_DST_ALPHA; break;	// (1-Ad, 1-Ad, 1-Ad, 1-Ad)  
				case e6::BM_DESTCOLOR:  
					mode = GL_DST_COLOR; break;	// (Rd, Gd, Bd, Ad )  
				case e6::BM_INVDESTCOLOR: 
					mode = GL_ONE_MINUS_DST_COLOR; break;	// (1-Rd, 1-Gd, 1-Bd, 1-Ad)  
				//~ case e6::BM_SRCALPHASAT:  
					//~ mode = GL_SRCALPHASAT; break;		// (f, f, f, 1 ) f = min (As, 1-Ad)  
				default: assert(!"unknown type!");  break;
			}
			return mode;
		}

		virtual uint _setRenderState( uint e6_key, uint value )
		{
			//~ printf("gl render flag  %d %d\n", key,value );
			switch( e6_key ) 
			{
				case e6::RF_NONE: 
					break;
				case e6::RF_TTL:
					break;
				case e6::RF_WIRE:
					glPolygonMode(GL_BACK, value ? GL_LINE : GL_FILL);
					glPolygonMode(GL_FRONT,value ? GL_LINE : GL_FILL);
					break;
				case e6::RF_TEXTURE:
					CHANGE_STATE( GL_TEXTURE_2D, value );
					break;
				case e6::RF_LIGHTING:
					CHANGE_STATE( GL_LIGHTING, value );
					break;
				case e6::RF_CULL:
					CHANGE_STATE( GL_CULL_FACE, value );
					break;
				case e6::RF_CCW:
					glCullFace( value ? GL_BACK : GL_FRONT  );
					break;
				case e6::RF_CLEAR_COLOR: 
					break;
				case e6::RF_SHADE: 
					glShadeModel( value? GL_SMOOTH : GL_FLAT );
					break;
				case e6::RF_SHOT:
				{
					static int n = 0;
					char fn[200];
					//char * s = (value?reinterpret_cast<char*>(value):"frame");
					//sprintf( fn, "%s_%d.ppm", s, n++ );
					sprintf( fn, "frame_%d.ppm", n++ );
					float vp[4];
					glGetFloatv( GL_VIEWPORT, vp );
					RGL::screenShot(fn, (int)vp[2], (int)vp[3]);
					break;
				}
				case e6::RF_SRCBLEND:
				{
					bool doBlend = ( value != e6::BM_ZERO && stateBlendDst != e6::BM_ONE );
					CHANGE_STATE( GL_BLEND, doBlend );
					glBlendFunc( _blendMode(value), _blendMode(stateBlendDst) );
					break;
				}
				case e6::RF_DSTBLEND:
				{
					bool doBlend = ( stateBlendSrc != e6::BM_ZERO &&  value == e6::BM_ONE );
					CHANGE_STATE( GL_BLEND, doBlend );
					glBlendFunc( _blendMode(stateBlendSrc), _blendMode(value) );
					break;
				}
				default:
					return 0;   
			}
			CHECK_ERR;
			return true;
		}

		virtual uint setRenderState( uint key, uint value )
		{
			//~ printf("gl render flag  %d %d\n", key,value );
			uint r = CBaseRendererGL::set(key,value);
			if ( ! r )
				return 0;
			return _setRenderState( key, value );
		}

		
		virtual uint setVertexShaderConstant( uint i, const float4 & v )
		{
			RGL::UniformInfo * ui = RGL::uniformInfo(i);
			if ( ui )
			{
				for ( int i=0; i<4; i++ )
					ui->val[i] = v[i];
				return 1;
			}
			return 0;
		}
		virtual uint setPixelShaderConstant( uint i, const float4 & v ) 
		{
			RGL::UniformInfo * ui = RGL::uniformInfo(i);
			if ( ui )
			{
				for ( int i=0; i<4; i++ )
					ui->val[i] = v[i];
				return 1;
			}

			return 0;
		}

		virtual uint end3D()
		{
			glPopMatrix();
			if ( last_ti )
			{
				RGL::flushRenderTarget( last_ti, last_ti_w, last_ti_h );
				CHECK_ERR;
				
				clear();

				last_ti = 0;
			}
			return 1;
		}

		virtual uint get( uint what )
		{
			uint r = CBaseRendererGL::get(what);
			if ( r!=~0 )
				return r;
			switch( what )
			{
				//~ case e6::RF_CULL: return doCull; 
				//~ case e6::RF_CCW: return doCCW; 
				//~ case e6::RF_WIRE: return doWire; 
				//~ case e6::RF_SHADE: return shadeMode; 
				//~ case e6::RF_TEXTURE: return doTexture; 
				//~ case e6::RF_LIGHTING: return doLighting; 
				//~ case e6::RF_ALPHA: return doAlpha; 
				//~ case e6::RF_ALPHA_TEST: return doAlphaTest; 
				//~ case e6::RF_ZWRITE: return doZWrite; 
				//~ case e6::RF_ZTEST: return doZTest; 
				//~ case e6::RF_ZDEPTH: return zBufferDepth; 
				//~ case e6::RF_SRCBLEND: return stateBlendSrc; 
				//~ case e6::RF_DSTBLEND: return stateBlendDst; 
				//~ case e6::RF_CLEAR_COLOR: return bgColor; 
				//~ case e6::RF_CLEAR_BUFFER: return doClearBackBuffer;
			case e6::RF_NUM_HW_TEX: return _tex.cache.size(); 
			//case e6::RF_NUM_HW_VB:  return _pro.cache.size(); 
			case e6::RF_NUM_HW_VSH: return _pro.cache.size(); 
			case e6::RF_NUM_HW_PSH: return _pro._sha.cache.size(); 
			}
			return 0;
		}

		virtual uint captureFrontBuffer( const char * fileName )
		{
			float vp[4];
			glGetFloatv( GL_VIEWPORT, vp );
			uint ok = RGL::screenShot( (char*)fileName, (int)vp[2], (int)vp[3] );
			return ok;
		}
		//void _setBlend( int mode ) 
		//{
		//	BlendingState &b = _blends[mode];

		//	CHANGE_STATE(GL_BLEND,b.blend);
		//	glBlendFunc(b.blend_l, b.blend_r);

		//	CHANGE_STATE(GL_DEPTH_TEST, b.depth);
		//	glDepthMask(b.depth_m);
		//	glDepthFunc(b.depth_f);

		//	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, b.env);
		//	CHECK_ERR;
		//}


	}; // RendererGL

}; // RGL






extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Renderer",		"RendererGL",		RGL::CRenderer::createSingleton, RGL::CRenderer::classRef		},
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
	mv->modVersion = ( "RendererGL 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
