#include "../e6/e6_sys.h"
#include "../e6/e6_prop.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../e6/e6_math.h"
#include "../Core/CoreString.h"

namespace Core
{

    //template < class Super, class Impl, class Node >
    struct CNodeString : e6::Class< NodeString, CNodeString >
    {
        Core::Node * node;

        uint init( Core::Node * n )
        {
            node =(n);
            $();
            if ( ! n )
            {
                printf( __FUNCTION__ " called wo. Node !\n" );
            }
            return 1;
        }
        enum NPROPS
        {
            NUM_PROPS = 6
        };
        virtual uint numProps() 
        {
            return NUM_PROPS;
        }
        virtual const char * getPropName(uint i)
        {
            static char * _s[] = { "name", "pos", "rot", "friction", "bounce","visible"  };
            assert(i<NUM_PROPS);
            return _s[i];
        }
        virtual const char * getPropValue(uint i)
        {
            assert(i<NUM_PROPS);
            assert(node);
            switch( i )
            {
                case 0:	return node->getName();
                case 1:	return e6::toString( node->getPos() );
                case 2:	return e6::toString( node->getRot() );
                //~ case 3:	return e6::toString( node->getScale() );
                case 3:	return e6::toString( node->getFriction() );
                case 4:	return e6::toString( node->getBounce() );
                case 5:	return e6::toString( node->getVisibility() );
            }
            return 0;
        }
        virtual const char * getPropType(uint i)
        {
            static char * _s[] = { "s", "f3", "f3", "f", "f", "i"  };
            assert(i<NUM_PROPS);
            return _s[i];
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            assert(i<NUM_PROPS);
            assert(node);
            switch( i )
            {
                case 0:	return (node->setName(s)>0);
                case 1:	node->setPos( e6::stringToFloat3(s) ); return 1;
                case 2:	node->setRot( e6::stringToFloat3(s) ); return 1;
                //~ case 3:	node->setScale( e6::stringToFloat3(s) ); return 1;
                case 3:	node->setFriction( e6::stringToFloat(s) ); return 1;
                case 4:	node->setBounce( e6::stringToFloat(s) ); return 1;
                case 5:	node->setVisibility( e6::stringToInt(s) ); return 1;
            }
            return 0;
        }
    };



    struct CMeshString : e6::Class< MeshString, CMeshString > 
    {
        CNodeString ns;
        Core::Mesh * mesh;
        uint size_node;
        uint size_mesh;
        
        uint init( Core::Mesh * m )
        {
            ns.init(m);
            mesh = (m);
            $();
            assert( m );
            size_node = ns.numProps();
            size_mesh = size_node + 2;		
            return 1;
        }
        virtual uint numProps()
        {
            return size_mesh;
        }
        virtual const char * getPropName(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropName(i);

            static char * _s[] = { "format", "nfaces"  };
            return _s[i-size_node];
        }
        virtual const char * getPropValue(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropValue(i);

            switch(i-size_node)
            {
                case 0: return e6::toString( mesh->format() );
                case 1: return e6::toString( mesh->numFaces() );
            }
            return 0;
        }
        virtual const char * getPropType(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropType(i);

            static char * _s[] = { "i", "i"  };
            return _s[i-size_node];
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.setPropValue(i,s);
            return 0;
        }
    };

    struct CMeshVSString : e6::Class< MeshVSString, CMeshVSString > 
    {
        Core::Mesh * mesh;
        uint count;
        uint off;
        uint nc;
        
