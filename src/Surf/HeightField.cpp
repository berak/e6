#include "../e6/e6.h"
#include "../e6/e6_sys.h"
#include "../e6/e6_math.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../Core/Core.h"
#include "Surf.h"


using e6::uint;
using e6::float3;
using e6::float4;
using e6::ClassInfo;


namespace Surf
{
	uint check_ref( e6::Base * base )
	{
		if ( ! base ) return 0;
		uint r = base->addref();
		r = base->release();
		return r;
	}
	struct CHeightField : e6::Class< HeightField, CHeightField >
	{
		float w,h;
		float zScale;
		uint dx,dy;
		float buffer[128][128];
		float3 norms[128][128];
		Core::Texture * tex;
		
		CHeightField()
			: w(0)
			, h(0)
			, dx(0)
			, dy(0)
			, tex(0)
			, zScale( 1.0/16.0 )
		{}
			
		~CHeightField()
		{
			$();
			E_RELEASE( tex );
		}

			
		float _h( float x, float y, float t )
		{
			return ( sin(0.05*t+0.036*x)+sin(0.014*x)*sin(0.071*y*t)-sin(0.1073*y)-sin(0.063*(t+w-y*x)) );
		}
		void _h()
		{
			static float t = 0;
			t += 0.017315;
			float z  = 5;
			for ( uint i=0; i<dx; i++ )
			for ( uint j=0; j<dy; j++ )
			{
				buffer[i][j] = _h(i,j,t) * z;
			}
		}
		void _htex()
		{
			//~ $();
			static float t = 0;
			t += 0.017315;
			E_ASSERT( tex );
			Core::Buffer * b = tex->getLevel(0);
			float tw = tex->width();
			float th = tex->height();
			float tdx = tw / dx; 
			float tdy = th / dy; 
			float cScale = 1.0f / 255.0f;
			//~ printf( "[%-4.2f][%2.2f %2.2f][%2.3f %2.3f]\n",t,tw,th,tdx,tdy); 
			uint maxPix = tw*th;
			uint *pixel = 0;
			uint ok = b->lock( (void**)&pixel, 0 );
			if ( ok ) 
			for ( uint i=0; i<dx; i++ )
			for ( uint j=0; j<dy; j++ )
			{
				uint ui = (i * tdx);
				//~ E_ASSERT( ui < tw );
				uint uj = (j * tdy);
				//~ E_ASSERT( uj < th );
				uint idx = ui + uj*tw;
				E_ASSERT( idx < maxPix );
				uint p = pixel[ idx ];
				buffer[i][j] = float((p>>8)&0xff) * zScale;
				//~ norms[i][j] = float3(float((p>>16)&0xff)*cScale, 1, float((p)&0xff)*cScale);
				//~ norms[i][j].normalize();
			}
			b->unlock();
			E_RELEASE( b );
		}
		virtual uint setTiling( uint nTiles, uint w, uint h, uint divX, uint divZ ) 
		{
			//std::cout << "setTiling " << w << ", " << h << ", " << divX << "; " << dixZ << "\n" ;
			//~ $();
			this->w = w;
			this->h = h;
			this->dx = divX;
			this->dy = divZ;
			return 1;
		}
		virtual uint setTexture( Core::Texture * t, float pixelRatio ) 
		{
			//~ $();
			E_ADDREF(t);
			E_RELEASE(this->tex);
			this->tex = t;
			this->zScale = pixelRatio;
			return (t!=0);
		}
		virtual uint update( Core::Mesh * mesh )
		{
			//~ uint _nm = check_ref( mesh );
			//~ $();
			E_ASSERT( mesh );
			
			if ( tex )
			{
				_htex();
			}
			else
			{
				_h();
			}

			E_ADDREF( mesh );
			mesh->setup( 6, (dx-1)*(dy-1)*3*2 );
			Core::VertexFeature * pos = mesh->getFeature( e6::VF_POS );
			Core::VertexFeature * uv0 = mesh->getFeature( e6::VF_UV0 );
			Core::VertexFeature * nor = mesh->getFeature( e6::VF_NOR );
			float * p = (float*)pos->getElement(0); 
			float * n = (float*)nor->getElement(0); 
			float * t = (float*)uv0->getElement(0); 
			float sx = w / dx;
			float sy = h / dy;
			float tx = 1.0 / dx;
			float ty = 1.0 / dy;
			for ( uint i=0; i<dx-1; i++ )
			for ( uint j=0; j<dy-1; j++ )
			{
				#define _V(i,j)	\
					*p++ = (i)*sx; \
					*p++ = buffer[(i)][(j)]; \
					*p++ = (j)*sy; \
					*t++ = (i)*tx; \
					*t++ = (j)*ty; 
					//~ *n++ = norms[i][j].x; \
					//~ *n++ = norms[i][j].y; \
					//~ *n++ = norms[i][j].z; 
			// cw:
				//~ _V(i,j);
				//~ _V(i,j+1);
				//~ _V(i+1,j+1);
				//~ _V(i+1,j+1);
				//~ _V(i+1,j);
				//~ _V(i,j);
			// ccw:
				_V(i,j);
				_V(i+1,j);
				_V(i+1,j+1);
				_V(i+1,j+1);
				_V(i,j+1);
				_V(i,j);
				
				#undef _V
			}
			E_RELEASE( pos );
			E_RELEASE( uv0 );

			mesh->recalcNormals(0);
			//~ uint _nm2 = check_ref(mesh);
			E_RELEASE( mesh );
			//~ printf( "mesh %d %d\n", _nm, _nm2 );
			return 1;
		}
		
	}; // HeightField

} // Surf



ClassInfo ClassInfo_HeightField =
{	"Surf.HeightField",	"Surf",	Surf::CHeightField::createInterface,Surf::CHeightField::classRef	};

