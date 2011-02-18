#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../Video/Video.h"

#include <conio.h>


using e6::uint;


//
// run face detector test:
//

int main(int argc, char **argv)
{
	e6::Engine * e = e6::CreateEngine();	
	Video::FilterGraph * graph = (Video::FilterGraph *)e->createInterface("Video","Video.FilterGraph");
	if ( graph )
	{
		Video::FaceDetect * cb = (Video::FaceDetect *)e->createInterface("Video","Video.FaceDetect");
		char * src = ( argc>1 ? argv[1] : "Camera" );
		//char * ren = ( argc>2 ? argv[2] : "NullRenderer" ); // "Video Renderer" opens a window ( note the blank ! )
		char * ren = ( argc>2 ? argv[2] : "Video Renderer" ); 
		if ( graph->build( src, cb, ren  ) )
		{
			if ( graph->start() )
			{
				uint frame = 0;
				while ( ! kbhit() )
				{
					int nf = cb->numFacesDetected();
					if ( ! nf ) continue;

					for (int i=0; i<nf; i++)
					{
						printf(
							"%4d\t%d\tx:%2.3f\ty:%2.3f\tsize:%2.3f\n", 
							frame, i, cb->getFaceX(i), cb->getFaceY(i), cb->getFaceW(i) 
						);
					}
					//Sleep(100);
					frame ++;
				}	
				graph->stop();
			}
		}

		E_RELEASE(graph);	
		E_DELETE(cb);	
	}
	E_RELEASE(e);	
	return 0;
}



//
//struct CGrabber 
//	: e6::Class< Video::Grabber, CGrabber >
//{
//	const static uint threshold = 120;
//	const static uint numQuadrants = 9;
//
//	uint *cp;
//	uint quad[numQuadrants][numQuadrants];
//	float qiw,qih;
//	uint mx,my, mv;
//
//	CGrabber() 
//		: cp(0), qiw(0), qih(0), mx(0), my(0), mv(60) 
//	{  
//		memset(quad,0,numQuadrants*numQuadrants*sizeof(uint));  
//	}
//
//	~CGrabber() 
//	{
//		E_DELETEA(cp); 
//	}
//
//	virtual uint processRGB( void * pixel, uint w, uint h ) 
//	{	
//		if ( ! cp )
//		{
//			cp = new uint[ w*h ];
//			memset( cp, 0, w*h*sizeof(uint) );
//			qiw = float(numQuadrants) / w;
//			qih = float(numQuadrants) / h;
//		}
//
//		//memset(quad,0,4*4*sizeof(uint));  
//		for ( uint j=0; j<numQuadrants; j++ )
//		{
//			for ( uint i=0; i<numQuadrants; i++ )
//			{
//				quad[ j ][ i ] = 0;
//			}
//		}
//
//
//		struct _rgb { unsigned char b,g,r; };
//		_rgb * pix = (_rgb*)pixel;
//
//		for ( uint j=0; j<h-1; j++ )
//		{
//			uint wj  = w*(j  );
//			uint wj1 = w*(j+1);
//			uint qj  = uint(j*qih);
//			for ( uint i=0; i<w-1; i++ )
//			{
//				uint p = pix[ (i  )+wj  ].g
//					   + pix[ (i  )+wj  ].g
//					   + pix[ (i  )+wj1 ].g
//					   + pix[ (i+1)+wj  ].g
//					   + pix[ (i+1)+wj1 ].g;
//				if ( abs(p - cp[i+wj]) > threshold )
//				{
//					pix[i+wj].b = 255;
//					quad[ qj ][ int(i*qiw) ] ++;
//				}
//				//else
//				//	pix[i+wj].r = pix[i+wj].g = pix[i+wj].b = 0;
//				cp[i+wj] = (p + cp[i+wj]) / 2;
//			}
//		}
//		uint mi=2000,mj=2000;
//		mv = float(mv) * 0.95f;
//		for ( uint j=0; j<numQuadrants; j++ )
//		{
//			for ( uint i=0; i<numQuadrants; i++ )
//			{
//				if ( mv < quad[ j ][ i ] )
//				{
//					mi = i;
//					mj = j;
//					mv = quad[ j ][ i ];
//				}
//			}
//		}
//		mx = mi;//(mx+mi)/2;//mi;//
//		my = mj;//(my+mj)/2; //mj;//
//		//if ( mj > my ) my++;
//		//if ( mj < my ) my--;
//		//if ( mi > mx ) mx++;
//		//if ( mi < mx ) mx--;
//
//		uint pSize = 8;
//		uint pw = numQuadrants * pSize;
//		for ( uint j=0; j<pw; j++ )
//		{
//			for ( uint i=0; i<pw; i++ )
//			{
//				uint id = (w-10-pw+i)+w*(j+10);
//				uint is = i/pSize;
//				uint js = j/pSize;
//				uint v = quad[ js ][ is ] / 16;
//				if ( (is==mx)&&(js==my) )
//				{
//					pix[id].r = v*8;
//					pix[id].g = 0;
//					pix[id].b = 0;
//				}
//				else
//				{
//					pix[id].r = v;
//					pix[id].g = v;
//					pix[id].b = v;
//				}
//			}
//		}
//
//		blink( pixel, w );
//		//transform( pixel, w, h );
//		//printf( "hello, (%i / %i).\n", w, h );
//		return 1;
//	}
//};

//int main(int argc, char **argv)
//{
//	e6::Engine * e = e6::CreateEngine();	
//	Video::FilterGraph * graph = (Video::FilterGraph *)e->createInterface("Video","Video.FilterGraph");
//	if ( graph )
//	{
//		Video::Quadrant * cb = (Video::Quadrant *)e->createInterface("Video","Video.Quadrant");
//		char * src = ( argc>1 ? argv[1] : "Camera" );
//		//char * ren = ( argc>2 ? argv[2] : "NullRenderer" ); // "Video Renderer" opens a window ( note the blank ! )
//		char * ren = ( argc>2 ? argv[2] : "Video Renderer" ); 
//		if ( graph->build( src, cb, ren  ) )
//		{
//			if ( graph->start() )
//			{
//				while ( ! kbhit() )
//				{
//					printf( "%2.2f %2.2f %2.2f \n", cb->peakX(), cb->peakY(), cb->peakV() );
//					//Sleep(100);
//				}	
//				graph->stop();
//			}
//		}
//
//		E_RELEASE(graph);	
//		E_DELETE(cb);	
//	}
//	E_RELEASE(e);	
//	return 0;
//}
//


//
//int main(int argc, char **argv)
//{
//	e6::Engine * e = e6::CreateEngine();	
//	TimeLine::Filter * cb = (TimeLine::Filter *)e->createInterface("Video","Video.QuadrantFilter");
//	while ( ! kbhit() )
//	{
//		for ( uint i=0; i<cb->numOutputs(); i++ )
//		{
//			printf( "%s(%2.2f) ", cb->getOutputName(i), cb->getOutput(i) );
//		}
//		printf( "\n" );
//		//Sleep(100);
//	}	
//	E_DELETE(cb);	
//	E_RELEASE(e);	
//	return 0;
//}





//  twain:

//
//int main(int argc, char **argv)
//{
//	e6::Engine * e = e6::CreateEngine();	
//	Video::TwainSource * ts = (Video::TwainSource *)e->createInterface("Video","Video.TwainSource");
//	if ( ts )
//	{
//		uint r = ts->capture( "Camera", 0 );
//		if ( ! r  )
//		{
//			printf( "TwainSource capture failed !\n" );
//		}
//	}
//	E_DELETE(ts);	
//	E_RELEASE(e);	
//	return 0;
//}
