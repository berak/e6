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

	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{
		FILE *fp;
		char line[500];
		uint lineNo;

		e6::Engine *engine;
	    Core::World * world;
		Core::Node * root;
	
		CImporter()
			: lineNo(0)
		{
		}
		~CImporter()
		{
		}
		uint parseError(const char *s)
		{
			e6::sys::alert( s, " parse error in line %i : %s", lineNo, line );
			return 0;
		}

		char * readLine()
		{
			line[0]=0;
			while( 1 )
			{
				if ( ! fgets( line,499,fp ) ) 
					return 0;
				lineNo ++;
//	printf("* %s", line );
				if ( line[0] == '#' )
					continue;
				if ( line[0] == '\r' )
					continue;
				if ( line[0] == '\n' )
					continue;
	        	break;
			}

	        return line;
		}

		uint readFreeNode( Core::FreeNode * node)
		{
			return 1;
		}

		uint readFeature3( Core::VertexFeature * f )
		{
			uint n = f->numElements();
			float p[3];
			for ( uint i=0; i<n; i++ )
			{
				char * s = readLine();
				if ( 3 != sscanf(s, "%f %f %f", &p[0], &p[1], &p[2] ) )
					return parseError(__FUNCTION__ " expected Feature3" );
				f->setElement( i, p );			
			}
			return 1;
		}
		uint readFeature2( Core::VertexFeature * f )
		{
			uint n = f->numElements();
			float p[2];
			for ( uint i=0; i<n; i++ )
			{
				char * s = readLine();
				if ( 2 != sscanf(s, "%f %f", &p[0], &p[1] ) )
					return parseError(__FUNCTION__ " expected Feature2");
				f->setElement( i, p );			
			}
			return 1;
		}
		uint readFeature1( Core::VertexFeature * f )
		{
			uint n = f->numElements();
			uint c;
			for ( uint i=0; i<n; i++ )
			{
				char * s = readLine();
				if ( 1 != sscanf(s, "%i", &c ) )
					return parseError(__FUNCTION__  " expected Feature1");
				f->setElement( i, &c );			
			}
			return 1;
		}

		uint readMesh( Core::Mesh * mesh )
		{
			// format
			int fmt,nf;
			char n[200];
			char * s = readLine();
			if ( 1 != sscanf(s, "format %i", &fmt ) )
				return parseError(__FUNCTION__   " expected format");
			// faces
			s = readLine();
			if ( 1 != sscanf(s, "faces %i", &nf ) )
				return parseError(__FUNCTION__  " expected nfaces");

			mesh->setup( fmt, nf );

			// vs
			s = readLine();
			if ( 1 != sscanf(s, "vshader %s", n ) )
				return parseError(__FUNCTION__  " expected vshader");
			if ( strcmp( n, "none" ) )
			{
				if ( world->load( engine, n ) )
				{
					Core::Shader * vs = world->shaders().get(n);
					//Core::Shader * vs = (Core::Shader *)engine->createInterface("Core","Core.Shader");
					//vs->setName( n );
					//world->shaders().add( vs->getName(), vs );
					mesh->setVertexShader( vs );
					E_RELEASE( vs );
				}
			}
			int nc=0;
			int ic=0;
			float f[4];

			s = readLine();
			if ( 1 != sscanf(s, "vshader_constants %i", &nc ) )
				return 0;
			for ( int i=0; i<nc; i++ )
			{
				s = readLine();
				if ( 5 != sscanf(s, "%i %f %f %f %f", &ic, &f[0], &f[1], &f[2], &f[3] ) )
					return parseError(__FUNCTION__ "vshader const");
				mesh->setVertexShaderConstant( ic, f );
			}

			// ps
			s = readLine();
			if ( 1 != sscanf(s, "pshader %s", n ) )
					return parseError(__FUNCTION__  " expected pshader");
			if ( strcmp( n, "none" ) )
			{
				if ( world->load( engine, n ) )
				{
					Core::Shader * ps = world->shaders().get(n);
					//Core::Shader * ps = (Core::Shader *)engine->createInterface("Core","Core.Shader");
					//ps->setName( n );
					//world->shaders().add( ps->getName(), ps );
					//assert( ps );
					mesh->setPixelShader( ps );
					E_RELEASE( ps );
				}
			}
			s = readLine();
			if ( 1 != sscanf(s, "pshader_constants %i", &nc ) )
					return parseError(__FUNCTION__  " expected shader_constants");
			for ( int i=0; i<nc; i++ )
			{
				s = readLine();
				if ( 5 != sscanf(s, "%i %f %f %f %f", &ic, &f[0], &f[1], &f[2], &f[3] ) )
					return parseError(__FUNCTION__ " expected shader_constants");
				mesh->setPixelShaderConstant( ic, f );
			}

			uint rs=0;
			s = readLine();
			if ( 1 != sscanf(s, "renderstates %i", &nc ) )
				return 0;
			for ( int i=0; i<nc; i++ )
			{
				s = readLine();
				if ( 2 != sscanf(s, "%i %i", &ic, &rs ) )
					return parseError(__FUNCTION__ " expected renderstates");
				mesh->setRenderState( ic, rs );
			}

			// texmapping:
			for ( int i=0; i<4; i++ )
			{
				int m=0;
				s = readLine();
				if ( 2 != sscanf(s, "texmap %i %s", &m, n ) )
					return parseError(__FUNCTION__ " expected texmap");
				if ( strcmp( n, "none" ) )
				{
					Core::Texture * t = 0;
					if ( strstr( n, ".dds" ) )
					{
						t = (Core::Texture *)engine->createInterface( "Core", "Core.Texture" );
						t->setName( n );
						world->textures().add(t->getName(),t);
					}
					else
					{
						uint r = world->load( engine, n );
						t = world->textures().get(n);
					}
					mesh->setTexture( i, t );
					m++;
					//mesh->setMapping( i, m++ );
					E_RELEASE( t );
				}
			}			
			uint r = 0;
			//if ( fmt & e6::VF_POS )
			{
				Core ::VertexFeature *vf = mesh->getFeature( e6::VF_POS );
				r = readFeature3( vf );
				E_RELEASE( vf );				
				if ( ! r ) return 0;
			}
			if ( fmt & e6::VF_COL )
			{
				Core ::VertexFeature *vf = mesh->getFeature( e6::VF_COL );
				r = readFeature1( vf );
				E_RELEASE( vf );				
				if ( ! r ) return 0;
			}
			if ( fmt & e6::VF_NOR )
			{
				Core ::VertexFeature *vf = mesh->getFeature( e6::VF_NOR );
				r = readFeature3( vf );
				E_RELEASE( vf );				
				if ( ! r ) return 0;
			}
			if ( fmt & e6::VF_UV0 )
			{
				Core ::VertexFeature *vf = mesh->getFeature( e6::VF_UV0 );
				r = readFeature2( vf );
				E_RELEASE( vf );				
				if ( ! r ) return 0;
			}
			if ( fmt & e6::VF_UV1 )
			{
				Core ::VertexFeature *vf = mesh->getFeature( e6::VF_UV1 );
				r = readFeature2( vf );
				E_RELEASE( vf );				
				if ( ! r ) return 0;
			}
			if ( fmt & e6::VF_UV2 )
			{
				Core ::VertexFeature *vf = mesh->getFeature( e6::VF_UV2 );
				r = readFeature2( vf );
				E_RELEASE( vf );				
				if ( ! r ) return 0;
			}
			if ( fmt & e6::VF_TAN )
			{
				Core ::VertexFeature *vf = mesh->getFeature( e6::VF_TAN );
				r = readFeature3( vf );
				E_RELEASE( vf );				
				if ( ! r ) return 0;
			}

			return 1;
		}

		uint readCamera( Core::Camera * cam )
		{
			// fov
			float f=0, g=0;
			char * s = readLine();
			if ( 1 != sscanf(s, "fov %f", &f ) )
					return parseError(__FUNCTION__ " expected fov");
			cam->setFov( f );
			// planes
			s = readLine();
			if ( 1 != sscanf(s, "nearplane %f", &f ) )
					return parseError(__FUNCTION__" expected nearplane");
			cam->setNearPlane( f );
			s = readLine();
			if ( 1 != sscanf(s, "farplane %f", &f ) )
					return parseError(__FUNCTION__" expected farplane");
			cam->setFarPlane( f );
			return 1;
		}

		uint readLight( Core::Light * light )
		{
			// type
			int i=0;
			char * s = readLine();
			if ( 1 != sscanf(s, "type %i", &i ) )
					return parseError(__FUNCTION__ " expected light type");
			light->setType( i );

			// decay
			s = readLine();
			if ( 1 != sscanf(s, "decay %i", &i ) )
					return parseError(__FUNCTION__ " expected light decay");
			light->setDecay( i );

			// color
			e6::float3 p;
			s = readLine();
			if ( 3 != sscanf(s, "color %f %f %f", &p[0], &p[1], &p[2] ) )
					return parseError(__FUNCTION__ " expected light color");
			light->setColor( p );			

			// dir
			s = readLine();
			if ( 3 != sscanf(s, "dir %f %f %f", &p[0], &p[1], &p[2] ) )
					return parseError(__FUNCTION__ " expected light dir");
			light->setDir( p );			
			return 1;
		}


		uint readRenderToTexture( Core::RenderToTexture * rt )
		{
			// type
			int i=0;
			char * s = readLine();
			char b[100] = "";
			if ( 1 != sscanf(s, "target %s", b ) )
					return parseError(__FUNCTION__ " expected target name");

			Core::Texture * t = (Core::Texture *) engine->createInterface( "Core", "Core.Texture" );
			if ( ! t )
			{
				E_RELEASE(rt);
				return parseError(__FUNCTION__ " failed to create rendertexture");
			}
			t->alloc( e6::PF_X8B8G8R8, 512, 512, 1, e6::TU_RENDERTARGET );
			t->setName(b);
			world->textures().add( t->getName(), t );

			rt->setRenderTarget(t);
			E_RELEASE(t);
			return 1;
		}

		uint readNode()
		{
			// type name
			char t[100], n[200];
			char * s = readLine();
			if ( ! s )
			{
				//~ printf( "EOF.\n" );
				return 0; 
			}
			if ( 2 != sscanf(s, "%s %s", t,n ) )
					return parseError(__FUNCTION__ " expected node name");
			char tt[200] = "Core.";
			strcat( tt, t );
			Core::Node * node = (Core::Node*)engine->createInterface( "Core", tt );			
			node->setName( n );

			// parent
			s = readLine();
			if ( 1 != sscanf(s, "parent %s", n ) )
					return parseError(__FUNCTION__ " expected node parent");
			// pos
			e6::float3 p;
			s = readLine();
			if ( 3 != sscanf(s, "pos %f %f %f", &p[0], &p[1], &p[2] ) )
					return parseError(__FUNCTION__ " expected node pos");
			node->setPos( p );			

			// rot
			s = readLine();
			if ( 3 != sscanf(s, "rot %f %f %f", &p[0], &p[1], &p[2] ) )
					return parseError(__FUNCTION__ " expected node rot");
			node->setRot( p );			

			//~ // scale
			//~ s = readLine();
			//~ if ( 3 != sscanf(s, "scale %f %f %f", &p[0], &p[1], &p[2] ) )
					//~ return parseError(__FUNCTION__ " expected node scale");
			//~ node->setScale( p );			

			// sphere
			s = readLine();
			if ( 1 != sscanf(s, "sphere %f", &p[0] ) )
					return parseError(__FUNCTION__ " expected node sphere");
			node->setSphere( p[0] );			

			// bounce
			s = readLine();
			if ( 1 != sscanf(s, "bounce %f", &p[0] ) )
					return parseError(__FUNCTION__ " expected node bounce");
			node->setBounce( p[0] );			

			// friction
			s = readLine();
			if ( 1 != sscanf(s, "friction %f", &p[0] ) )
					return parseError(__FUNCTION__ " expected node friction");
			node->setFriction( p[0] );			

			// vis
			int v=0;
			s = readLine();
			if ( 1 != sscanf(s, "visible %i", &v ) )
					return parseError(__FUNCTION__ " expected node vis");
			node->setVisibility( v );			

			uint res = 0;
			if ( ! strcmp( t, "Mesh" ) )
				res = readMesh( (Core::Mesh*)node );
			else
			if ( ! strcmp( t, "Camera" ) )
				res = readCamera( (Core::Camera*)node );
			else
			if ( ! strcmp( t, "Light" ) )
				res = readLight( (Core::Light*)node );
			else
			if ( ! strcmp( t, "RenderToTexture" ) )
				res = readRenderToTexture( (Core::RenderToTexture*)node );
			else
			if ( ! strcmp( t, "FreeNode" ) )
				res = readFreeNode( (Core::FreeNode*)node );
			if ( ! res )
			{
				E_RELEASE( node );
				return 0;
			}
			Core::Node * prnt = root->findRecursive( n );
			if ( prnt )
			{
				//~ printf( "lINK : %s->%s\n", prnt->getName(), node->getName() );
				prnt->link( node );
				E_RELEASE( prnt );
			}
			E_RELEASE( node );
	        return res;
		}

		uint import(const char * orig)
		{
		    fp=fopen(orig,"rb");

		    if (!fp)
		        return 0;

			while ( readNode() )
			{}

			fclose( fp );
			return 1;
		}
	
	
		virtual uint load(e6::Engine * e, Core::World * w, const char * orig) 
		{
			engine = e;
			world = w;
			root = w->getRoot();
			uint start = e6::sys::getMicroSeconds();

			uint r = import(orig);

			uint stop = e6::sys::getMicroSeconds();
			if ( r )
			{
				printf( "loaded %s in %lu micros.\n", orig, stop-start );
			}
			else
			{
				printf( __FUNCTION__ " : ERROR : failed to load %s .\n", orig );
			}

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
		{	"Core.Importer",	 "Importe6",	Ie6::CImporter::createInterface, Ie6::CImporter::classRef	},
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