        uint init( Core::Mesh * m )
        {
            //$();           
            assert( m );
            mesh = (m);
            nc = mesh->getNumVertexShaderConstants();
            count = 32;
            off   = 12;
            return 1;
        }
        virtual uint numProps()
        {
            nc = mesh->getNumVertexShaderConstants();
            return count;
        }
        virtual const char * getPropName(uint i)
        {
            if(i>=count) return 0;
            static char _s[100];
            sprintf( _s, "vs_%i",i+off);
            return _s;
        }
        virtual const char * getPropValue(uint i)
        {
            if(i>=count) return 0;
            for ( uint j=0; j<nc; j++ )
            {
                Core::Shader::Constant *c = mesh->getVertexShaderConstant(j);
                if ( c->index == i+off )
                    return e6::toString( c->value );
            }
            return "";
        }
        virtual const char * getPropType(uint i)
        {
            return "f4";
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            if(i>=count) return 0;
            mesh->setVertexShaderConstant(i+off, e6::stringToFloat4(s));
            return 1;
        }
    };

    struct CMeshPSString : e6::Class< MeshPSString, CMeshPSString > 
    {
        Core::Mesh * mesh;
        uint count;
        uint nc;
        uint off;
        
        uint init( Core::Mesh * m )
        {
            // $();           
            assert( m );
            mesh = (m);
            nc = mesh->getNumPixelShaderConstants();
            count = 32;
            off   = 0;
            return 1;
        }
        virtual uint numProps()
        {
            nc = mesh->getNumPixelShaderConstants();
            return count;
        }
        virtual const char * getPropName(uint i)
        {
            if(i>=count) return 0;
            static char _s[100];
            sprintf( _s, "ps_%i",i+off);
            return _s;
        }
        virtual const char * getPropValue(uint i)
        {
            if(i>=count) return 0;
            for ( uint j=0; j<nc; j++ )
            {
                Core::Shader::Constant *c = mesh->getPixelShaderConstant(j);
                if ( c->index == i+off )
                    return e6::toString( c->value );
            }
            return "";
        }
        virtual const char * getPropType(uint i)
        {
            return "f4";
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            if(i>=count) return 0;
            mesh->setPixelShaderConstant(i+off, e6::stringToFloat4(s));
            return 1;
        }
    };

    struct CMeshRSString : e6::Class< MeshRSString, CMeshRSString > 
    {
        Core::Mesh * mesh;
        uint count;
        
        uint init( Core::Mesh * m )
        {
            // $();           
            assert( m );
            mesh = (m);
            count = e6::RF_MAX;
            return 1;
        }
        virtual uint numProps()
        {
            return count;
        }
        virtual const char * getPropName(uint i)
        {
            if(i>=count) return 0;
            static char _s[100];
            sprintf( _s, "rs_%i",i);
            return _s;
        }
        virtual const char * getPropValue(uint i)
        {
            if(i>=count) return 0;
            uint nc = mesh->getNumRenderStates();
            for ( uint j=0; j<nc; j++ )
            {
                Core::Shader::RenderState *c = mesh->getRenderState(j);
                if ( c->index == i )
                    return e6::toString( c->value );
            }
            return "";
        }
        virtual const char * getPropType(uint i)
        {
            return "i";
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            if(i>=count) return 0;
            mesh->setRenderState(i, e6::stringToInt(s));
            return 1;
        }
    };


    struct CEffectPassesString : e6::Class< EffectPassesString, CEffectPassesString > 
    {
        Core::Effect * effect;
        uint nc;
        
        uint init( Core::Effect * e )
        {
            //$();           
            assert( e );
            effect = (e);
			nc = effect->numPasses();
            return 1;
        }
        virtual uint numProps()
        {
            nc = effect->numPasses();
            return nc;
        }
        virtual const char * getPropName(uint i)
        {
            if(i>=nc) return 0;
			static char *_s = 0, *_d=0;
			effect->getPass( i, &_s, &_d, &_d );
            return _s;
        }
        virtual const char * getPropValue(uint i)
        {
            if(i>=nc) return 0;
			static char *_s = 0, *_d=0;
			effect->getPass( i, &_s, &_d, &_d );
			return _d;
        }
        virtual const char * getPropType(uint i)
        {
            return "s";
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            return 0;
        }
    };

	struct CEffectParamsString : e6::Class< EffectParamsString, CEffectParamsString > 
    {
        Core::Effect * effect;
        uint nc;
        
