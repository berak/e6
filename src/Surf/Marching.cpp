#include "../e6/e6.h"
#include "../e6/e6_sys.h"
#include "../e6/e6_math.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../Core/Core.h"
#include "Surf.h"

#include <vector>

using e6::uint;
using e6::float3;
using e6::float4;
using e6::ClassInfo;


namespace Surf
{
	
	struct Profile
	{
		const char * name;
		uint t0, t1, ncalls;
		float tp;

		static std::vector<Profile*> profs;
		
		Profile( const char * n ) 
			: name(n), t0(0), t1(0), ncalls(0), tp(0) 
		{
			profs.push_back(this);
		}
		~Profile()
		{
			printf(__FUNCTION__ "\n");
		}
		void start() 
		{
			t0 = e6::sys::getMicroSeconds();
		}
		void stop() 
		{
			t1 = e6::sys::getMicroSeconds();
			tp += (t1-t0) * 0.001;
			ncalls ++;
		}
		float getTime() 
		{
			if ( ! ncalls ) return 0;
			float r = tp / (float)ncalls;
			ncalls = 0;
			tp  = 0;
			return r;
		}
	};
	std::vector<Profile*> Profile::profs;
	
	//These tables are used so that everything can be done in little loops that you can look at all at once
	// rather than in pages and pages of unrolled code.

	//a2fVertexOffset lists the positions, relative to vertex0, of each of the 8 vertices of a cube
	static const float a2fVertexOffset[8][3] =	
	{
			{0.0, 0.0, 0.0},{1.0, 0.0, 0.0},{1.0, 1.0, 0.0},{0.0, 1.0, 0.0},
			{0.0, 0.0, 1.0},{1.0, 0.0, 1.0},{1.0, 1.0, 1.0},{0.0, 1.0, 1.0}
	};

	//a2iEdgeConnection lists the index of the endpoint vertices for each of the 12 edges of the cube
	static const int a2iEdgeConnection[12][2] = 
	{
			{0,1}, {1,2}, {2,3}, {3,0},
			{4,5}, {5,6}, {6,7}, {7,4},
			{0,4}, {1,5}, {2,6}, {3,7}
	};

	//a2fEdgeDirection lists the direction vector (vertex1-vertex0) for each edge in the cube
	static const float a2fEdgeDirection[12][3] =
	{
			{1.0, 0.0, 0.0},{0.0, 1.0, 0.0},{-1.0, 0.0, 0.0},{0.0, -1.0, 0.0},
			{1.0, 0.0, 0.0},{0.0, 1.0, 0.0},{-1.0, 0.0, 0.0},{0.0, -1.0, 0.0},
			{0.0, 0.0, 1.0},{0.0, 0.0, 1.0},{ 0.0, 0.0, 1.0},{0.0,  0.0, 1.0}
	};

	//a2iTetrahedronEdgeConnection lists the index of the endpoint vertices for each of the 6 edges of the tetrahedron
	static const int a2iTetrahedronEdgeConnection[6][2] =
	{
			{0,1},  {1,2},  {2,0},  {0,3},  {1,3},  {2,3}
	};

	//a2iTetrahedronEdgeConnection lists the index of verticies from a cube 
	// that made up each of the six tetrahedrons within the cube
	static const int a2iTetrahedronsInACube[6][4] =
	{
			{0,5,1,6},
			{0,1,2,6},
			{0,2,3,6},
			{0,3,7,6},
			{0,7,4,6},
			{0,4,5,6},
	};


	// For any edge, if one vertex is inside of the surface and the other is outside of the surface
	//  then the edge intersects the surface
	// For each of the 4 vertices of the tetrahedron can be two possible states : either inside or outside of the surface
	// For any tetrahedron the are 2^4=16 possible sets of vertex states
	// This table lists the edges intersected by the surface for all 16 possible vertex states
	// There are 6 edges.  For each entry in the table, if edge #n is intersected, then bit #n is set to 1

	int aiTetrahedronEdgeFlags[16]=
	{
			0x00, 0x0d, 0x13, 0x1e, 0x26, 0x2b, 0x35, 0x38, 0x38, 0x35, 0x2b, 0x26, 0x1e, 0x13, 0x0d, 0x00, 
	};


