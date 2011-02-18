
#include "../e6/e6_impl.h"
#include "../e6/sys/w32/e6_thread.h"
#include "Video.h"
#include "FGraph.h"

extern "C"
{
	//
	// ! libar was compiled with	AR_PIXEL_FORMAT_BGR !
	//
	#include "AR/ar.h"
};




using e6::uint;
using e6::ClassInfo;



namespace Video
{
	e6::sys::CriticalSection crybaby;

	////
	//// camera_para.dat
	//// 136 bytes
	////
	//static char cam_dat[] = 
	//{
	//   0x00, 0x00, 0x02, 0x80, 0x00, 0x00, 0x01, 0xe0, 0x40, 0x85, 0xe7, 0x9c, 0x9c, 0x75, 0xd2, 0x47, 
	//   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x73, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 
	//   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	//   0x40, 0x86, 0xb0, 0xc0, 0xe2, 0x4f, 0x8e, 0x96, 0x40, 0x6e, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 
	//   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	//   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	//   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x73, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 
	//   0x40, 0x70, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3a, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 
	//   0x3f, 0xf0, 0x34, 0x40, 0x2e, 0x71, 0x21, 0x0c
	//};
	extern Video::FilterGraph * createGraph();
	
	//
	//! shared data block for detect callback & filtergraph
	//
	struct ArtData
	{
		int width, height, thresh;
		double pattSize; // in mm
		int frm;

		uint nPatterns;
		struct Marker
		{
			ARMarkerInfo  ar;
			double        transform[3][4];
			e6::float3    p;
			e6::float3    r;
			e6::float4    q;
			int           framesLost;
		} marker[64];

		ArtData()
			: width(0)
			, height(0)
			, thresh(40)
			, nPatterns(0)
			, frm(0)
			, pattSize(60)
		{
			memset( marker, 0, 64*sizeof(ArtData::Marker) );
		}
	};

	//
	// detector callback
	// this is refcounted by the videosampler.
	// had to be in a separate class to avoid cyclic refs.
	//
	struct CArtDetect : Grabber
	{
		// shared
		ArtData & art;

		CArtDetect( ArtData & a )
			: art(a)
		{
		}

		virtual ~CArtDetect()
		{
		}

		virtual int processBGR( void * pixel, int w, int h ) 
		{	
			e6::sys::Lock lock( &crybaby );

			ARUint8         *dataPtr = (ARUint8*)pixel;
			ARMarkerInfo    *marker_info;
			int             marker_num = 0;
			double          patt_center[2] = {0.0, 0.0};;

			// this is the earliest moment to access the video size....
			if ( w != art.width || h != art.height )
			{
				art.width  = w;
				art.height = h;

 				ARParam cparam, wparam;
				// try some places for cparams file:
				if( arParamLoad("camera_para.dat", 1, &wparam) < 0 ) {
					if( arParamLoad("bin/camera_para.dat", 1, &wparam) < 0 ) {
						if( arParamLoad("../camera_para.dat", 1, &wparam) < 0 ) {
							printf("Camera parameter load error !!\n");
							return 0;
						}
					}
				}
				arParamChangeSize( &wparam, w,h, &cparam );
				arInitCparam( &cparam );
			}

			// detect the markers in the video frame 
			if( arDetectMarker(dataPtr, art.thresh, &marker_info, &marker_num) < 0 )
			{
				printf( __FUNCTION__ " ardetectmarker FAILED !\n");
				return 0;
			}

			for( uint i=0; i<art.nPatterns; i++ ) 
			{
				art.marker[i].framesLost ++;
			}

			int nDetected = 0;
			if ( marker_num )
			{
				for( int j = 0; j < marker_num; j++ ) 
				{
					int id =  marker_info[j].id;
					if ( id < 0 ) // what's this ??
						continue;
					if ( (uint)id >= art.nPatterns ) // ??
						continue;

					// cache transform data:

					ArtData::Marker & amark = art.marker[id];
					memcpy( &(amark.ar), &(marker_info[j]), sizeof(ARMarkerInfo) );

					arGetTransMat(&(marker_info[j]), patt_center, art.pattSize, amark.transform);

					double q[4], r[3], p[3];// = 
					//	{ amark.transform[0][3],amark.transform[1][3],amark.transform[2][3] };

					// repack to 3x3 for getangle
					double m[3][3] = { 
						{ amark.transform[0][0],amark.transform[0][1],amark.transform[0][2] },
						{ amark.transform[1][0],amark.transform[1][1],amark.transform[1][2] },
						{ amark.transform[2][0],amark.transform[2][1],amark.transform[2][2] } };
					int res = arGetAngle( m, &r[0], &r[1], &r[2] );
					if ( res > -1 )
					{
						amark.r = e6::float3( r[0], -r[1], r[2] ); /// -z ??
					}

					res = arUtilMat2QuatPos( amark.transform, q, p );

					if ( res > -1 )
					{
						amark.p = e6::float3( p[0], p[1], p[2] ); /// -z ??
						amark.q = e6::float4( q[0], q[1], q[2], q[3] ); 
					}
					else
					{
						amark.p = e6::float3( amark.transform[0][3],amark.transform[1][3],amark.transform[2][3] ); /// -z ??
					}

					// make visible:
					amark.framesLost = 0;
					nDetected ++;
				}
			}

			if ( nDetected )
			{
				FGraph::blink(pixel,w);
			}
			art.frm ++;
			return 1;
		}
	};



