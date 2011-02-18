#include "e6_math.h"
#include <stdio.h>

namespace e6
{
	float3 operator * ( const float4x4 & m, const float3 & v ) 
	{
		return float3(
			m.m00 * v.x + m.m10 * v.y + m.m20 * v.z + m.m30,
			m.m01 * v.x + m.m11 * v.y + m.m21 * v.z + m.m31,
			m.m02 * v.x + m.m12 * v.y + m.m22 * v.z + m.m32
			);
	}
	float3 operator * ( const float3 & v, const float4x4 & m ) 
	{
		return float3(
			m.m00 * v.x + m.m01 * v.y + m.m02 * v.z + m.m03,
			m.m10 * v.x + m.m11 * v.y + m.m12 * v.z + m.m13,
			m.m20 * v.x + m.m21 * v.y + m.m22 * v.z + m.m23
			);
	}		


	void print( const float2 & v )
	{
		printf( "%3.3f\t%3.3f\n", v.x,v.y ); 
	}
	void print( const float3 & v )
	{
		printf( "%3.3f\t%3.3f\t%3.3f\n", v.x,v.y,v.z ); 
	}
	void print( const float4 & v )
	{
		printf( "%3.3f\t%3.3f\t%3.3f\t%3.3f\n", v.x,v.y,v.z,v.w ); 
	}
	void print( const float4x4 & m )
	{
		printf( "%3.3f\t%3.3f\t%3.3f\t%3.3f\n", m.m00, m.m01, m.m02, m.m03 ); 
		printf( "%3.3f\t%3.3f\t%3.3f\t%3.3f\n", m.m10, m.m11, m.m12, m.m13 ); 
		printf( "%3.3f\t%3.3f\t%3.3f\t%3.3f\n", m.m20, m.m21, m.m22, m.m23 ); 
		printf( "%3.3f\t%3.3f\t%3.3f\t%3.3f\n", m.m30, m.m31, m.m32, m.m33 ); 
	}

	const char *toString( const float3 & v )
	{
		return toString( (float*)&(v), 3 );
	}

	float3 stringToFloat3( const char *s )
	{
		float3 v;
		sscanf( s, "%f %f %f", &v[0], &v[1], &v[2] );
		return v;
	}
	const char *toString( const float4 & v )
	{
		return toString( (float*)&(v), 4 );
	}

	float4 stringToFloat4( const char *s )
	{
		float4 v;
		sscanf( s, "%f %f %f %f", &v[0], &v[1], &v[2], &v[3] );
		return v;
	}

	//float4x4 stringToFloat4x4( const char *s )
	//{
	//	float4x4 m;
	//	float * v = m;
	//	sscanf( s, "%f %f %f %f", v  ,  v+1,  v+2,  v+3 );
	//	sscanf( s, "%f %f %f %f", v+4,  v+5.  v+6,  v+7  );
	//	sscanf( s, "%f %f %f %f", v+8,  v+9,  v+10, v+11 );
	//	sscanf( s, "%f %f %f %f", v+12, v+13, v+14, v+15 );
	//	return m;
	//}




}; // e6