        uint init( Core::Effect * e )
        {
            //$();           
            assert( e );
            effect = (e);
			nc = effect->numParams();
            return 1;
        }
        virtual uint numProps()
        {
            nc = effect->numParams();
            return nc;
        }
        virtual const char * getPropName(uint i)
        {
            if(i>=nc) return 0;
			static char *_s = 0;
			e6::float4x4 mat;
			effect->getParam( i, &_s, mat );
            return _s;
        }
        virtual const char * getPropValue(uint i)
        {
            if(i>=nc) return 0;
			static char *_s = 0;
			e6::float4x4 mat;
			effect->getParam( i, &_s, mat );
			return e6::toString( float4(mat) );
        }
        virtual const char * getPropType(uint i)
        {
            return "f4";
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            return 0;
        }
    };

	struct CCameraString : e6::Class< CameraString, CCameraString > 
    {
        CNodeString ns;
        Core::Camera * cam;
        uint size_node;
        uint size_mesh;
        
        uint init( Core::Camera * m )
        {
            ns.init(m);
            cam = (m);
            // $();
            assert( m );
            size_node = ns.numProps();
            size_mesh = size_node + 3;		
            return 1;
        }
        virtual uint numProps()
        {
            return size_mesh;
        }
        virtual const char * getPropName(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropName(i);

            static char * _s[] = { "fov", "nearPlane", "farPlane"  };
            return _s[i-size_node];
        }
        virtual const char * getPropValue(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropValue(i);

            switch(i-size_node)
            {
                case 0: return e6::toString( cam->getFov() );
                case 1: return e6::toString( cam->getNearPlane() );
                case 2: return e6::toString( cam->getFarPlane() );
            }
            return 0;
        }
        virtual const char * getPropType(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropType(i);

            static char * _s[] = { "f", "f", "f"  };
            return _s[i-size_node];
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.setPropValue(i,s);
            switch(i-size_node)
            {
                case 0: cam->setFov( e6::stringToFloat(s)  ); return 1;
                case 1: cam->setNearPlane( e6::stringToFloat(s)  ); return 1;
                case 2: cam->setFarPlane( e6::stringToFloat(s)  ); return 1;
            }
            return 0;
        }
    };

    struct CLightString : e6::Class< LightString, CLightString > 
    {
        CNodeString ns;
        Core::Light * light;
        uint size_node;
        uint size_mesh;
        
        uint init( Core::Light * m )
        {
            // $();
            ns.init(m);
            light = m;
            assert( m );
            size_node = ns.numProps();
            size_mesh = size_node + 3;		
            return 1;
        }
        virtual uint numProps()
        {
            return size_mesh;
        }
        virtual const char * getPropName(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropName(i);

            static char * _s[] = { "type", "color", "dir"  };
            return _s[i-size_node];
        }
        virtual const char * getPropValue(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropValue(i);

            switch(i-size_node)
            {
                case 0: return e6::toString( light->getType() );
                case 1: return e6::toString( light->getColor() );
                case 2: return e6::toString( light->getDir() );
            }
            return 0;
        }
        virtual const char * getPropType(uint i)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.getPropType(i);

