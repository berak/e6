#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../e6/e6_enums.h"
#include "../Core/Core.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h> // FILE



using e6::uint;
using e6::ClassInfo;


namespace Ie6
{
	// simple math types just to store the raw data
	typedef float octVect2[2];
	typedef float octVect3[3];
	typedef float octPlane[4];				// 4th float is distance

	struct octVert
	{
	   octVect2 tv;										// texture coordinates
	   octVect2 lv;										// lightmap coordinates
	   octVect3 pos;									// vertex position
	};

	struct octFace
	{
	   int start;											// first face vert in vertex array
	   int num;												// number of verts in the face
	   int id;												// texture index into the texture array
	   int lid;												// lightmap index into the lightmap array
	   octPlane p;
	};

	#define MAXTEXTURENAMELEN 64

	struct octTexture
	{
	   unsigned int id;							// texture id
	   char name[MAXTEXTURENAMELEN];// texture name
	};

	struct octLightmap
	{
	   unsigned int id;							// lightmaps id
	   unsigned char map[49152];		// 128 x 128 raw RGB data
	};

	class OctMap  
	{
	public:
		OctMap();
		virtual ~OctMap();

		void SetNumVerts(int n)           { numVerts = n; };
		void SetNumFaces(int n)           { numFaces = n; };
		void SetNumTextures(int n)        { numTextures = n; };
		void SetNumLightmaps(int n)       { numLightmaps = n; };
		void SetVerts(octVert *v)         { verts = v; };
		void SetFaces(octFace *f)         { faces = f; };
		void SetTextures(octTexture *t)   { textures = t; };
		void SetLightmaps(octLightmap *l) { lightmaps = l; };

		int GetNumVerts(void)							{ return numVerts; };
		int GetNumFaces(void)							{ return numFaces; };
		int GetNumTextures(void)					{ return numTextures; };
		int GetNumLightmaps(void)					{ return numLightmaps; };

		octVert *GetVerts(void)           { return verts; };
		octFace *GetFaces(void)           { return faces; };
		octTexture *GetTextures(void)     { return textures; };
		octLightmap *GetLightmaps(void)   { return lightmaps; };
		octVect3 *GetPlayerStartPos(void)	{ return &playerStartPos; };
		float GetPlayerStartRot(void)			{ return playerStartRot; };
		
		void Reset(void);
		bool Load(char *name, bool text = 0);
		bool Write(char *dest, bool text = 0);	
	protected:
		int numVerts;
		int numFaces;
		int numTextures;
		int numLightmaps;
		octVert *verts;
		octFace *faces;
		octTexture *textures;
		octLightmap *lightmaps;
		octVect3 playerStartPos;
		float playerStartRot;
	};


















	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	OctMap::OctMap()
	{
		numVerts = numFaces = numTextures = numLightmaps = 0;
		verts = 0;
		faces = 0;
		textures = 0;
		lightmaps = 0;
		playerStartPos[0] = playerStartPos[1] = playerStartPos[2] = 0;
		playerStartRot = 0;
	}

	OctMap::~OctMap()
	{
		Reset();
	}

	void OctMap::Reset(void)
	{
		delete [] verts;
		delete [] faces;
		delete [] textures;
		delete [] lightmaps;
		
		numVerts = numFaces = numTextures = numLightmaps = 0;
		verts = 0;
		faces = 0;
		textures = 0;
		lightmaps = 0;
	}

	unsigned char *FileReadAll(FILE *in)
	{
	   unsigned char *b = 0;
	   long size = 0;
	   
	   long curpos;
	   curpos = ftell(in);
	   fseek(in, 0L, SEEK_END);
	   size = ftell(in);
	   fseek(in, curpos, SEEK_SET);

	   if (!size)
	      return 0;
	   
	   b = new unsigned char[size + 1];
	   memset(b, '\0', size + 1);

	   if (!b)
	      return 0;
	   
	   fread(b, 1, size, in); // check return with size?

	   return b;
	}

	bool OctMap::Load(char *name, bool text)
	{  
		FILE *in = 0;
		
		if (!text)
		{
			if (!(in = fopen(name, "rb"))) return 0;
			
			unsigned char *buffer = FileReadAll(in);
			unsigned char *where = buffer;
			
			fclose(in);
			
			if (!buffer) return 0;
			
			numVerts = *(int *) where; where += sizeof(int);
			numFaces = *(int *) where; where += sizeof(int);
			numTextures = *(int *) where; where += sizeof(int);
			numLightmaps = *(int *) where; where += sizeof(int);
			
			verts = new octVert[numVerts];
			faces = new octFace[numFaces];
			textures = new octTexture[numTextures];
	//		lightmaps = new octLightmap[numLightmaps];
			
			int size = 0;
			
			size = sizeof(octVert) * numVerts;
			memcpy(verts, where, size);
			where += size;
			
			size = sizeof(octFace) * numFaces;
			memcpy(faces, where, size);
			where += size;
			
			size = sizeof(octTexture) * numTextures;
			memcpy(textures, where, size);
			where += size;
			
			size = sizeof(octLightmap) * numLightmaps;
	//		memcpy(lightmaps, where, size);
			where += size;
			
			// read in player start pos and rot
			size = sizeof(octVect3);
			memcpy(playerStartPos,where,size);
			where += size;
			size = sizeof(float);
			memcpy(&playerStartRot,where,size);
			where += size;

			delete [] buffer;
		}
		return 1;
	}




	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{
		virtual uint load(e6::Engine * e, Core::World * w, const char * orig) 
		{
			engine = e;
			world = w;
			root = w->getRoot();
			uint start = e6::sys::getMicroSeconds();


			OctMap oct;
			if ( ! oct.Load((char*)orig) )
				return 0;

			u_int nv     = oct.GetNumVerts() ;
			u_int ni     = oct.GetNumFaces() ;
			octVert *ov  = oct.GetVerts();
			octFace *of  = oct.GetFaces();

			sec->setName("oct_section");

			u_int ni2 = 0;
			for ( u_int i=0; i<ni; i++ )
			{
				ni2 += of[i].num-2;
			}
			Machine::log("sec : %d verts %d/%d faces\n", nv, ni,ni2 ); 

			IVertexFeature *pos = sec->createFeature(0,nv,ni2);
			IVertexFeature *uv0 = sec->createFeature(2,nv,ni2);

			for ( u_int i=0; i<nv; i++ )
			{
				TVector p( ov[i].pos );
				p *= 0.4f;
				pos->setVertex(i, p.v ) ;
				uv0->setVertex(i, ov[i].tv ) ;
			}

			for ( u_int i=0,k=0; i<ni; i++ )
			{
				u_int z = of[i].start;
				Machine::log("face : %d start %d verts %d\n", i, z, of[i].num ); 
				bool toggle=1;		

				//fan:
				for ( int j=1; j<of[i].num-1; j++ )
				{
					u_int tri[] = { z, z+j, z+j+1 };
					toggle = ! toggle;
					pos->setIndex(k, tri ) ;
					uv0->setIndex(k, tri ) ;
					k++;
				}
			}
			IMaterial *mtl = sec->createMaterial();
			mtl->setName("oct_mat");
			mtl->setFaces(0,ni2);


			uint stop = e6::sys::getMicroSeconds();
			printf( "loaded %s in %lu micros.\n", orig, stop-start );
			E_RELEASE( w );
			E_RELEASE( e );
			E_RELEASE( root );
			return r;
		}
	};
};



extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Importer",	 "Importe6",	Ie6::CImporter::createInterface	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}



#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ( "Importe6 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}



