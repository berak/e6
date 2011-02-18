
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "Fgraph.h"
#include "Video.h"
//#include "twaini.h"
#include "../Core/Core.h"
#include "fdlib.h"
#include <qedit.h>
#include <math.h>
//#include <stdio.h>

// Warnung	1	warning C4996: 'stricmp' wurde als veraltet deklariert	e:\code\e6\src\video\video.cpp	244
#pragma warning (disable:4996)


using e6::uint;
using e6::ClassInfo;


namespace Video
{

	//// red rec led:
	//void blink ( void *pData, uint w, uint red=255 ) 
	//{
	//	if (  red < 5 ) return ;

	//    static int pt[40][2] = 
	//	{
	//                             {3,0}, {4,0},
	//                      {2,1}, {3,1}, {4,1}, {5,1},
	//               {1,2}, {2,2}, {3,2}, {4,2}, {5,2}, {6,2},
	//        {0,3}, {1,3}, {2,3}, {3,3}, {4,3}, {5,3}, {6,3}, {7,3},
	//        {0,4}, {1,4}, {2,4}, {3,4}, {4,4}, {5,4}, {6,4}, {7,4},
	//               {1,5}, {2,5}, {3,5}, {4,5}, {5,5}, {6,5},
	//                      {2,6}, {3,6}, {4,6}, {5,6},
	//                             {3,7}, {4,7},
	//    };
	//	
	//	struct _bgr {unsigned char b,g,r;} *prgb = (_bgr*) pData;

	//    for ( int n=0; n<40; n++ ) 
	//	{
	//        int k = pt[n][0] + pt[n][1] * w;
	//        prgb[k].r = red;
	//    }
	//}

	//void printGUID( const GUID & ng2 )
	//{
	//   printf_s(  "{%x08-%x04-%x04-", ng2.Data1, ng2.Data2, ng2.Data3 );
	//   for (int i = 0 ; i < 8 ; i++) {
	//	  if (i == 2)
	//		 printf_s("-");
	//	  printf_s("%02x", ng2.Data4[i]);
	//   }
	//   printf_s("}\n");
	//}




	struct CFilterGraph 
		: public e6::CName< FilterGraph, CFilterGraph >
	{
		FGraph graph;

		CFilterGraph()
		{
			_name.set( "FilterGraph" );
			graph.setup();
		}
		virtual ~CFilterGraph()
		{
			graph.clear();
		}


		virtual uint build( const char * srcName, Grabber * grab, const char * dstName )
		{
			return graph.buildGraph( srcName, grab, dstName );
		}


		virtual uint start() 
		{
			bool ok = graph.isValid();
			if ( ok )
			{
				ok = graph.play();
			}
			return ok;
		}
		virtual uint pause() 
		{
			bool ok = graph.isValid();
			if ( ok )
			{
				ok = graph.pause();
			}
			return ok;
		}
		virtual uint stop() 
		{
			bool ok = graph.isValid();
			if ( ok )
			{
				ok = graph.stop();
			}
			return ok;
		}
		virtual uint wind( float toPos ) 
		{
			bool ok = graph.isValid();
			if ( ok )
			{
				ok = graph.wind(toPos);
			}
			return ok;
		}

	};  // CFilterGraph

	Video::FilterGraph * createGraph()
	{
		return (Video::CFilterGraph *)CFilterGraph::createInterface();
	}


	struct CQuadrantGrabber : Grabber
	{
		const static uint threshold = 100;
		const static uint numQuadrants = 9; // must be odd to have a center value!
		uint *cp; // last buffer
		uint quad[numQuadrants][numQuadrants]; // scaled down matrix
		float qiw, qih; // scale factors, pixel to quad
		float mx, my, mv; // pos x,y and value of maximum
		float dx, dy; // delta pos of max;
		float lx, ly; // last pos

		CQuadrantGrabber() 
			: cp(0), qiw(0), qih(0), mx(0), my(0), mv(60), dx(0), dy(0), lx(0), ly(0) 
		{  
			memset(quad,0,numQuadrants*numQuadrants*sizeof(uint));  
		}

