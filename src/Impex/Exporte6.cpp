#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../e6/e6_enums.h"
#include "../Core/Core.h"



using e6::uint;
using e6::ClassInfo;

const char * e6ex_version = ( "Exporte6 00.00.2 (" __DATE__ ")" );

namespace Ee6
{
	struct CExporter 
		: public e6::Class< Core::Exporter, CExporter >
	{
		FILE * out;
		Core::Node * root;

		void saveNode( Core::Node * node, const char * type )
		{
			fprintf( out, "%s %s\n", type, node->getName() );
			Core::Node * prnt = node->getParent();
			if ( prnt )
			{
				fprintf( out, "parent %s\n", prnt->getName() );
				E_RELEASE( prnt );
			}
			e6::float3 p = node->getPos();
			fprintf( out, "pos %3.3f %3.3f %3.3f\n", p[0], p[1], p[2] );
			p = node->getRot();
			fprintf( out, "rot %3.3f %3.3f %3.3f\n", p[0], p[1], p[2] );
			//~ p = node->getScale();
			//~ fprintf( out, "scale %3.3f %3.3f %3.3f\n", p[0], p[1], p[2] );
			fprintf( out, "sphere %3.3f\n", node->getSphere() );
			fprintf( out, "bounce %3.3f\n", node->getBounce() );
			fprintf( out, "friction %3.3f\n", node->getFriction() );
			fprintf( out, "visible %i\n", node->getVisibility() );
			//fprintf( out, "\n" );
		}

		void save( Core::Mesh * mesh, int type, int esize )
		{
			Core::VertexFeature * v = mesh->getFeature( type );
			if ( v )
			{
				fprintf( out, "# feature %i %i \n", type, v->numElements() );
				if ( esize == 3 )
				for ( uint i=0; i<v->numElements(); i++ )
				{
					float * p((float*)v->getElement(i));
					fprintf( out, "%3.3f %3.3f %3.3f\n", p[0],p[1],p[2] );				
					//fwrite( p, 3, sizeof(float), out);
				}
				if ( esize == 2 )
				for ( uint i=0; i<v->numElements(); i++ )
				{
					float* p((float*)v->getElement(i));
					fprintf( out, "%3.3f %3.3f\n", p[0],p[1] );				
					//fwrite( p, 2, sizeof(float), out);
				}
				if ( esize == 1 )
				for ( uint i=0; i<v->numElements(); i++ )
				{
					e6::rgba c(*(e6::rgba*)v->getElement(i));
					fprintf( out, "0x%08x\n", c );				
					//fwrite( &c, 1, sizeof(e6::rgba), out);
				}
				E_RELEASE( v );
				//fprintf( out, "\n" );
			}
		}

		void save( Core::VertexBuffer * vb )
		{
			fprintf( out, "%04x\t", vb->format() );
			fprintf( out, "%i\n", vb->numFaces() );
		}

		void save( Core::Mesh * mesh )
		{
			fprintf( out, "format %04x\n", mesh->format() );
			fprintf( out, "faces %i\n", mesh->numFaces() );

			Core::Shader *vsh = mesh->getVertexShader();
			fprintf( out, "vshader %s\n", (vsh?vsh->getName():"none") );
			E_RELEASE( vsh );

			int nvc = mesh->getNumVertexShaderConstants();
			fprintf( out, "vshader_constants %i\n",nvc );
			for ( int i=0; i<nvc; i++ )
			{
				Core::Shader::Constant *c = mesh->getVertexShaderConstant(i);
				fprintf( out, "%i\t%3.3f %3.3f %3.3f %3.3f\n",c->index, c->value[0], c->value[1], c->value[2], c->value[3] );			
			}
			
			Core::Shader *psh = mesh->getPixelShader();
			fprintf( out, "pshader %s\n", (psh?psh->getName():"none") );
			E_RELEASE( psh );
			
			nvc = mesh->getNumPixelShaderConstants();
			fprintf( out, "pshader_constants %i\n",nvc );
			for ( int i=0; i<nvc; i++ )
			{
				Core::Shader::Constant *c = mesh->getPixelShaderConstant(i);
				fprintf( out, "%i\t%3.3f %3.3f %3.3f %3.3f\n",c->index, c->value[0], c->value[1], c->value[2], c->value[3] );			
			}
			
			nvc = mesh->getNumRenderStates();
			fprintf( out, "renderstates %i\n",nvc );
			for ( int i=0; i<nvc; i++ )
			{
				Core::Shader::RenderState *c = mesh->getRenderState(i);
				fprintf( out, "%i\t%i\n",c->index, c->value );			
			}

			for ( int i=0; i<4; i++ )
			{
				Core::Texture * t = mesh->getTexture(i);
				fprintf( out, "texmap 0 %s\n", (t?t->getName():"none") );
				E_RELEASE( t);
			}

			save( mesh, e6::VF_POS, 3 );
			save( mesh, e6::VF_COL, 1 );
			save( mesh, e6::VF_NOR, 3 );
			save( mesh, e6::VF_UV0, 2 );
			save( mesh, e6::VF_UV1, 2 );
			save( mesh, e6::VF_UV2, 2 );
			save( mesh, e6::VF_UV3, 2 );
			save( mesh, e6::VF_TAN, 3 );
			fprintf( out, "\n" );
		}
		void save( Core::Camera * cam )
		{
			fprintf( out, "fov %3.3f\n", cam->getFov() );
			fprintf( out, "nearplane %3.3f\n", cam->getNearPlane() );
			fprintf( out, "farplane %3.3f\n", cam->getFarPlane());
			fprintf( out, "\n" );
		}
		void save( Core::Light * light )
		{
			fprintf( out, "type %i\n", light->getType() );
			fprintf( out, "decay %i\n", light->getDecay() );
			e6::float3 p = light->getColor();
			fprintf( out, "color %3.3f %3.3f %3.3f\n",p[0],p[1],p[2] );
			p = light->getDir();
			fprintf( out, "dir %3.3f %3.3f %3.3f\n",p[0],p[1],p[2] );
			fprintf( out, "\n" );
		}
		void save( Core::FreeNode * f )
		{
			fprintf( out, "\n" );
		}
		void save( Core::RenderToTexture * rt )
		{
			Core::Texture * t = rt->getRenderTarget();
			fprintf( out, "target %s\n", (t ? t->getName() : "none") );
			E_RELEASE(t);
		}
		void save( Core::Node * node )
		{
			if ( node != root )
			{
				const char * t = node->typeName();
				saveNode( node, t );;
	
				if ( !strcmp(t, "Mesh") )
					save((Core::Mesh*)node);
				else
				if ( !strcmp(t, "Camera") )
					save((Core::Camera*)node);
				else
				if ( !strcmp(t, "Light") )
					save((Core::Light*)node);
				else
				if ( !strcmp(t, "RenderToTexture") )
					save((Core::RenderToTexture*)node);
				else
				save((Core::FreeNode*)node);
			}

			int i=0;
			while ( Core::Node * n = node->getChild(i++) )
			{
				save( n );
				E_RELEASE( n );
			} 
		}

		virtual uint save(e6::Engine * e, Core::World * w, const char * path, void *item ) 
		{
			$1(path);
			Core::Node * start_node = (Core::Node *)item;
			root = w->getRoot();
			out = fopen( path, "wb" );
			if ( out )
			{
				fprintf( out, "# %s\n", e6ex_version );
				save( start_node );
				fprintf( out, "\n" );
				fclose( out );
			}

//			E_RELEASE( start_node );
			E_RELEASE( root );
			//E_RELEASE( w );
			//E_RELEASE( e );
			return 1;
		}				
	};
};

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Exporter",	 "Exporte6",	Ee6::CExporter::createInterface, Ee6::CExporter::classRef	},
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
	mv->modVersion = e6ex_version;
	mv->e6Version =	e6::e6_version;
	return 1; 
}
