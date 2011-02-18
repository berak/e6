#include "../e6/e6_impl.h"
#include "../e6/e6_math.h"
#include "../e6/e6_enums.h"
#include "../main/SceneTree.h"
#include "resource.h"

#include <stdio.h>

using e6::uint;
using e6::float3;
using e6::float4;
using e6::float4x4;
using e6::toString;
using w32::TreeView;
using w32::PopupMenu;
using w32::TreeView;




	SceneTree::SceneTree() 
		: h_tl(0),h_tc(0),h_tf(0), mask(0)
	{}
	SceneTree::~SceneTree()
	{}
	bool SceneTree::notify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
	{	
		NMTREEVIEW *ntv= ((NM_TREEVIEW *)lParam);

		switch( ntv->hdr.code ) 
        {
			case NM_RCLICK: 
			{
				TVITEM it = getClickedItem();
				if ( it.lParam ) 
                {
					popItem = it;
					strcpy( popName, it.pszText );
					popItem.pszText = popName;
					
					switch( it.iImage ) 
                    {
						case EICO_TEX : 
						{
							w32::PopupMenu pop;
							pop.add( "Copy", IDPOP_COPY_TEX );
							//pop.add( "Load", IDM_FILE_OPEN_SCENE );
							pop.add( "View", IDM_VIEW_IMAGE );
							pop.show(hWnd);
							break;
						}
						case EICO_SHADER : 
						{
							w32::PopupMenu pop;
							pop.add( "Copy", IDPOP_COPY_SHADER );
							pop.show(hWnd);
							break;
						}
						case EICO_RENDERER : 
						{
							w32::PopupMenu pop;
							pop.add( "Edit", IDPOP_RENDERER_EDIT );
							pop.add( "Change", IDM_FILE_RENDERER );
							pop.show(hWnd);
							break;
						}
						case EICO_SCENE : 
						{
							w32::PopupMenu pop;
							pop.add( "Clear", IDM_FILE_NEW_SCENE );
							pop.add( "Load", IDM_FILE_OPEN_SCENE );
							pop.add( "Save", IDM_FILE_SAVE );
							pop.show(hWnd);
							break;
						}
						case EICO_RT : 
						case EICO_MESH : 
						case EICO_CAMERA : 
						case EICO_LIGHT : 
						case EICO_FREENODE : 
						{
							w32::PopupMenu pop;
							pop.add( "Edit", IDPOP_EDIT );
							if ( it.iImage==EICO_MESH )
							{
								pop.add( "Edit VS Const", IDPOP_EDIT_VS );
								pop.add( "Edit PS Const", IDPOP_EDIT_PS );
								pop.add( "Edit RenderStates", IDPOP_EDIT_RS );
							}
							pop.add( "Move", IDPOP_MOVE_NODE );
							if ( it.iImage==EICO_CAMERA )
							{
								pop.add( "Select", IDPOP_SELECT_CAM );
							}								
							pop.addSep();
							pop.add( "Clone Node", IDPOP_CLONE );
//							pop.add( "Save", IDPOP_SAVE );
							pop.add( "Delete Node", IDPOP_REMOVE );
							pop.addSep();
							pop.add( "Cut Node", IDPOP_CUT );
							if ( mask & CCM_NODE )
								pop.add( "Paste Node", IDPOP_PASTE );
							if ( it.iImage==EICO_MESH )
							{
								if ( mask & CCM_TEX )
									pop.add( "Paste Tex", IDPOP_PASTE_TEX );
								if ( mask & CCM_SHA )
								{
									pop.add( "Paste VShader", IDPOP_PASTE_VSHADER );
									pop.add( "Paste PShader", IDPOP_PASTE_PSHADER );
								}
							}
							pop.addSep();
							pop.add( "Create Camera", IDPOP_CREATE_CAMERA );
							pop.add( "Create Light", IDPOP_CREATE_LIGHT );
							pop.add( "Create Node", IDPOP_CREATE_NODE );
							pop.add( "Create RenderTarget", IDPOP_CREATE_RENDERTARGET );
							pop.show(hWnd);
							break;
						}
						case EICO_FILTERTABLE : 
						{
							w32::PopupMenu pop;
							pop.add( "Create", IDPOP_CREATE_FILTER );
							pop.show(hWnd);
							break;
						}
						case EICO_FILTER : 
						{
							w32::PopupMenu pop;
							pop.add( "Remove", IDPOP_REMOVE_FILTER );
							pop.show(hWnd);
							break;
						}
						case EICO_PIN_IN : 
						{
							w32::PopupMenu pop;
							pop.add( "Disconnect", IDPOP_PIN_DISCONNECT );
							pop.add( "Paste", IDPOP_PIN_PASTE );
							pop.show(hWnd);
							break;
						}
						case EICO_PIN_OUT : 
						{
							w32::PopupMenu pop;
							pop.add( "Copy", IDPOP_PIN_COPY );
							pop.show(hWnd);
							break;
						}
					}
				}
				break;
			}
			case TVN_SELCHANGED:
			{
				printf( "selchgd\n" );
				break;
 			}
			case TVN_ENDLABELEDIT:
			{	//MessageBox(0,cur.pszText,0,0);
				break;
			}
			default:
				return false;
		}
		return true;
	}


    HTREEITEM SceneTree::_insert( HTREEITEM at, const char * value, u_int icon ) 
	{
		HTREEITEM h_v = newEntry( at, 0, (char*)value,0, icon);
		return h_v;
	}
    HTREEITEM SceneTree::_insert( HTREEITEM at, const char * name, const char * value, u_int icon ) 
	{
		HTREEITEM h_k = newEntry( at, 0, (char*)name, 0, icon);
		HTREEITEM h_v = newEntry( h_k, 0, (char*)value,0, icon);
		return h_k;
	}
	HTREEITEM SceneTree::_insert( HTREEITEM at, const char * name, u_int v, u_int icon ) 
	{
		char s[100]; 
		sprintf( s, "%i", v );
		return _insert( at, name, s, icon );
	}
	
    HTREEITEM  SceneTree::insertMatrix( HTREEITEM item, const e6::float4x4 & mat, u_int icon ) 
	{
		const float * p = &mat[0];
		HTREEITEM h_n = newEntry( item, 0, "Transform", 0, icon);
		_insert( h_n, toString(p,4), icon);
		_insert( h_n, toString(p+4,4), icon);
		_insert( h_n, toString(p+8,4), icon);
		_insert( h_n, toString(p+12,4), icon);
		return h_n;
	}
    HTREEITEM  SceneTree::insertCamera( HTREEITEM item, Core::Camera * cam, u_int icon ) 
	{
//		char str[256];
		HTREEITEM h_n = insertNode( item, cam, icon );
		_insert( h_n, "fov", toString(cam->getFov()), icon);
		_insert( h_n, "nearPlane", toString(cam->getNearPlane()), icon);
		_insert( h_n, "farPlane", toString(cam->getFarPlane()), icon);
		return h_n;
	}
    HTREEITEM SceneTree::insertLight( HTREEITEM item, Core::Light * light, u_int icon ) 
	{
//		char str[256];
		HTREEITEM h_n = insertNode( item, light, icon );
		_insert( h_n, "type", toString(light->getType()), icon);
		_insert( h_n, "color", toString((float*)&(light->getColor()),3), icon);
		_insert( h_n, "dir", toString((float*)&(light->getDir()),3), icon);
		return h_n;
	}

    HTREEITEM  SceneTree::insertMesh( HTREEITEM item, Core::Mesh * mesh, u_int icon ) 
	{
//		char str[256];
		HTREEITEM h_n = insertNode( item, mesh, icon );
		_insert( h_n, "numFaces", toString(mesh->numFaces()), icon);	
		_insert( h_n, "format", toString(mesh->format()), icon);
		Core::Shader * s = mesh->getVertexShader();
		_insert( h_n, "vshader", (s?s->getName():"none"), icon);
		E_RELEASE(s);
		s = mesh->getPixelShader();
		_insert( h_n, "pshader", (s?s->getName():"none"), icon);
		E_RELEASE(s);
		for ( uint i=0; i<4; i++ )
		{
			Core::Texture * t = mesh->getTexture(i);
			if ( ! t ) break;
			_insert( h_n, "texture", t->getName(), icon);
			E_RELEASE(t);
		}
		return h_n;
	}
    HTREEITEM  SceneTree::insertTexture( HTREEITEM item, Core::Texture * tex, u_int icon ) 
	{
		if( ! tex ) return 0;
		HTREEITEM h_tt = newEntry( item, 0, (char*)tex->getName(), uint(tex), icon);
		insertRef( h_tt, tex, icon);
		_insert( h_tt, "path", tex->getPath(), icon );
		_insert( h_tt, "levels", tex->levels(), icon );
		_insert( h_tt, "width",  tex->width(), icon );
		_insert( h_tt, "height", tex->height(), icon );
		_insert( h_tt, "format", tex->format(), icon );
		_insert( h_tt, "type", (tex->type()==2?"RenderTarget":(tex->type()==1?"Dynamic":"Standard")), icon );
		return h_tt;
	}
    HTREEITEM  SceneTree::insertFreeNode( HTREEITEM item, Core::FreeNode * node, u_int icon ) 
	{
		HTREEITEM h_n = insertNode( item, node, icon );
		return h_n;
	}

	HTREEITEM  SceneTree::insertRenderToTexture( HTREEITEM item, Core::RenderToTexture * node, u_int icon ) 
	{
		HTREEITEM h_n = insertNode( item, node, icon );
		Core::Texture * tex = node->getRenderTarget();
		HTREEITEM h_t = insertTexture( h_n, tex, EICO_TEX );
		E_RELEASE( tex );
		return h_n;
	}

    HTREEITEM  SceneTree::insertRef( HTREEITEM item, Core::Base * base, u_int icon ) 
	{
		uint r = base->addref();
		r = base->release();
        return _insert( item, "refs", toString(r), icon);
	}
    HTREEITEM  SceneTree::insertNode( HTREEITEM item, Core::Node * node, u_int icon ) 
	{
//		char str[256];
		HTREEITEM h_n = newEntry( item, 0, (char*)node->typeName(), 0, icon);

		e6::float4x4 mat;
		
		node->getWorldMatrix( mat );
		insertRef( h_n, node, icon);
		insertMatrix( h_n, mat, icon );
		_insert( h_n, "radius", toString(node->getSphere()), icon);
		_insert( h_n, "visible", toString(node->getVisibility()), icon);

        return h_n;
	}
    
	HTREEITEM  SceneTree::update( HTREEITEM item, Core::Node * node, u_int icon ) 
    {
		HTREEITEM it = 0;
		const char * t = node->typeName();
		if ( ! strcmp( t, "FreeNode" ) )
		{
			it = newEntry( item, 0, (char*)node->getName(), (uint)node, EICO_FREENODE);
			insertFreeNode( it, (Core::FreeNode*)node, EICO_FREENODE );
		}
		if ( ! strcmp( t, "Camera" ) )
		{
			it = newEntry( item, 0, (char*)node->getName(), (uint)node, EICO_CAMERA);
			insertCamera( it, (Core::Camera*)node,  EICO_CAMERA  );
		}
		if ( ! strcmp( t, "Light" ) )
		{
			it = newEntry( item, 0, (char*)node->getName(), (uint)node, EICO_LIGHT);
			insertLight( it, (Core::Light*)node,  EICO_LIGHT  );
		}

		if ( ! strcmp( t, "Mesh" ) )
		{
			it = newEntry( item, 0, (char*)node->getName(), (uint)node, EICO_MESH);
			insertMesh( it, (Core::Mesh*)node,  EICO_MESH  );
		}
		if ( ! strcmp( t, "RenderToTexture" ) )
		{
			it = newEntry( item, 0, (char*)node->getName(), (uint)node, EICO_RT);
			insertRenderToTexture( it, (Core::RenderToTexture*)node,  EICO_RT  );
		}
		for ( uint i=0; i<node->numChildren(); i++ )
		{
			Core::Node * c = node->getChild(i);
			update( it, c, icon );
			E_RELEASE( c );
		}
        return it;
	}
	void SceneTree::update( Core::World * world, u_int icon ) 
    {
//		char b[300];
    
		clearAll();
			
		HTREEITEM h_w = newEntry( 0, 0, "World", uint(world), EICO_SCENE);
		Core::Node * root = world->getRoot();
		update( h_w, root, EICO_SCENE );
		E_RELEASE( root );

		HTREEITEM h_t = newEntry( h_w, 0, "Textures", 0, EICO_TEX);
		world->textures().reset();
		while ( Core::Texture * t = world->textures().next() )
		{	
			insertTexture( h_t, t, EICO_TEX );
			E_RELEASE( t );
		}

		HTREEITEM h_s = newEntry( h_w, 0, "Shaders", 0, EICO_SHADER);
		world->shaders().reset();
		while ( Core::Shader * s = world->shaders().next() )
		{
			HTREEITEM h_ss = newEntry( h_s, 0, (char*)s->getName(), uint(s), EICO_SHADER);
			insertRef( h_ss, s, EICO_SHADER);

			_insert( h_ss, "path", s->getPath(), EICO_SHADER );
			//HTREEITEM h_sc = newEntry( h_s, 0, (char*)s->getCode(), uint(s), EICO_SHADER);
			const char* code = s->getCode();
			if ( code )
			{
				_insert( h_ss, "code", code , EICO_SHADER);
			}


			E_RELEASE( s );
		}
	}
    void SceneTree::update( Core::Renderer * renderer, u_int node )
	{
		uint icon = EICO_RENDERER;
		HTREEITEM h_r = newEntry(0, 0, (char*)renderer->getName(), uint(renderer), icon);
		//~ _insert( h_r, "do_wire",  toString(renderer->get( e6::RF_WIRE )), icon);
		//~ _insert( h_r, "do_texture",  toString(renderer->get( e6::RF_TEXTURE )), icon);
		//~ _insert( h_r, "do_lighting",  toString(renderer->get( e6::RF_LIGHTING )), icon);
		//~ _insert( h_r, "do_cull",  toString(renderer->get( e6::RF_CULL )), icon);
		//~ _insert( h_r, "hw_textures",  toString(renderer->get( e6::RF_NUM_HW_TEX )), icon);
		//~ _insert( h_r, "hw_vbuffer",  toString(renderer->get( e6::RF_NUM_HW_VB )), icon);
		//~ _insert( h_r, "hw_pshader",  toString(renderer->get( e6::RF_NUM_HW_PSH )), icon);
		//~ _insert( h_r, "hw_vshader",  toString(renderer->get( e6::RF_NUM_HW_VSH )), icon);
	}

	///PPP UUUUUH
	TimeLine::TimeLine * _tl=0;
    void SceneTree::update( TimeLine::TimeLine * tl, u_int icon )
	{
		$X()
		h_tl = newEntry(0, 0, "TimeLine", uint(tl), EICO_TIMELINE);
		h_tf = _insert( h_tl, "Filters", EICO_TIMELINE );   
		FilterTable * f = _filters;
		while ( f->name )
		{
			newEntry(h_tf, 0, f->name, uint(f), EICO_FILTERTABLE);
			f ++;
		}

		h_tc = _insert( h_tl, "Graph", EICO_TIMELINE );   
		_tl = tl;
		tl->traverse( (TimeLine::Visitor*)this );
		_tl = 0;
	}

	bool SceneTree::visitFilter( const TimeLine::Filter * filter ) 
	{
		E_ASSERT( filter );
		$X();
		HTREEITEM h_f = newEntry(h_tc, 0, (char*)filter->getName(), uint(filter), EICO_FILTER);
		HTREEITEM h_i = _insert(h_f, "Inputs", EICO_PIN_IN);
		for ( uint i=0; i< filter->numInputs(); i++ )
		{
			HTREEITEM h_ip = newEntry(h_i, 0, (char*)filter->getInputName(i), uint(filter), EICO_PIN_IN);
			char outFilterName[200], outpin[200];
			if ( _tl->getConnection( filter->getName(), filter->getInputName(i), outFilterName, outpin ) )
			{
				HTREEITEM h_o =_insert(h_ip, outFilterName, EICO_FILTER);
				HTREEITEM h_op =_insert(h_o, outpin, EICO_PIN_OUT);
			}
		}
		HTREEITEM h_o = _insert(h_f, "Outputs", EICO_PIN_OUT);
		for ( uint i=0; i< filter->numOutputs(); i++ )
		{
			HTREEITEM h_op = newEntry(h_o, 0, (char*)filter->getOutputName(i), uint(filter), EICO_PIN_OUT);
			_insert(h_op, e6::toString(filter->getOutput(i)), EICO_PIN_OUT);
		}
		return 1;
	}

	//~ uint SceneTree::call( const char * interfaceName, const char * moduleName, uint count )
	//~ {
		//~ $X();
		//~ HTREEITEM h = _insert( h_tf, interfaceName, EICO_FILTER );   
		//~ _insert( h, moduleName, EICO_FILTER );   
		//~ return 1;
	//~ }