		~CQuadrantGrabber() 
		{
			E_DELETEA(cp); 
		}

		virtual int processBGR( void * pixel, int w, int h ) 
		{	
			if ( ! cp )
			{
				cp = new uint[ w*h ];
				memset( cp, 0, w*h*sizeof(uint) );
				qiw = float(numQuadrants) / w;
				qih = float(numQuadrants) / h;
			}

			//memset(quad,0,4*4*sizeof(uint));  
			for ( uint j=0; j<numQuadrants; j++ )
			{
				for ( uint i=0; i<numQuadrants; i++ )
				{
					quad[ j ][ i ] = 0;
				}
			}

			// sample pixelbuffer for motion:
			struct _rgb { unsigned char b,g,r; };
			_rgb * pix = (_rgb*)pixel;

			uint scan = 0;
			for ( uint j=0; j<h-1; j++ )
			{
				uint qj  = uint(j*qih);
				for ( uint i=0; i<w-1; i++ )
				{
					// sample with a little blur right/down to denoise.
					// can't use up/left, since i'm drawing blue motion there..
					uint p = pix[ i   +scan   ].g
						   + pix[ i   +scan   ].g
						   + pix[ i   +scan+w ].g
						   + pix[ i+1 +scan   ].g
						   + pix[ i+1 +scan+w ].g;

					// motion detected:
					if ( abs(int(p - cp[i+scan])) > threshold )
					{
						// increase hit quadfield:
						quad[ qj ][ int(i*qiw) ] ++;
						// visualize motion:
						pix[ i+scan ].b = 255;
					}
					// keep mean value for next iter:
					cp[i+scan] = (p + cp[i+scan]) / 2;
				}
				scan += w;
			}

			// search for peak val:
			uint mi=2000,mj=2000;

			mv = mv * 0.90f; // friction or decay
			for ( uint j=0; j<numQuadrants; j++ )
			{
				for ( uint i=0; i<numQuadrants; i++ )
				{
					if ( (uint)mv < quad[ j ][ i ] )
					{
						mi = i;
						mj = j;
						mv = quad[ j ][ i ];
					}
				}
			}
			lx = mx;
			ly = my;
			mx = mi==2000?mx:mi;//(mx+mi)/2;//mi;//
			my = mj==2000?my:mj;//(my+mj)/2; //mj;//
			dx = lx - mx;
			dy = ly - my;
			// visualize quads:
			uint pSize = 8;
			uint pw = numQuadrants * pSize;
			for ( uint j=0; j<pw; j++ )
			{
				for ( uint i=0; i<pw; i++ )
				{
					uint id = (w-10-pw+i)+w*(j+10);
					uint is = i/pSize;
					uint js = j/pSize;
					uint v = quad[ js ][ is ] / 16;
					if ( (is==mx)&&(js==my) )
					{
						pix[id].r = v*8;
						pix[id].g = 0;
						pix[id].b = 0;
					}
					else
					{
						pix[id].r = v;
						pix[id].g = v;
						pix[id].b = v;
					}
				}
			}

			return 1;
		}
		float peakX() const 
		{
			return floor(0.5f + mx - numQuadrants*0.5) / (numQuadrants*0.5);
		}
		float peakY() const 
		{
			return floor(0.5f + my - numQuadrants*0.5) / (numQuadrants*0.5);
		}
		float peakV() const 
		{
			return mv;
		}
		float peakVelX() const 
		{
			return dx;
		}
		float peakVelY() const 
		{
			return dy;
		}
	}; // CQuadrantGrabber