	// For each of the possible vertex states listed in aiTetrahedronEdgeFlags there is a specific triangulation
	// of the edge intersection points.  a2iTetrahedronTriangles lists all of them in the form of
	// 0-2 edge triples with the list terminated by the invalid value -1.
	//
	// I generated this table by hand

	int a2iTetrahedronTriangles[16][7] =
	{
			{-1, -1, -1, -1, -1, -1, -1},
			{ 0,  3,  2, -1, -1, -1, -1},
			{ 0,  1,  4, -1, -1, -1, -1},
			{ 1,  4,  2,  2,  4,  3, -1},

			{ 1,  2,  5, -1, -1, -1, -1},
			{ 0,  3,  5,  0,  5,  1, -1},
			{ 0,  2,  5,  0,  5,  4, -1},
			{ 5,  4,  3, -1, -1, -1, -1},

			{ 3,  4,  5, -1, -1, -1, -1},
			{ 4,  5,  0,  5,  2,  0, -1},
			{ 1,  5,  0,  5,  3,  0, -1},
			{ 5,  2,  1, -1, -1, -1, -1},

			{ 3,  4,  2,  2,  4,  1, -1},
			{ 4,  1,  0, -1, -1, -1, -1},
			{ 2,  3,  0, -1, -1, -1, -1},
			{-1, -1, -1, -1, -1, -1, -1},
	};

	// For any edge, if one vertex is inside of the surface and the other is outside of the surface
	//  then the edge intersects the surface
	// For each of the 8 vertices of the cube can be two possible states : either inside or outside of the surface
	// For any cube the are 2^8=256 possible sets of vertex states
	// This table lists the edges intersected by the surface for all 256 possible vertex states
	// There are 12 edges.  For each entry in the table, if edge #n is intersected, then bit #n is set to 1

	int aiCubeEdgeFlags[256]=
	{
			0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00, 
			0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90, 
			0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30, 
			0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0, 
			0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60, 
			0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0, 
			0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950, 
			0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc, 0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0, 
			0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0, 
			0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x055, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650, 
			0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0, 
			0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x066, 0x76a, 0x663, 0x569, 0x460, 
			0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa, 0x1a3, 0x2a9, 0x3a0, 
			0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x033, 0x339, 0x230, 
			0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x099, 0x190, 
			0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x000
	};

	//  For each of the possible vertex states listed in aiCubeEdgeFlags there is a specific triangulation
	//  of the edge intersection points.  a2iTriangleConnectionTable lists all of them in the form of
	//  0-5 edge triples with the list terminated by the invalid value -1.
	//  For example: a2iTriangleConnectionTable[3] list the 2 triangles formed when corner[0] 
	//  and corner[1] are inside of the surface, but the rest of the cube is not.
	//
	//  I found this table in an example program someone wrote long ago.  It was probably generated by hand