            static char * _s[] = { "i", "f3", "f3"  };
            return _s[i-size_node];
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            if(i>=size_mesh) return 0;
            if(i<size_node) return ns.setPropValue(i,s);
            switch(i-size_node)
            {
                case 0: light->setType( e6::stringToInt(s)  ); return 1;
                case 1: light->setColor( e6::stringToFloat3(s)  ); return 1;
                case 2: light->setDir( e6::stringToFloat3(s)  );  return 1;
            }
            return 0;
        }
    };

    struct RSP
    {
        const char *name;
        uint key;
    };
    RSP _rsp[] = {
         {"do_wire",e6::RF_WIRE}
        ,{"do_texture",e6::RF_TEXTURE}
        ,{"do_lighting",e6::RF_LIGHTING}
        ,{"do_cull",e6::RF_CULL}
        ,{"do_ccw",e6::RF_CCW}
        ,{"do_alpha",e6::RF_ALPHA}
        ,{"do_alphatest",e6::RF_ALPHA_TEST}
        ,{"do_ztest",e6::RF_ZTEST}
        ,{"do_zwrite",e6::RF_ZWRITE}
        ,{"do_clear",e6::RF_CLEAR_BUFFER}
        ,{"zb_depth",e6::RF_ZDEPTH}
        ,{"shading",e6::RF_SHADE}
        ,{"blend_src",e6::RF_SRCBLEND}
        ,{"blend_dst",e6::RF_DSTBLEND}
        ,{"bg_col",e6::RF_CLEAR_COLOR}
        ,{"ttl",e6::RF_TTL}
        ,{"sh_norms",e6::RF_NORMALS}
        ,{"hw_tex",e6::RF_NUM_HW_TEX}
        ,{"hw_vb",e6::RF_NUM_HW_VB}
        ,{"hw_vsh",e6::RF_NUM_HW_VSH}
        ,{"hw_psh",e6::RF_NUM_HW_PSH}
    };
    enum RSP_NUM
    {
        RSP_SIZE = 21
    };

    struct CRenderString : e6::Class< RenderString, CRenderString >
    {
        Core::Renderer * renderer;
        uint init( Core::Renderer * re )
        {
            renderer = (re);
            // $();
            return 1;
        }
        virtual uint numProps()
        {
            return RSP_SIZE+1;
        }
        virtual const char * getPropName(uint i)
        {
            if ( !i ) return "name";
            assert(i<RSP_SIZE+1);
            return _rsp[i-1].name;
        }
        virtual const char * getPropValue(uint i)
        {
            assert(renderer);
            if ( !i ) return renderer->getName();
            assert(i<RSP_SIZE+1);
			uint v = renderer->get(_rsp[i-1].key);
			static char b[100];
			char *fmt = ( (v>0xff) ? "0x%08x" : "%i" );
			sprintf( b, fmt, v );
			return b;
            //return e6::toString( renderer->get(_rsp[i-1].key) );
        }
        virtual const char * getPropType(uint i)
        {
            if ( !i ) return "s";
            return "u";
        }
        virtual bool setPropValue(uint i, const char *s)
        {
            assert(renderer);
            if ( !i ) return (renderer->setName(s)>0);
            assert(i<RSP_SIZE+1);
            uint n=0;
            if ( ! sscanf(s,"%i",&n ) )
                return 0;
			uint r = renderer->setRenderState( _rsp[i-1].key, n ); 
            return r!=0;
        }
    };

} // Core

e6::ClassInfo ClassInfo_NodeString =
	{	"Core.NodeString",	 "Core",	Core::CNodeString::createInterface, Core::CNodeString::classRef	};
e6::ClassInfo ClassInfo_MeshString =
	{	"Core.MeshString",	 "Core",	Core::CMeshString::createInterface, Core::CMeshString::classRef	};
e6::ClassInfo ClassInfo_MeshVSString =
	{	"Core.MeshVSString",	 "Core",	Core::CMeshVSString::createInterface, Core::CMeshVSString::classRef	};
e6::ClassInfo ClassInfo_MeshPSString =
	{	"Core.MeshPSString",	 "Core",	Core::CMeshPSString::createInterface, Core::CMeshPSString::classRef	};
e6::ClassInfo ClassInfo_MeshRSString =
	{	"Core.MeshRSString",	 "Core",	Core::CMeshRSString::createInterface, Core::CMeshRSString::classRef	};
e6::ClassInfo ClassInfo_CameraString =
	{	"Core.CameraString",	 "Core",	Core::CCameraString::createInterface, Core::CCameraString::classRef	};
e6::ClassInfo ClassInfo_LightString =
	{	"Core.LightString",	 "Core",	Core::CLightString::createInterface, Core::CLightString::classRef	};
e6::ClassInfo ClassInfo_RenderString =
	{	"Core.RenderString",	 "Core",	Core::CRenderString::createInterface, Core::CRenderString::classRef	};