	struct CMotionDetector
		: e6::CName< Video::MotionDetector, CMotionDetector >
	{
		FilterGraph * graph;
		CQuadrantGrabber * grabber;

		CMotionDetector()
			: graph( createGraph() )
			//: graph( new CFilterGraph )
			, grabber( new CQuadrantGrabber )
		{
			//if ( graph->build( "Camera", grabber, "NullRenderer"  ) )
			if ( graph->build( "Camera", grabber, "VideoRenderer"  ) )
			{
				if ( graph->start() )
				{
				}
			}
		}

		~CMotionDetector()
		{
			if ( graph )
			{
				graph->stop();
			}	
			E_DELETE( grabber );
			E_RELEASE( graph );
		}

		virtual float peakX() const 
		{
			return grabber->peakX();
		}
		virtual float peakY() const 
		{
			return grabber->peakY();
		}
		virtual float peakV() const 
		{
			return grabber->peakV();
		}
		virtual float peakVelX() const 
		{
			return grabber->peakVelX();
		}
		virtual float peakVelY() const 
		{
			return grabber->peakVelY();
		}
	}; // CMotionDetector






	struct CFaceDetectGrabber : Grabber 
	{
		int threshold;
		int nDetected;
		int x[256], y[256], size[256];
		float w, h;
		unsigned char *graydata;

		CFaceDetectGrabber() 
			: threshold(0)
			, nDetected(0)
			, w(1)
			, h(1)
			, graydata(0)
		{  
		}

		~CFaceDetectGrabber() 
		{
			E_DELETEA( graydata );
		}

		virtual int getThreshold() const { return threshold; }
		virtual void setThreshold(int t) { threshold = t; }
		virtual uint numFacesDetected() const { return nDetected; }
		virtual float getFaceX( uint i ) const { return (2.0f*((x[i]-w*0.5f)/w)); }
		virtual float getFaceY( uint i ) const { return (2.0f*((y[i]-h*0.5f)/h)); }
		virtual float getFaceW( uint i ) const { return (float(size[i])/h);   }

		virtual int processBGR( void * pixel, int w, int h ) 
		{	
			this->w = w;
			this->h = h;

			//nDetected = 0;

			// sample pixelbuffer for motion:
			struct _BGR { unsigned char b,g,r; } * pix = (_BGR*)pixel;
			if ( ! graydata )
			{
				graydata = new unsigned char[w*h];
			}
	
			for (uint i=0; i<w*h; i++)
			{
				graydata[i] = (unsigned char) ((.11*pix[i].r + .59*pix[i].g + .3*pix[i].b));
			}	
	
			fdlib_detectfaces(graydata, w, h, threshold);	
		
			nDetected = fdlib_getndetections();
			
			for (int i=0; i<nDetected; i++)
			{
				fdlib_getdetection(i, x+i, y+i, size+i);
				//printf("x:%d y:%d size:%d\n", x[i], y[i], size[i]);
			}
			
			
			// show led on detection:
			static int red = 0;
			if ( nDetected )
			{
				red = 255;
			}
			FGraph::blink( pixel, w, red );
			// slow fade out:
			red *= 0.7;;
			return 1;
		}
	}; // CFaceDetectGrabber







	//
	// scripting
	//
	struct CFaceDetector
		: e6::CName< FaceDetector, CFaceDetector >
	{
		FilterGraph * graph;
		CFaceDetectGrabber * grabber;

		CFaceDetector()
			: graph( createGraph() )
			//: graph( new CFilterGraph )
			, grabber( new CFaceDetectGrabber )
		{}


		~CFaceDetector()
		{
			if ( graph )
			{
				graph->stop();
			}	
			E_DELETE( grabber );
			E_RELEASE( graph );
		}

		virtual int getThreshold() const { return grabber->getThreshold(); }
		virtual void setThreshold(int t) { grabber->setThreshold(t); }
		virtual uint  numFacesDetected() const { return grabber->numFacesDetected(); }
		virtual float getFaceX( uint i ) const { return grabber->getFaceX(i); }
		virtual float getFaceY( uint i ) const { return grabber->getFaceY(i); }
		virtual float getFaceW( uint i ) const { return grabber->getFaceW(i); }

		virtual uint start( const char *inp="Camera", const char *outp="VideoRenderer" )
		{
			//if ( graph->build( "Camera", grabber, "NullRenderer"  ) )
			if ( graph->build( inp, grabber, outp  ) )
			{
				if ( graph->start() )
				{
					return 1;
				}
			}
			return 0;
		}

	}; // CFaceDetector