	int a2iTriangleConnectionTable[256][16] =  
	{
			{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
			{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
			{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
			{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
			{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
			{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
			{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
			{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
			{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
			{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
			{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
			{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
			{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
			{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
			{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
			{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
			{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
			{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
			{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
			{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
			{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
			{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
			{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
			{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
			{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
			{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
			{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
			{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
			{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
			{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
			{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
			{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
			{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
			{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
			{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
			{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
			{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
			{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
			{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
			{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
			{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
			{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
			{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
			{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
			{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
			{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
			{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
			{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
			{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
			{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
			{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
			{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
			{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
			{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
			{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
			{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
			{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
			{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
			{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
			{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
			{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
			{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
			{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
			{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
			{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
			{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
			{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
			{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
			{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
			{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
			{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
			{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
			{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
			{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
			{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
			{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
			{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
			{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
			{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
			{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
			{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
			{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
			{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
			{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
			{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
			{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
			{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
			{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
			{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
			{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
			{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
			{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
			{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
			{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
			{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
			{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
			{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
			{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
			{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
			{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
			{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
			{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
			{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
			{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
			{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
			{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
			{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
			{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
			{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
			{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
			{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
			{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
			{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
			{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
			{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
			{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
			{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
			{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
			{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
			{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
			{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
			{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
			{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
			{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
			{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
			{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
			{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
			{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
			{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
			{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
			{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
			{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
			{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
			{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
			{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
			{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
			{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
			{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
			{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
			{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
			{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
			{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
			{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
			{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
			{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
			{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
			{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
			{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
			{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
			{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
			{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
			{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
			{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
			{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
			{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
			{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
			{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
			{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
			{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
			{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
			{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
			{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
			{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
			{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
			{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
			{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
			{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
			{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
			{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
			{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
			{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
			{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
			{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
			{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
			{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
			{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
			{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
			{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
			{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
			{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
			{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
	};




	struct CMarching : e6::Class< Marching, CMarching >
	{
		struct Metaball
		{
			int nTris;
			Core::Mesh * mesh;
			Core::VertexFeature * pos;
			Core::VertexFeature * nor;
			Core::VertexFeature * col;
			uint  *c;
			float *p;
			float *n;
			
			Metaball()
				: nTris(0)
				, mesh(0)
				, pos(0)
				, col(0)
				, nor(0)
				, p(0)
				, n(0)
				, c(0)
			{}

			void setMesh(Core::Mesh * m)
			{
				if ( (! mesh) || (mesh != m) )
				{
					E_RELEASE( pos );
					E_RELEASE( nor );
					E_RELEASE( col );
					E_RELEASE( mesh );
					E_ADDREF( m );
					mesh = m;
					mesh->setup( 3, 8012 );
					pos = mesh->getFeature( e6::VF_POS );
					col = mesh->getFeature( e6::VF_COL );
					nor = mesh->getFeature( e6::VF_NOR );
				}
				reset();
			}
				
			~Metaball()
			{
				$X();
				E_RELEASE( pos );
				E_RELEASE( col );
				E_RELEASE( nor );
				E_RELEASE( mesh );
			}
			
			void reset()
			{
				nTris = 0;
				p = (float*)pos->getElement(0);
				n = (float*)nor->getElement(0);
				c = (uint*)col->getElement(0);
			}
		};

		Metaball  ball;
		int     iDataSetSize;
		float   scale;
		float   fStepSize;
		float   fTargetValue;
		float4  sSourcePoint[32];
		uint	nSourcePoints;
		
		CMarching()
			: iDataSetSize(16)
			, fStepSize(1.0/16.0)
			, fTargetValue(48.0)
			, scale(8.0)
			, nSourcePoints(6)
		{}
		~CMarching()
		{}
			

		virtual uint setSize( float s ) 
		{
			fTargetValue = s;
			return 1;
		}
		virtual uint setScale( float s ) 
		{
			scale = s;
			return 1;
		}
		virtual uint update( Core::Mesh * mesh ) 
		{
			static Profile _prof(__FUNCTION__);
			_prof.start();
			static Profile _prof1(__FUNCTION__ ".setMesh");
			_prof1.start();
			ball.setMesh( mesh );
			_prof1.stop();

			vMarchingCubes();

			mesh->setup( 3, ball.nTris );

			_prof.stop();
			static uint _i=0;
			if ( _i++ == 33 )
			{
				_i = 0;
				printf( __FUNCTION__" %i tris.\n", ball.nTris );
				for ( uint i=0; i<_prof.profs.size(); i++ )
				{
					uint   n = _prof.profs[i]->ncalls;
					float tp = _prof.profs[i]->tp;
					float  t = _prof.profs[i]->getTime();
					printf( "%-40s : %-8i %-8.5f %-8.5f\n", _prof.profs[i]->name,n,t,tp );
				}
			}
			return 1;
		}
				
		virtual void setNumPotentials( uint n )
		{	
			if ( n > 32 ) n=32;
			nSourcePoints = n;
		}
		virtual uint setPotential( uint i, float x, float y, float z, float r  )
		{
			if ( i >= nSourcePoints ) return 0;
			sSourcePoint[i] = float4(x,y,z,r);
			return 1;
		}
		
		//fGetOffset finds the approximate point of intersection of the surface
		// between two points with the values fValue1 and fValue2
		float fGetOffset(float fValue1, float fValue2, float fValueDesired)
		{
				double fDelta = fValue2 - fValue1;

				if(fDelta == 0.0)
				{
						return 0.5;
				}
				return (fValueDesired - fValue1)/fDelta;
		}


		//vGetColor generates a color from a given position and normal of a point
		void vGetColor(float3 &rfColor, float3 &rfPosition, float3 &rfNormal)
		{
				float x = rfNormal.x;
				float y = rfNormal.y;
				float z = rfNormal.z;
				rfColor.x = (x > 0.0 ? x : 0.0) + (y < 0.0 ? -0.5*y : 0.0) + (z < 0.0 ? -0.5*z : 0.0);
				rfColor.y = (y > 0.0 ? y : 0.0) + (z < 0.0 ? -0.5*z : 0.0) + (x < 0.0 ? -0.5*x : 0.0);
				rfColor.z = (z > 0.0 ? z : 0.0) + (x < 0.0 ? -0.5*x : 0.0) + (y < 0.0 ? -0.5*y : 0.0);
		}

		void vNormalizeVector(float3 &rfVectorResult, float3 &rfVectorSource)
		{
				float fOldLength;
				float fScale;

				fOldLength = sqrtf( (rfVectorSource.x * rfVectorSource.x) +
									(rfVectorSource.y * rfVectorSource.y) +
									(rfVectorSource.z * rfVectorSource.z) );

				if(fOldLength == 0.0)
				{
						rfVectorResult.x = rfVectorSource.x;
						rfVectorResult.y = rfVectorSource.y;
						rfVectorResult.z = rfVectorSource.z;
				}
				else
				{
						fScale = 1.0/fOldLength;
						rfVectorResult.x = rfVectorSource.x*fScale;
						rfVectorResult.y = rfVectorSource.y*fScale;
						rfVectorResult.z = rfVectorSource.z*fScale;
				}
		}


		//~ //Generate a sample data set.  fSample1(), fSample2() and fSample3() define three scalar fields whose
		//~ // values vary by the X,Y and Z coordinates and by the fTime value set by vSetTime()
		//~ void vSetTime(float fNewTime)
		//~ {
				//~ float fOffset;
				//~ int iSourceNum;

				//~ for(iSourceNum = 0; iSourceNum < 3; iSourceNum++)
				//~ {
						//~ sSourcePoint[iSourceNum].x = 0.5;
						//~ sSourcePoint[iSourceNum].y = 0.5;
						//~ sSourcePoint[iSourceNum].z = 0.5;
				//~ }

				//~ fTime = fNewTime;
				//~ fOffset = 1.0 + sinf(fTime);
				//~ sSourcePoint[0].x *= fOffset;
				//~ sSourcePoint[1].y *= fOffset;
				//~ sSourcePoint[2].z *= fOffset;
		//~ }

		//fSample1 finds the distance of (x, y, z) from three moving points
		float fSample1(float x, float y, float z)
		{
			static Profile _prof(__FUNCTION__);
			_prof.start();
			double fResult = 0.0;
			double fDx, fDy, fDz;
			for ( uint i=0; i<nSourcePoints; i++ )
			{
				fDx = x - sSourcePoint[i].x;
				fDy = y - sSourcePoint[i].y;
				fDz = z - sSourcePoint[i].z;
				fResult += sSourcePoint[i].w/(fDx*fDx + fDy*fDy + fDz*fDz);
			}
			_prof.stop();
			return fResult;
		}

		//~ //fSample2 finds the distance of (x, y, z) from three moving lines
		//~ float fSample2(float x, float y, float z)
		//~ {
				//~ double fResult = 0.0;
				//~ double fDx, fDy, fDz;
				//~ fDx = x - sSourcePoint[0].x;
				//~ fDy = y - sSourcePoint[0].y;
				//~ fResult += 0.5/(fDx*fDx + fDy*fDy);

				//~ fDx = x - sSourcePoint[1].x;
				//~ fDz = z - sSourcePoint[1].z;
				//~ fResult += 0.75/(fDx*fDx + fDz*fDz);

				//~ fDy = y - sSourcePoint[2].y;
				//~ fDz = z - sSourcePoint[2].z;
				//~ fResult += 1.0/(fDy*fDy + fDz*fDz);

				//~ return fResult;
		//~ }


		//~ //fSample2 defines a height field by plugging the distance from the center into the sin and cos functions
		//~ float fSample3(float x, float y, float z)
		//~ {
				//~ float fHeight = 20.0*(fTime + sqrt((0.5-x)*(0.5-x) + (0.5-y)*(0.5-y)));
				//~ fHeight = 1.5 + 0.1*(sinf(fHeight) + cosf(fHeight));
				//~ double fResult = (fHeight - z)*50.0;

				//~ return fResult;
		//~ }


		//vGetNormal() finds the gradient of the scalar field at a point
		//This gradient can be used as a very accurate vertx normal for lighting calculations
		void vGetNormal(float3 &rfNormal, float x, float y, float z)
		{
			static Profile _prof(__FUNCTION__);
			_prof.start();
				rfNormal.x = fSample1(x-0.01, y, z) - fSample1(x+0.01, y, z);
				rfNormal.y = fSample1(x, y-0.01, z) - fSample1(x, y+0.01, z);
				rfNormal.z = fSample1(x, y, z-0.01) - fSample1(x, y, z+0.01);
				vNormalizeVector(rfNormal, rfNormal);
			_prof.stop();
		}


		//vMarchCube1 performs the Marching Cubes algorithm on a single cube
		void vMarchCube1(float x, float y, float z, float fScale)
		{
			static Profile _prof(__FUNCTION__);
			_prof.start();
			extern int aiCubeEdgeFlags[256];
			extern int a2iTriangleConnectionTable[256][16];

			int iCorner, iVertex, iVertexTest, iEdge, iTriangle, iFlagIndex, iEdgeFlags;
			float fOffset;
			float3 sColor;
			float afCubeValue[8];
			float3 asEdgeVertex[12];
			float3 asEdgeNorm[12];

			static Profile _prof1(__FUNCTION__ ".copy");
			_prof1.start();
			//Make a local copy of the values at the cube's corners
			for(iVertex = 0; iVertex < 8; iVertex++)
			{
				afCubeValue[iVertex] = fSample1(x + a2fVertexOffset[iVertex][0]*fScale,
												y + a2fVertexOffset[iVertex][1]*fScale,
												z + a2fVertexOffset[iVertex][2]*fScale);
			}

			//Find which vertices are inside of the surface and which are outside
			iFlagIndex = 0;
			for(iVertexTest = 0; iVertexTest < 8; iVertexTest++)
			{
				if(afCubeValue[iVertexTest] <= fTargetValue) 
					iFlagIndex |= 1<<iVertexTest;
			}

			//Find which edges are intersected by the surface
			iEdgeFlags = aiCubeEdgeFlags[iFlagIndex];
			_prof1.stop();

			//If the cube is entirely inside or outside of the surface, then there will be no intersections
			if(iEdgeFlags == 0) 
			{
				_prof.stop();
				return;
			}

			static Profile _prof2(__FUNCTION__ ".edge");
			_prof2.start();
			//Find the point of intersection of the surface with each edge
			//Then find the normal to the surface at those points
			for(iEdge = 0; iEdge < 12; iEdge++)
			{
				//if there is an intersection on this edge
				if(iEdgeFlags & (1<<iEdge))
				{
					fOffset = fGetOffset(afCubeValue[ a2iEdgeConnection[iEdge][0] ], 
										 afCubeValue[ a2iEdgeConnection[iEdge][1] ], fTargetValue);

					asEdgeVertex[iEdge].x = scale * ( x + (a2fVertexOffset[ a2iEdgeConnection[iEdge][0] ][0]  +  fOffset * a2fEdgeDirection[iEdge][0]) * fScale );
					asEdgeVertex[iEdge].y = scale * ( y + (a2fVertexOffset[ a2iEdgeConnection[iEdge][0] ][1]  +  fOffset * a2fEdgeDirection[iEdge][1]) * fScale );
					asEdgeVertex[iEdge].z = scale * ( z + (a2fVertexOffset[ a2iEdgeConnection[iEdge][0] ][2]  +  fOffset * a2fEdgeDirection[iEdge][2]) * fScale );

					vGetNormal(asEdgeNorm[iEdge], asEdgeVertex[iEdge].x, asEdgeVertex[iEdge].y, asEdgeVertex[iEdge].z);
				}
			}
			_prof2.stop();

			//~ printf( "- [%3.3f\t%3.3f\t%3.3f] %3.3f\n",x,y,z,fScale );
			//Draw the triangles that were found.  There can be up to five per cube
			static Profile _prof3(__FUNCTION__ ".tri");
			_prof3.start();
			for(iTriangle = 0; iTriangle < 5; iTriangle++)
			{
				uint iTriangle3 = iTriangle * 3;
				if(a2iTriangleConnectionTable[iFlagIndex][iTriangle3] < 0)
				{
					break;
				}
				ball.nTris ++;
				assert( ball.nTris < 8012 );
				for(iCorner = 0; iCorner < 3; iCorner++)
				{
					iVertex = a2iTriangleConnectionTable[iFlagIndex][iTriangle3+iCorner];

					vGetColor(sColor, asEdgeVertex[iVertex], asEdgeNorm[iVertex]);
					//~ glColor3f(sColor.x, sColor.y, sColor.z);
					//~ glNormal3f(asEdgeNorm[iVertex].x,   asEdgeNorm[iVertex].y,   asEdgeNorm[iVertex].z);
					//~ glVertex3f(asEdgeVertex[iVertex].x, asEdgeVertex[iVertex].y, asEdgeVertex[iVertex].z);

					{
						*(ball.p)++ = asEdgeVertex[iVertex].x;
						*(ball.p)++ = asEdgeVertex[iVertex].y;
						*(ball.p)++ = asEdgeVertex[iVertex].z;
						*(ball.n)++ = asEdgeNorm[iVertex].x;
						*(ball.n)++ = asEdgeNorm[iVertex].y;
						*(ball.n)++ = asEdgeNorm[iVertex].z;
						*(ball.c)++ = 0xff000000 | int(sColor.z*255)<<16 | int(sColor.y*255)<<8 | int(sColor.x*255) ;
					}
				}
			}
			_prof3.stop();
			_prof.stop();
		}

		//~ //vMarchTetrahedron performs the Marching Tetrahedrons algorithm on a single tetrahedron
		//~ void vMarchTetrahedron(float3 *pasTetrahedronPosition, float *pafTetrahedronValue)
		//~ {
				//~ extern int aiTetrahedronEdgeFlags[16];
				//~ extern int a2iTetrahedronTriangles[16][7];

				//~ int iEdge, iVert0, iVert1, iEdgeFlags, iTriangle, iCorner, iVertex, iFlagIndex = 0;
				//~ float fOffset, fInvOffset, fValue = 0.0;
				//~ float3 asEdgeVertex[6];
				//~ float3 asEdgeNorm[6];
				//~ float3 sColor;

				//~ //Find which vertices are inside of the surface and which are outside
				//~ for(iVertex = 0; iVertex < 4; iVertex++)
				//~ {
						//~ if(pafTetrahedronValue[iVertex] <= fTargetValue) 
								//~ iFlagIndex |= 1<<iVertex;
				//~ }

				//~ //Find which edges are intersected by the surface
				//~ iEdgeFlags = aiTetrahedronEdgeFlags[iFlagIndex];

				//~ //If the tetrahedron is entirely inside or outside of the surface, then there will be no intersections
				//~ if(iEdgeFlags == 0)
				//~ {
						//~ return;
				//~ }
				//~ //Find the point of intersection of the surface with each edge
				//~ // Then find the normal to the surface at those points
				//~ for(iEdge = 0; iEdge < 6; iEdge++)
				//~ {
						//~ //if there is an intersection on this edge
						//~ if(iEdgeFlags & (1<<iEdge))
						//~ {
								//~ iVert0 = a2iTetrahedronEdgeConnection[iEdge][0];
								//~ iVert1 = a2iTetrahedronEdgeConnection[iEdge][1];
								//~ fOffset = fGetOffset(pafTetrahedronValue[iVert0], pafTetrahedronValue[iVert1], fTargetValue);
								//~ fInvOffset = 1.0 - fOffset;

								//~ asEdgeVertex[iEdge].x = fInvOffset*pasTetrahedronPosition[iVert0].x  +  fOffset*pasTetrahedronPosition[iVert1].x;
								//~ asEdgeVertex[iEdge].y = fInvOffset*pasTetrahedronPosition[iVert0].y  +  fOffset*pasTetrahedronPosition[iVert1].y;
								//~ asEdgeVertex[iEdge].z = fInvOffset*pasTetrahedronPosition[iVert0].z  +  fOffset*pasTetrahedronPosition[iVert1].z;
								
								//~ vGetNormal(asEdgeNorm[iEdge], asEdgeVertex[iEdge].x, asEdgeVertex[iEdge].y, asEdgeVertex[iEdge].z);
						//~ }
				//~ }
				//~ //Draw the triangles that were found.  There can be up to 2 per tetrahedron
				//~ for(iTriangle = 0; iTriangle < 2; iTriangle++)
				//~ {
						//~ if(a2iTetrahedronTriangles[iFlagIndex][3*iTriangle] < 0)
								//~ break;

						//~ for(iCorner = 0; iCorner < 3; iCorner++)
						//~ {
								//~ iVertex = a2iTetrahedronTriangles[iFlagIndex][3*iTriangle+iCorner];

								//~ vGetColor(sColor, asEdgeVertex[iVertex], asEdgeNorm[iVertex]);
								//~ glColor3f(sColor.x, sColor.y, sColor.z);
								//~ glNormal3f(asEdgeNorm[iVertex].x,   asEdgeNorm[iVertex].y,   asEdgeNorm[iVertex].z);
								//~ glVertex3f(asEdgeVertex[iVertex].x, asEdgeVertex[iVertex].y, asEdgeVertex[iVertex].z);
						//~ }
				//~ }
		//~ }



		//~ //vMarchCube2 performs the Marching Tetrahedrons algorithm on a single cube by making six calls to vMarchTetrahedron
		//~ void vMarchCube2(float x, float y, float z, float fScale)
		//~ {
				//~ int iVertex, iTetrahedron, iVertexInACube;
				//~ float3 asCubePosition[8];
				//~ float  afCubeValue[8];
				//~ float3 asTetrahedronPosition[4];
				//~ float  afTetrahedronValue[4];

				//~ //Make a local copy of the cube's corner positions
				//~ for(iVertex = 0; iVertex < 8; iVertex++)
				//~ {
						//~ asCubePosition[iVertex].x = x + a2fVertexOffset[iVertex][0]*fScale;
						//~ asCubePosition[iVertex].y = y + a2fVertexOffset[iVertex][1]*fScale;
						//~ asCubePosition[iVertex].z = z + a2fVertexOffset[iVertex][2]*fScale;
				//~ }

				//~ //Make a local copy of the cube's corner values
				//~ for(iVertex = 0; iVertex < 8; iVertex++)
				//~ {
						//~ afCubeValue[iVertex] = fSample(asCubePosition[iVertex].x,
														   //~ asCubePosition[iVertex].y,
													   //~ asCubePosition[iVertex].z);
				//~ }

				//~ for(iTetrahedron = 0; iTetrahedron < 6; iTetrahedron++)
				//~ {
						//~ for(iVertex = 0; iVertex < 4; iVertex++)
						//~ {
								//~ iVertexInACube = a2iTetrahedronsInACube[iTetrahedron][iVertex];
								//~ asTetrahedronPosition[iVertex].x = asCubePosition[iVertexInACube].x;
								//~ asTetrahedronPosition[iVertex].y = asCubePosition[iVertexInACube].y;
								//~ asTetrahedronPosition[iVertex].z = asCubePosition[iVertexInACube].z;
								//~ afTetrahedronValue[iVertex] = afCubeValue[iVertexInACube];
						//~ }
						//~ vMarchTetrahedron(asTetrahedronPosition, afTetrahedronValue);
				//~ }
		//~ }
				

		//vMarchingCubes iterates over the entire dataset, calling vMarchCube on each cube
		void vMarchingCubes()
		{
			static Profile _prof(__FUNCTION__);
			_prof.start();
			ball.reset();
			int iX, iY, iZ;
			for(iX = 0; iX < iDataSetSize; iX++)
			for(iY = 0; iY < iDataSetSize; iY++)
			for(iZ = 0; iZ < iDataSetSize; iZ++)
			{
				vMarchCube1(iX*fStepSize, iY*fStepSize, iZ*fStepSize, fStepSize);
			}
			_prof.stop();
		}
		
	}; // Marching



} // Surf



ClassInfo ClassInfo_Marching =
{	"Surf.Marching",	"Surf",	Surf::CMarching::createInterface,Surf::CMarching::classRef	};

