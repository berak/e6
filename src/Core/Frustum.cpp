#include "Frustum.h"

namespace Core
{

	void Frustum::setup( const float4x4 &clipMat )
	{    
		enum BoxSide
		{
			RIGHT	= 0,		
			LEFT	= 1,		
			BOTTOM	= 2,		
			TOP		= 3,		
			BACK	= 4,		
			FRONT	= 5			
		}; 

		float * clip = (float*) &(clipMat);

		float3 n;
		float l;

		n = float3( 
			clip[ 3] + clip[ 0],
			clip[ 7] + clip[ 4],
			clip[11] + clip[ 8] );
		l = n.length();
		l = ( fabsf(l)> 0.0001 ) ? 1.0f/l : 1.0f;
		frust[LEFT] = Plane( n*l , l * ( clip[15] + clip[12] ) );

		n = float3( 
			clip[ 3] - clip[ 0],
			clip[ 7] - clip[ 4],
			clip[11] - clip[ 8] );
		l = n.length();
		l = ( fabsf(l)> 0.0001 ) ? 1.0f/l : 1.0f;
		frust[RIGHT] = Plane( n*l , l * ( clip[15] - clip[12] ) );

		n = float3( 
			clip[ 3] + clip[ 1],
			clip[ 7] + clip[ 5],
			clip[11] + clip[ 9] );
		l = n.length();
		l = ( fabsf(l)> 0.0001 ) ? 1.0f/l : 1.0f;
		frust[BOTTOM] = Plane( n*l, l * (clip[15] + clip[13]) );

		n = float3( 
			clip[ 3] - clip[ 1],
			clip[ 7] - clip[ 5],
			clip[11] - clip[ 9] );
		l = n.length();
		l = ( fabsf(l)> 0.0001 ) ? 1.0f/l : 1.0f;
		frust[TOP] = Plane( n*l, l * (clip[15] - clip[13]) );

		n = float3( 
			clip[ 3] + clip[ 2],
			clip[ 7] + clip[ 6],
			clip[11] + clip[10] );
		l = n.length();
		l = ( fabsf(l)> 0.0001 ) ? 1.0f/l : 1.0f;
		frust[FRONT] = Plane( n * l, l * (clip[15] + clip[14]) );

		n = float3( 
			clip[ 3] - clip[ 2],
			clip[ 7] - clip[ 6],
			clip[11] - clip[10] );
		l = n.length();
		l = ( fabsf(l)> 0.0001 ) ? 1.0f/l : 1.0f;
		frust[BACK] = Plane( n*l, l * (clip[15] - clip[14]) );
	}

	/*
	bool Frustum::intersectBBox( const BBox & b ) 
	{
		float3 m( b.inf );
		float3 M( b.sup );
		float3 e = ( M - m ) * 0.5f;
		float3 p = m + e;
		unsigned int o = 0;

		return intersectAABB( p, e, o );
	}
	*/

	bool Frustum::intersectAABB( const float3 & pos, const float3 & e,
												uint & outClipMask, uint inClipMask )
	{
		const Plane * plane = frust;
		uint p  = 0;
		uint mk = 1;
		outClipMask = 0; // init outclip mask
	    
		// loop while there are active planes..
		while ( (mk <= inClipMask) && (p < 6) )
		{
			// if clip plane is active...
			if ( inClipMask & mk )
			{
				const float3 & n = plane[p].normal();
				
				float NP = e.getX() * fabs( n.getX() ) 
							 + e.getY() * fabs( n.getY() ) 
							 + e.getZ() * fabs( n.getZ() );

				float MP = plane[p].distance(pos); //m.X*p->X+m.y*p->y+m.z*p->z+p->w;

				if ( (MP + NP) < 0.0f )
				{
					return false; // behind clip plane
				}

				if ( (MP - NP) < 0.0f )
				{
					outClipMask |= mk;
				}
			}

			mk += mk;
			p++; // next plane
		}

		return true; // AABB intersects frustum
	}


	bool Frustum::intersectSphere( const float3 & center, const float radius ) 
	{
		for ( int i = 0; i < 6; i++ )
		{
			if ( frust[i].distance( center ) <= - radius )
			{
				return false;
			}
		}

		return true;
	}

} // Core