	struct CTextureGrabber : Grabber
	{
		Core::Texture * tex;

		CTextureGrabber() 
			: tex(0)
		{  
		}

		~CTextureGrabber() 
		{
			E_RELEASE(tex);
		}

		uint setTexture( const Core::Texture * t ) 
		{
			tex = const_cast<Core::Texture*>(t);
			return 1;
		}
		Core::Texture * getTexture() const 
		{
			E_ADDREF(tex);
			return tex;
		}

		virtual int processBGR( void * pixel, int w, int h ) 
		{	
			if ( ! tex ) 
				return 0;
			int tw = tex->width();
			int th = tex->height();
			if ( tw != w || th != h )
			{
				tex->alloc( e6::PF_X8B8G8R8, w, h, 1, e6::TU_VIDEO );
			}
			Core::Buffer * cbuf = tex->getLevel(0);
			if ( cbuf )
			{
				unsigned char * tpixel = 0;
				unsigned char * px = (unsigned char *)pixel;
				cbuf->lock( (void**)&tpixel );
				if ( tpixel )
				{
					for ( uint j=0; j<h; j++ )
					for ( uint i=0; i<w; i++ )
					{
						*tpixel++ = *px++;
						*tpixel++ = *px++;
						*tpixel++ = *px++;
						*tpixel++ = 255;
					}
					cbuf->unlock();
				}
				E_RELEASE( cbuf );
			}
			
			return 1;
		}
	}; // CTextureGrabber


	struct CVideoTexture 
		: e6::CName< VideoTexture, CVideoTexture >
	{
		CTextureGrabber *tex;
		FilterGraph * graph;


		CVideoTexture() 
			: graph( createGraph() )
			, tex( new CTextureGrabber )
		{  
		}

		~CVideoTexture() 
		{
			E_DELETE(tex);
			E_RELEASE(graph);
		}
		//! start filtergraph
		virtual uint start( const char *inp="Camera", const char *outp="NullRenderer" )
		{
			if ( graph->build( inp, tex, outp  ) )
			{
				if ( graph->start() )
				{
					return 1;
				}
			}
			return 0;
		}

		virtual uint setTexture( const Core::Texture * t ) 
		{
			return tex->setTexture(t);
		}
		virtual Core::Texture * getTexture() const 
		{
			return tex->getTexture();
		}

	}; // CVideoTexture



	//struct CTwainSource : e6::CName< TwainSource, CTwainSource >
	//{
	//	CTwainSource() {}
	//	~CTwainSource() {}

	//	virtual uint capture( const char * srcName, Grabber * grabber ) 
	//	{
 //           if ( TwainExists() )
 //           {
	//			HANDLE h = TwainGetImage( GetDesktopWindow() );
	//			if ( h )
	//			{
	//				return 1;
	//			}
 //           }
	//		return 0;
	//	}
	//};

} // video



extern ClassInfo ClassInfoArthur;

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Video.FilterGraph",	 "Video",	Video::CFilterGraph::createInterface,	  Video::CFilterGraph::classRef	},

		//{	"Video.Quadrant",		 "Video",	Video::CQuadrantGrabber::createInterface, Video::CQuadrantGrabber::classRef	},
		{	"Video.MotionDetector",	 "Video",	Video::CMotionDetector::createInterface, Video::CMotionDetector::classRef	},

		//{	"Video.FaceDetect",		 "Video",	Video::CFaceDetectGrabber::createInterface, Video::CFaceDetectGrabber::classRef	},
		{	"Video.FaceDetector",	 "Video",	Video::CFaceDetector::createInterface, Video::CFaceDetector::classRef	},

		{	"Video.VideoTexture",	 "Video",	Video::CVideoTexture::createInterface, Video::CVideoTexture::classRef	},
		//{	"Video.TwainSource",	 "Video",	Video::CTwainSource::createInterface, Video::CTwainSource::classRef	},

		ClassInfoArthur,

		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 5; // classses
}


#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Video 00.000.0123 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