	struct CArthur
		: public e6::CName< Video::ArtDetector, CArthur >
	{
		// owned
		ArtData art;

		CArtDetect * detect;
		FilterGraph * graph;

		CArthur()
			: art()
			, detect( new CArtDetect( art ) )
			, graph( createGraph() )
		{
		}

		virtual ~CArthur()
		{
			for ( uint i=0; i<art.nPatterns; i++ )
			{
				arFreePatt( i );
			}
			
			if ( graph )
			{
				graph->stop();
			}	
			E_DELETE( detect );
			E_RELEASE( graph );
		}

		virtual int   getThreshold() const { return art.thresh; }
		virtual void  setThreshold(int t) { art.thresh = t; }

		virtual void  setPatternSize( float w ) { art.pattSize = w; }
		virtual float getPatternSize() const { return art.pattSize; }

		virtual uint  addPattern( const char *fileName ) 
		{		
			int n = arLoadPatt( fileName );
			if ( n < 0 ) 
			{
				printf( " COULD NOT LOAD '%s'!\n", fileName ); 
				n = 0;
				return 0;
			}
			art.nPatterns ++;
			return 1;
		}

		virtual uint  numPatterns() const { return art.nPatterns; }
		virtual uint  getPatternLost( uint i ) const { return (i<art.nPatterns) ? art.marker[i].framesLost : 0; }
		virtual uint  getPatternOrient( uint i ) const { return (i<art.nPatterns) ? art.marker[i].ar.dir : 0; }
		virtual float getPatternX( uint i ) const { return (i<art.nPatterns) ? art.marker[i].ar.pos[0] : 0; }
		virtual float getPatternY( uint i ) const { return (i<art.nPatterns) ? art.marker[i].ar.pos[1] : 0; }
		virtual e6::float4 getPatternQuat( uint i ) const 
		{
			e6::sys::Lock lock( &crybaby );
			if (i<art.nPatterns)
			{
				return art.marker[i].q;
			}
			return  e6::float4(0,0,0,0);
		}
		virtual e6::float3 getPatternRot( uint i ) const 
		{
			e6::sys::Lock lock( &crybaby );
			if (i<art.nPatterns)
			{
				return art.marker[i].r;
			}
			return  e6::float3(0,0,0);
		}
		virtual e6::float3 getPatternPos( uint i ) const 
		{
			e6::sys::Lock lock( &crybaby );
			if (i<art.nPatterns)
			{
				return art.marker[i].p;
			}
			return  e6::float3(0,0,0);
		}
		virtual uint  start( const char *inp="Camera", const char *outp="VideoRenderer" )
		{
			//if ( graph->build( "Camera", grabber, "NullRenderer"  ) )
			if ( graph->build( inp, detect, outp  ) )
			{
				if ( graph->start() )
				{
					return 1;
				}
			}
			return 0;
		}


	}; // CArthur
	
}; // video


ClassInfo ClassInfoArthur =
{	"Video.ArtDetector",	 "Video",	Video::CArthur::createInterface, Video::CArthur::classRef	};

