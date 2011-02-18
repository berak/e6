
#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../e6/e6_str.h"
#include "../e6/e6_arr.h"
#include "../e6/e6_math.h"
#include "../e6/e6_enums.h"
#include "../e6/e6_container.h"

#include "../Core/Core.h"

#include <vector>

using e6::rgba;
using e6::uint;
using e6::Class;
using e6::CName;
using e6::ClassInfo;

namespace Core 
{





	template < class Super, class Impl >
    struct CResource
		: public e6::CName< Super, Impl > 
    {
		e6::CString _path;

		CResource() { }
		virtual ~CResource() {}

		virtual const char * getPath() const  { return _path.get(); }
		virtual uint setPath( const char * s) { return _path.set(s); }

    };

	struct CEffect
		: e6::CName< Effect, CEffect > 
	{

		struct Pass
		{
			e6::CString name, vs, ps;

			Pass() {}

			Pass( const char * name,  const char * vs, const char * ps ) 
			{
				this->name.set( name );
				this->vs.set( vs );
				this->ps.set( ps );
			}
		};
		struct Param
		{
			e6::float4x4 f44;
			e6::CString name;

			Param()					{}
			Param(const char * n)					  { name.set(n); }
			Param(const char * n, uint i)			  { name.set(n); set(i); }
			Param(const char * n, float f)			  { name.set(n); set(f); }
			Param(const char * n, const e6::float4 & f4)	  { name.set(n); set(f4); }
			Param(const char * n, const e6::float4x4 & f44) { name.set(n); set(f44); }

			void set( uint  i ) {  f44.m00 = (float)i; }
			void set( float  f ) { f44.m00 = f; }
			void set( const e6::float4 & f4 ) {  f44.m00 = f4[0]; f44.m01 = f4[1]; f44.m02 = f4[2]; f44.m03 = f4[3]; }
			void set( const e6::float4x4 & f44 ) { this->f44 = f44; }

			void get( uint & i ) { i=int(f44.m00); }
			void get( float & f ) { f=f44.m00; }
			void get( e6::float4 & f4 ) { f4 = e6::float4( this->f44 ); }
			void get( e6::float4x4 & f44 ) { f44 = this->f44; }
		};

		std::vector< Pass * > passes;
		typedef std::map< const char*, Param *, e6::StrLess > ParamMap;
		ParamMap params;

		virtual ~CEffect()
		{
			for ( uint  i=0; i<passes.size(); i++ )
			{
				Pass * p = passes[i];
				delete p;
			}
			for ( ParamMap::iterator it = params.begin(); it!=params.end(); it++ )
			{
				Param * p = it->second;
				delete p;
			}
		}
		virtual uint numPasses() const 
		{
			return passes.size();
		}

		virtual uint getPass( uint i, char ** name,  char ** vs, char ** ps ) const
		{
			*name = (char*)passes[i]->name.get();
			*vs = (char*)passes[i]->vs.get();
			*ps = (char*)passes[i]->ps.get();
			return 1;
		}
		virtual uint addPass( const char * name,  const char * vs, const char * ps ) 
		{
			passes.push_back( new Pass( name, vs, ps ) );
			return 1;
		}

		// nonvirtual helper
		Param * getParam( const char * name ) const 
		{
			ParamMap::const_iterator it = params.find( name );
			if ( it!=params.end() )
			{
				return it->second;
			}
			return 0;
		}
		Param * getOrCreateParam( const char * name )
		{
			Param * p = getParam(name);
			if ( ! p )
			{
				p = new Param(name);
			}
			params[ p->name.get() ] = p;
			return p;
		}
		virtual uint numParams() const 
		{
			return params.size();
		}
		virtual uint getParam( uint i, char ** name, float4x4 & v ) const 
		{
			uint np = params.size();
			if ( i >= np ) return 0;

			Param * p = 0;
			ParamMap::const_iterator it = params.begin();
			for ( uint j=0; j<i; j++ )
			{
				it ++;
			}
			p = it->second;
			if ( p )
			{
				*name = (char*)p->name.get();
				p->get( v );
				return 1;
			}
			return 0;
		}

		virtual uint setParam( const char * name, uint v ) 
		{
			Param * p = getOrCreateParam(name);
			p->set(v);
			return 1;
		}
		virtual uint setParam( const char * name, const float4 & v ) 
		{
			Param * p = getOrCreateParam(name);
			p->set(v);
			return 1;
		}
		virtual uint setParam( const char * name, const float4x4 & v )
		{
			Param * p = getOrCreateParam(name);
			p->set(v);
			return 1;
		}

		virtual uint getParam( const char * name, uint &v )  const
		{
			Param * p = getParam(name);
			if ( p )
			{
				p->get(v);
				return 1;
			}
			return 0;
		}
		virtual uint getParam( const char * name, float4 & v )  const
		{
			Param * p = getParam(name);
			if ( p )
			{
				p->get(v);
				return 1;
			}
			return 0;
		}
		virtual uint getParam( const char * name, float4x4 & v ) const
		{
			Param * p = getParam(name);
			if ( p )
			{
				p->get(v);
				return 1;
			}
			return 0;
		}
	};



	//------------- ------------------ ----------------- --------------
	//
	// CBufferArray
	//
	//------------- ------------------ ----------------- --------------

	template < class T, int _T >
	struct CBufferArray 
		: Class< Core::BufferArray, CBufferArray<T,_T>  >
		, e6::Array<T> 
	{
		bool locked;
		
		CBufferArray(uint n) 
			: e6::Array<T>(n) 
			, locked(0)
		{}
		virtual ~CBufferArray() 
		{}

		virtual uint lock( void ** content, Buffer::LockInfo * inf ) 
		{ 
			if ( locked )
				return 0; 
			locked = 1;
			if ( content )
			{
				*content = elem(0);
			}
			return locked;
		}

		virtual uint unlock() 
		{ 
			if ( ! locked )
				return 0; 
			locked = 0;
			return 1;
		}
					
		virtual uint type() 
		{
			return _T;
		}
		virtual uint esize()			//!< elem-size in bytes
		{
			return sizeof(T);
		}
		virtual uint numElements()
		{
			return e6::Array<T>::count();
		}
		virtual uint grow( uint by )	//!<  grow by n elems
		{
			if ( by )
				e6::Array<T>::grow(by);
			return e6::Array<T>::count();
		}
		virtual void * getElement( uint vi ) 
		{
			return (void*)e6::Array<T>::elem(vi);
		}
		virtual void setElement( uint vi, void *vp, uint n=1 ) 
		{
			e6::Array<T>::setElemData(vp,vi,n);
		}
	};

	//------------- ------------------ ----------------- --------------
	//
	// CTexture
	//
	//------------- ------------------ ----------------- --------------

	struct CTexture 
		: CResource< Texture , CTexture >
	{
		BufferArray * pixel;
		uint w,h,l,f,t;

		CTexture() 
			: pixel(0) //new CBufferArray<rgba,e6::ET_RGBA>(0))
			, w(0),h(0),l(0),f(0),t(0)
		{
			_name.setUnique( "Texture" );
		}
		~CTexture()
		{
			E_RELEASE(pixel);
		}

		// Texture
		virtual uint width () const { return w; }     
		virtual uint height() const { return h; }    
		virtual uint levels() const { return l; }   //! returns numSurfaces  
		virtual uint format() const { return f; }   
		virtual uint type()   const { return t; }   

		struct rgb
		{
			char r,g,b;
		};
		typedef unsigned char _F16[2];
		struct F16_r
		{
			_F16 r;
		};
		struct F16_gr
		{
			_F16 g,r;
		};
		struct F16_abgr
		{
			_F16 a,b,g,r;
		};

		//virtual uint setRenderTarget( uint fmt, uint w, uint h, uint l ) 
		//{ 
		//	this->t = e6::TU_RENDERTARGET;
		//	// copy params
		//	this->f = fmt;
		//	this->w = w;
		//	this->h = h;
		//	this->l = l;

		//	_name.setUnique( "RenderTarget" );
		//	// clear
		//	E_RELEASE( pixel );

		//	return 1;
		//}


		virtual uint alloc( uint fmt, uint w, uint h, uint l, uint t ) 
		{ 
			//~ printf( "%s (%i %i %i %i)\n", __FUNCTION__, fmt,w,h,l );

			// clear record:
			E_RELEASE( this->pixel );
			this->f = this->w = this->h = this->l = this->t = 0;

			uint nPix = w * h * l;
			if ( ! nPix ) 
			{
				return 0; // no pixels(but cleared!).
			}

			if ( fmt == e6::PF_NONE ) fmt = e6::PF_X8B8G8R8;

			this->f = fmt;
			this->w = w;
			this->h = h;
			this->l = l;
			this->t = t;

			//// no software-buffer for video ??
			if ( t == e6::TU_VIDEO )
			{
				_name.setUnique( "VideoTex" );
			}
			// no software-buffer for RenderTarget .
			if ( t == e6::TU_RENDERTARGET )
			{
				_name.setUnique( "RenderTarget" );
				return nPix;
			}

			// alloc pixels:
			switch( this->f )
			{
				case e6::PF_R16F:
					{
						pixel = new CBufferArray<F16_r,e6::ET_F16_1>( nPix );
						break;
					}
				case e6::PF_B8G8R8:	
					{
						pixel = new CBufferArray<rgb,e6::ET_RGB>( nPix );
						break;
					}

				//case e6::PF_NONE:
				case e6::PF_A8B8G8R8:
				case e6::PF_X8B8G8R8:
				//default:			
					{
						pixel = new CBufferArray<rgba,e6::ET_RGBA>( nPix );
						break;
					}

				case e6::PF_R32F:
					{
						pixel = new CBufferArray<float,e6::ET_FLOAT>( nPix );
						break;
					}
				case e6::PF_G32R32F:	
					{
						pixel = new CBufferArray<e6::float2,e6::ET_FLOAT2>( nPix );
						break;
					}
				case e6::PF_G16R16F:	
					{
						pixel = new CBufferArray<F16_gr,e6::ET_F16_2>( nPix );
						break;
					}
				case e6::PF_A16B16G16R16:
					{
						pixel = new CBufferArray<F16_abgr,e6::ET_F16_4>( nPix );
						break;
					}

				case e6::PF_A32B32G32R32F:
					{
						pixel = new CBufferArray<e6::float4,e6::ET_FLOAT4>( nPix );
						break;
					}
				default:
					printf( __FUNCTION__ " : ERROR : unknown pixel format %x !\n", this->f );
			}
			return nPix ; 
		}   

		virtual Buffer * getLevel( uint l ) const
		{
			E_ADDREF(pixel);
			return pixel; 
		}     		

	};

			
	//------------- ------------------ ----------------- --------------
	//
	// CNode
	//
	//------------- ------------------ ----------------- --------------

			template < class I, class T >
			struct CNode
				: CName< I , T >
			{
				float3   pos;
				float3   rot;
//				float3   scale;
	
				float4x4 world;
				float4x4 local;

				uint visible;
				float bounce;
				float friction;
				float radius;
//				AAbb box;
				
				Node * prnt;
				std::vector<Node*> children;
				typedef typename std::vector<Node*>::iterator tciter;
				
				CNode () 
					: prnt(0)
					, children(0)
					, visible(1)
					, radius(1)
					//, box(1)
					, bounce(0.5)
					, friction(0.5) 
//					, scale(1,1,1)
				{
				}

				virtual ~CNode () 
				{
					for ( tciter it=children.begin(); it!=children.end(); ++it )
					{
						E_RELEASE(*it);
					}
				}

				virtual uint clearChildren() 
				{
					for ( tciter it=children.begin(); it!=children.end(); ++it )
					{
						E_RELEASE(*it);
					}
					children.clear();
					return 1;
				}
				
				virtual Node * findRecursive( const char *name )
				{
					if ( ! strcmp(name, getName() ) )
					{
						E_ADDREF(this);
						return this;
					}
					for ( tciter it=children.begin(); it!=children.end(); ++it )
					{
						Node * n = (*it)->findRecursive(name);
						if ( n ) return n;
					}
					return 0;
				}
				
				virtual uint numChildren() const 
				{
					return children.size();
				}
				virtual uint getVisibility() const 
				{
					return visible;
				}
				virtual void setVisibility(uint v) 
				{
					visible = v;
				}
				
				virtual const float3 & getPos()  const 
				{
					return pos;
				}
				virtual void setPos(const float3 &p) 	
				{
					pos = p;
				}
				virtual void addPos(const float3 &p) 	
				{
					pos += p;
				}
				virtual const float3 & getRot()  const 	
				{
					return rot;
				}
				virtual void setRot(const float3 &r) 	
				{
					rot = r;
				}
				virtual void addRot(const float3 &r) 	
				{
					rot += r;
				}
				//~ virtual const float3 & getScale()  const 	
				//~ {
					//~ return scale;
				//~ }
				//~ virtual void setScale(const float3 &r) 	
				//~ {
					//~ scale = r;
				//~ }
				//~ virtual void addScale(const float3 &r) 	
				//~ {
					//~ scale += r;
				//~ }
				virtual void getLocalMatrix( float4x4 & mat )  const 
				{
					mat = local;
				}
				virtual void getWorldMatrix( float4x4 & mat )  const 
				{
					mat = world;
				}

				virtual uint link( Node * child ) 
				{
					E_ADDREF(child);
					children.push_back(child);
					child->setParent( this );
					return 1; 
				}
				virtual uint unlink( Node * child ) 
				{
					for ( tciter ti=children.begin(); ti!=children.end(); ++ti )
					{
						if ( child == *ti )
						{
							children.erase(ti);
							child->setParent(0);
							E_RELEASE(child);
							return 1;
						}
					}
					return 0;
				}

				virtual void setParent( Node * p )
				{
					prnt = p;
				}

				virtual Node * getParent()  const 
				{
					E_ADDREF(prnt);
					return prnt; 
				}
				
				virtual Node * getChild( uint i )  const 
				{
					if ( i >= children.size() )
					{
						return 0;
					}
					E_ADDREF(children[i]);
					return children[i]; 
				}
				
				virtual void synchronize()
				{
					local.identity(); //(float4x4(scale.x,scale.y,scale.z);
					if ( fabs(rot.x) > e6::epsilon )
						local.rotateX(rot.x);
					if ( fabs(rot.y) > e6::epsilon )
						local.rotateY(rot.y);
					if ( fabs(rot.z) > e6::epsilon )
						local.rotateZ(rot.z);
					local.setPos(pos);
					//printf( "%s local\n", _name.get() );
					//print( local );
					if ( prnt )
					{
						float4x4 pw;
						prnt->getWorldMatrix( pw );
						//~ world = pw * local;
						world = local * pw;
					//printf( "%s parent\n", _name.get() );
					//print( pw );
					}
					else
					{
						world = local;
					}
					//~ printf( "%s world\n", _name.get() );
					//~ print( world );
					
					for ( tciter it=children.begin(); it!=children.end(); ++it )
					{
						(*it)->synchronize();
					}
				}

				virtual float getBounce() const 
				{
					return bounce;
				}
				virtual void setBounce(float b) 
				{
					bounce = b;
				} 

				virtual float getFriction() const
				{
					return friction;
				}
				virtual void setFriction(float f)
				{
					friction = f;
				} 
				//virtual const AAbb * getBox()
				//{
				//	return 0; //&box;
				//}

				virtual void  setSphere(float r)
				{
					radius = r;
					//box = AAbb(r);
				} 
				virtual float getSphere() const
				{
					return radius;
				}
				virtual void scaleBy( float factor )
				{
					pos *= factor;
					radius *= factor;
					for ( tciter it=children.begin(); it!=children.end(); ++it )
					{
						(*it)->scaleBy( factor );
					}
				}
				virtual float recalcSphere()
				{
					float rmax=radius;
					for ( tciter it=children.begin(); it!=children.end(); ++it )
					{
						float r = (*it)->recalcSphere();
						float d = (pos + (*it)->getPos()).length();
						if ( r+d > rmax ) rmax = r+d;
					}
					radius = rmax;
					//box = AAbb(rmax);
					return radius;
				}				

				uint copyNode( Node * to )
				{
					if ( ! prnt )
					{
						return 0;
					}
					prnt->link( to );
					to->setName( _name.clone() );
					to->setPos( pos + float3(radius,0,0) );					
					to->setRot( rot );					
					to->setVisibility( visible );					
					to->setFriction( friction );					
					to->setBounce( bounce );					
					to->setSphere( radius );			
					return 1;
				}											
			};
	
	
	//------------- ------------------ ----------------- --------------
	//
	// CVertexBuffer
	//
	//------------- ------------------ ----------------- --------------

			struct CVertexBuffer 
				: CName< Core::VertexBuffer, CVertexBuffer >
			{

				VertexFeature *_pos;
				VertexFeature *_nor;
				VertexFeature *_col;
				VertexFeature *_uv0;
				VertexFeature *_uv1;
				VertexFeature *_uv2;
				VertexFeature *_tan;

				uint	  _format;
				uint	  _numFaces;

				CVertexBuffer()
					: _pos(0)
					, _nor(0)
					, _col(0)
					, _uv0(0)
					, _uv1(0)
					, _uv2(0)
					, _tan(0)
					, _format(0)
					, _numFaces(0)
				{
				}

				virtual ~CVertexBuffer()
				{
					E_RELEASE( _pos );
					E_RELEASE( _nor );
					E_RELEASE( _col );
					E_RELEASE( _uv0 );
					E_RELEASE( _uv1 );
					E_RELEASE( _uv2 );
					E_RELEASE( _tan );
				}
				
				virtual uint setup( uint fmt, uint nFaces ) 
				{
					int diff = 0;
					if ( _numFaces )
						diff = 3 * nFaces - _pos->numElements();
					
					_numFaces = nFaces;
					_format = fmt;

					if ( diff < 0 ) 
						return 1;
					// if ( fmt & e6::VF_POS )
					{
						if ( !_pos )
							_pos = new CBufferArray< float3, e6::ET_FLOAT3 >(nFaces * 3);
						_pos->grow( diff );
					}
					if ( fmt & e6::VF_NOR )
					{
						if( !_nor )
							_nor = new CBufferArray< float3, e6::ET_FLOAT3 >(nFaces * 3);
						_nor->grow( diff );
					} else E_RELEASE( _nor );

					if ( fmt & e6::VF_COL )
					{
						if ( ! _col )
							_col = new CBufferArray< rgba, e6::ET_RGBA >(nFaces * 3);
						_col->grow( diff );
					} else E_RELEASE( _col );

					if ( fmt & e6::VF_UV0 )
					{
						if ( !_uv0 )
							_uv0 = new CBufferArray< float2, e6::ET_FLOAT2 >(nFaces * 3);
						_uv0->grow( diff );
					} else E_RELEASE( _uv0 );

					if ( fmt & e6::VF_UV1 )
					{
						if ( ! _uv1 )
							_uv1 = new CBufferArray< float2, e6::ET_FLOAT2 >(nFaces * 3);
						_uv1->grow( diff );
					} else E_RELEASE( _uv1 );

					if ( fmt & e6::VF_UV2 )
					{
						if( !_uv2 )
							_uv2 = new CBufferArray< float2, e6::ET_FLOAT2 >(nFaces * 3);
						_uv2->grow( diff );
					} else E_RELEASE( _uv2 );

					if ( fmt & e6::VF_TAN )
					{
						if( !_tan )
							_tan = new CBufferArray< float3, e6::ET_FLOAT3 >(nFaces * 3);
						_tan->grow( diff);
					} else E_RELEASE( _tan );

					return 1;
				}
				virtual VertexFeature * getFeature( uint id ) const
				{
					VertexFeature * vf = 0;
					switch( id )
					{
						case e6::VF_POS:
							vf = _pos;
							break;
						case e6::VF_NOR:
							vf = _nor;
							break;
						case e6::VF_COL:
							vf = _col;
							break;
						case e6::VF_UV0:
							vf = _uv0;
							break;
						case e6::VF_UV1:
							vf = _uv1;
							break;
						case e6::VF_UV2:
							vf = _uv2;
							break;
						case e6::VF_TAN:
							vf = _tan;
							break;
					}
					E_ADDREF(vf);
					return vf;
				}
				virtual uint format() const  
				{
					return _format;
				}
				virtual uint numFaces() const
				{
					return _numFaces;
				}

				virtual uint recalcNormals(float smooth)
				{
					E_ASSERT( _nor && (_format & e6::VF_NOR) ); 

					uint nVerts = _pos->numElements();
					for ( uint i=0; i<nVerts; i+=3 )
					{
						float3 v0( (float*)_pos->getElement(i)   );
						float3 v1( (float*)_pos->getElement(i+1) );
						float3 v2( (float*)_pos->getElement(i+2) );

						float3 e0 = v1-v0;
						float3 e1 = v2-v0;
						float3 n = e0.cross(e1);

						_nor->setElement( i  , (float*)&n );
						_nor->setElement( i+1, (float*)&n );
						_nor->setElement( i+2, (float*)&n );
					}

					if ( smooth > 0.01f )
					{
						for ( uint i=0; i<nVerts; i++ )
						{
							float3 v0( (float*)_pos->getElement(i) );
							float3 n0( (float*)_nor->getElement(i) );
							for ( uint j=0; j<nVerts && j!=i ; j++ )
							{
								float3 v1( (float*)_pos->getElement(j) );
								if ( v1 == v0 )
								{
									float3 n1( (float*)_nor->getElement(j) );
									n0 += n1 * smooth;
								}
							}
							_nor->setElement( i  , (float*)&n0 );
						}
					}

					for ( uint i=0; i<nVerts; i++ )
					{
						float3 n( (float*)_nor->getElement(i) );
						n.normalize();						
						_nor->setElement( i, (float*)&n );
					}

					return 1;
				}

				virtual uint recalcTangents()
				{
					if ( ! _tan )
					{
						_tan = new CBufferArray< float3, e6::ET_FLOAT3 >(_numFaces * 3);
						_format += e6::VF_TAN;
					}
					uint idx = 0;
					for ( uint i = 0; i < _numFaces; i++, idx+=3 )
					{
						float3 & A   = *(float3 *) _pos->getElement( idx+0 );
						float3 & B   = *(float3 *) _pos->getElement( idx+1 );
						float3 & C   = *(float3 *) _pos->getElement( idx+2 );
						///PPP??? _uv1 or 0 ???
						float2 & uvA = *(float2 *) _uv0->getElement( idx+0 );
						float2 & uvB = *(float2 *) _uv0->getElement( idx+1 );
						float2 & uvC = *(float2 *) _uv0->getElement( idx+2 );

						float2 duv21 = uvB - uvA;            			
						float2 duv31 = uvC - uvA;
						float2 duv23 = uvB - uvC;
						float3 d21 = B - A;
						float3 d31 = C - A;
						float3 d23 = B - C;

						float infDet = 1.0f / (duv21.x * duv31.y - duv31.x * duv21.y);
						float3 T = infDet * ( d21 * duv31.y - d31 * duv21.y );
						T.normalize();
						_tan->setElement( idx+0, &T );

						infDet = 1.0f / (duv21.x * duv23.y - duv23.x * duv21.y);
						T = infDet * (d21 * duv23.y - d23 * duv21.y);
						T.normalize();
						_tan->setElement( idx+1, &T );

						infDet = 1.0f / (-duv31.x * duv23.y + duv23.x * duv31.y);
						T = infDet * (d31 * -duv23.y + d23 * duv31.y);
						T.normalize();
						_tan->setElement( idx+2, &T );
					}
					return 1;
				}

				virtual float recalcSphere( float3 & m, float3 & M )
				{
					m = float3(1.0e6,1.0e6,1.0e6);
					M = float3(-1.0e6,-1.0e6,-1.0e6);
					for ( uint i=0; i<_pos->numElements(); i++ )
					{
						float3 e( (float*)_pos->getElement(i) );
						if ( m[0] > e[0] ) m[0] = e[0];
						if ( m[1] > e[1] ) m[1] = e[1];
						if ( m[2] > e[2] ) m[2] = e[2];
						if ( M[0] < e[0] ) M[0] = e[0];
						if ( M[1] < e[1] ) M[1] = e[1];
						if ( M[2] < e[2] ) M[2] = e[2];
					}
					float radius = 0.5 * (M-m).length();
					return radius;
				}
			};			

	//------------- ------------------ ----------------- --------------
	//
	// CMesh
	//
	//------------- ------------------ ----------------- --------------
			struct CMesh 
				: CNode< Core::Mesh, CMesh >
			{
//				AAbb box;

				VertexBuffer *_vb;

				Effect * _eff;
				Shader  * _vsha;
				Shader  * _psha;

				std::vector<Shader::Constant> _vsc;
				std::vector<Shader::Constant> _psc;
				std::vector<Shader::RenderState> _rst;

				Texture * _tex[4];
				uint      _mapping[4];

				CMesh()
					: _vb(0) //box(1)
					, _vsha(0)
					, _psha(0)
					, _eff(0)
				{
					_tex[0] = _tex[1] = _tex[2] = _tex[3] = (0);					
					//_mapping[0] = 0;
					//_mapping[1] = 1;
					//_mapping[2] = 2;
					//_mapping[3] = 3;
					_name.setUnique( typeName() );
					_vb = (VertexBuffer*) CVertexBuffer::createInterface();
//					_eff = (Effect*)CEffect::createInterface();
				}

				virtual ~CMesh()
				{
					E_RELEASE( _eff );
					E_RELEASE( _vsha );
					E_RELEASE( _psha );
					E_RELEASE( _tex[0] );
					E_RELEASE( _tex[1] );
					E_RELEASE( _tex[2] );
					E_RELEASE( _tex[3] );					
					E_RELEASE( _vb );
				}
				virtual const char* typeName() const 
				{ 
					return "Mesh";
				}
				
				virtual uint setBuffer( VertexBuffer * vb ) 
				{
					E_ADDREF(vb);
					E_RELEASE(_vb);
					_vb = vb;
					return (_vb != 0);
				}
				virtual VertexBuffer * getBuffer() const 
				{
					E_ADDREF(_vb);
					return _vb;
				}

				virtual uint setup( uint fmt, uint nFaces ) 
				{
					return _vb->setup( fmt, nFaces );
				}
				virtual VertexFeature * getFeature( uint id ) const
				{
					return _vb->getFeature( id );
				}
				virtual uint format() const  
				{
					return _vb->format();
				}
				virtual uint numFaces() const
				{
					return _vb->numFaces();
				}

				virtual uint recalcNormals(float smooth)
				{
					return _vb->recalcNormals(smooth);
				}

				//virtual const AAbb * getBox()
				//{
				//	return &box;
				//}
				virtual float recalcSphere()
				{
					float3 m,M;
					radius = _vb->recalcSphere( m, M );
					return CNode< Core::Mesh, CMesh >::recalcSphere();
				}

				virtual void scaleBy( float factor )
				{
					//for ( uint i=0; i<_vb->numFaces(); i++ )
					//{
					//	float3 & A   = *(float3 *) _vb->_pos->getElement( i );
					//	A *= factor;
					//	_vb->_pos->setElement( i, &A );
					//}

					//box.m *= factor;
					//box.M *= factor;

					CNode< Core::Mesh, CMesh >::scaleBy( factor );
				}

				virtual uint recalcTangents()
				{
					return _vb->recalcTangents();
				}

				virtual Texture * getTexture( uint stage )
				{
					E_ADDREF( _tex[stage] );
					return _tex[stage];
				}
				virtual void setTexture( uint stage, Texture * t )
				{
					E_ADDREF( t );
					E_RELEASE( _tex[stage] );
					_tex[stage] = t;	
				}

				virtual Effect * getEffect() 
				{
					E_ADDREF( _eff );
					return _eff;
				}
				virtual void setEffect( Effect * e )
				{
					E_ADDREF( e );
					E_RELEASE( _eff );
					_eff = e;	
				}

				//virtual uint getMapping( uint stage )
				//{
				//	return _mapping[stage];
				//}
				//virtual void setMapping( uint stage, uint t )
				//{
				//	_mapping[stage] = t;	
				//}

				virtual Shader * getPixelShader()
				{
					E_ADDREF( _psha );
					return _psha;
				}
				virtual void setPixelShader( Shader * s )
				{
					E_ADDREF( s );
					E_RELEASE( _psha );
					_psha = s;	
				}

				virtual void setPixelShaderConstant( uint index, const float4 & v )
				{
					for ( uint i=0; i<_psc.size(); i++ )
					{
						if ( _psc[i].index == index )
						{
							_psc[i].value = v;
							return;
						}
					}
					Shader::Constant c;
					c.index = index;
					c.value = v;
					_psc.push_back( c );
				}
				virtual Shader::Constant * getPixelShaderConstant( uint n )
				{
					if ( n < _psc.size() )	
					{
						return &(_psc[n]);
					}
					return 0;
				}
				virtual uint getNumPixelShaderConstants()
				{
					return _psc.size() ;
				}

				virtual Shader * getVertexShader()
				{
					E_ADDREF( _vsha );
					return _vsha;
				}
				virtual void setVertexShader( Shader * s )
				{
					E_ADDREF( s );
					E_RELEASE( _vsha );
					_vsha = s;	
				}

				virtual void setVertexShaderConstant( uint index, const float4 & v )
				{
					for ( uint i=0; i<_vsc.size(); i++ )
					{
						if ( _vsc[i].index == index )
						{
							_vsc[i].value = v;
							return;
						}
					}
					Shader::Constant c;
					c.index = index;
					c.value = v;
					_vsc.push_back( c );
				}
				virtual Shader::Constant * getVertexShaderConstant( uint n )
				{
					if ( n < _vsc.size() )	
					{
						return &(_vsc[n]);
					}
					return 0;
				}
				virtual uint getNumVertexShaderConstants()
				{
					return _vsc.size() ;
				}

				//virtual void setUniform( const char *name, const float4 & v ) 
				//{
				//	// NOT_IMPL;
				//}
				//virtual Shader::Constant * getUniform( const char *name ) 
				//{
				//	return 0; // NOT_IMPL;
				//}


				void setRenderState( uint key, uint value )
				{
					for ( uint i=0; i<_rst.size(); i++ )
					{
						if ( _rst[i].index == key )
						{
							_rst[i].value = value;
							return;
						}
					}
					Shader::RenderState r;
					r.index = key;
					r.value = value;
					_rst.push_back( r );
				}
				virtual Shader::RenderState * getRenderState( uint n )
				{
					if ( n < _rst.size() )	
					{
						return &(_rst[n]);
					}
					return 0;
				}
				virtual uint getNumRenderStates()
				{
					return _rst.size() ;
				}
	
				virtual uint render( Renderer * renderer ) 
				{
					$();
					return 1;
				}

				virtual Node * copy()
				{
					CMesh * other = (CMesh*)createInterface(); 
					copyNode( other );
//					other->box = box;
					other->radius = radius;

					other->setBuffer( _vb );

					other->_vsha = _vsha;	E_ADDREF( _vsha );
					other->_psha = _psha;	E_ADDREF( _psha );
					other->_eff  = _eff;	E_ADDREF( _eff );

					for ( uint i=0; i<4; i++ )
					{
						other->_tex[i] = _tex[i];		E_ADDREF( _tex[i] );
						//other->_mapping[i] = _mapping[i];
					}
					return other;
				}											

				virtual void * cast( const char *ifname )
				{
					return _cast( ifname, "Core.Mesh", "Core.Node", "e6.Name" );
				}
			};
			
	//------------- ------------------ ----------------- --------------
	//
	// CCamera
	//
	//------------- ------------------ ----------------- --------------

	struct CCamera 
		: CNode< Core::Camera, CCamera >
	{
		float fov,np,fp;


		CCamera() : fov(60), np(1), fp(300) 
		{
			friction = 0.3f;
			_name.setUnique( typeName() );
		}


		virtual ~CCamera()
		{}
	

		virtual float getFov() const 
		{
			return fov;
		}


		virtual void setFov(float v)
		{
			fov = v;
		}


		virtual float getNearPlane() const
		{
			return np;
		}

		virtual void setNearPlane( float  n )
		{
			np=n;
		}


		virtual float getFarPlane() const
		{
			return fp;
		}

		virtual void setFarPlane( float  f )
		{
			fp=f;
		}


		virtual const char* typeName() const 
		{ 
			return "Camera";
		}


		virtual uint render( Renderer * renderer ) 
		{
			$();
			return 1;
		}
		virtual Node * copy()
		{
			CCamera * other = (CCamera*)createInterface(); 
			copyNode( other );
			other->fov = fov;
			other->np = np;
			other->fp = fp;
			return other;
		}											
		virtual void * cast( const char *ifname )
		{
			return _cast( ifname, "Core.Camera", "Core.Node", "e6.Name" );
		}
	};
	

	//------------- ------------------ ----------------- --------------
	//
	// CLight
	//
	//------------- ------------------ ----------------- --------------

	struct CLight
		: CNode< Core::Light, CLight >
	{
		float3 dir;
		float3 col;
		uint lt;
		uint dm;


		CLight()
			: lt(0)
			, dm(0)
			, dir( 0,-1,0 )
		{
			_name.setUnique( typeName() );
		}

		virtual ~CLight() {}

		virtual void setRadius( float r ) 
		{
			radius = r;
		}


		virtual const float3 & getColor() const 
		{
			return col;
		}


		virtual void setColor( const float3 & c ) 
		{
			col = c;
		}


		virtual uint getType() const 
		{
			return lt;
		}


		virtual void setType( uint l ) 
		{
			lt = l;
		}


		virtual uint getDecay() const 
		{
			return dm;
		}


		virtual void setDecay( uint d ) 
		{
			dm = d;
		}


		virtual const float3 & getDir() const 
		{
			return dir;
		}


		virtual void setDir( const float3 & d )
		{
			dir = d;
		}


		virtual const char* typeName() const 
		{ 
			return "Light";
		}

		virtual uint render( Renderer * renderer ) 
		{
			$();
			return 1;
		}
		virtual Node * copy()
		{
			CLight * other = (CLight*)createInterface(); 
			copyNode( other );
			other->dir = dir;
			other->col = col;
			other->lt = lt;
			other->dm = dm;
			return other;
		}											
		virtual void * cast( const char *ifname )
		{
			return _cast( ifname, "Core.Light", "Core.Node", "e6.Name" );
		}
	};
			

	//------------- ------------------ ----------------- --------------
	//
	// CFreeNode
	//
	//------------- ------------------ ----------------- --------------

	struct CFreeNode
		: CNode< Core::FreeNode, CFreeNode >
	{
		FreeNodeUser * _user;
		

		CFreeNode() 
			: _user(0) 
		{
			_name.setUnique( typeName() );
		}


		virtual ~CFreeNode() 
		{
			E_RELEASE(_user);
		}

				
		virtual void setUser( FreeNodeUser * u ) 
		{
			_user = u;
		}


		virtual FreeNodeUser * getUser() const
		{
			E_ADDREF(_user);
			return _user;
		}


		virtual const char* typeName() const 
		{ 
			return "FreeNode";
		}
		virtual uint render( Renderer * renderer ) 
		{
			$();
			if ( _user )
			{
				_user->renderNode( this, renderer );
				return 1;
			}
			return 0;
		}
		virtual Node * copy()
		{
			if ( ! prnt )
			{
				e6::sys::alert( "Don't do This !", "You are trying to clone the root node." );
				return 0;
			}
			CFreeNode * other = (CFreeNode*)createInterface(); 
			copyNode( other );
			other-> _user =  _user; E_ADDREF( _user );
			return other;
		}											
		virtual void * cast( const char *ifname )
		{
			return _cast( ifname, "Core.FreeNode", "Core.Node", "e6.Name" );
		}
	};


	//------------- ------------------ ----------------- --------------
	//
	// CRenderToTexture
	//
	//------------- ------------------ ----------------- --------------

	struct CRenderToTexture
		: CNode< RenderToTexture, CRenderToTexture >
	{
		Texture * _tex;

		CRenderToTexture()
			: _tex(0)
		{
			_name.setUnique("RenderToTexture");
		}

		~CRenderToTexture() 
		{
			E_RELEASE( _tex );
		}

		virtual void * cast( const char *ifname )
		{
			return _cast( ifname, "Core.RenderToTexture", "Core.Node", "e6.Name" );
		}
		virtual const char* typeName() const 
		{ 
			return "RenderToTexture";
		}
		virtual uint render( Renderer * renderer ) 
		{
			return 0; //NOT_IMPL
		}
		virtual Node * copy()
		{
			if ( ! prnt )
			{
				e6::sys::alert( "Don't do This !", "You are trying to clone the root node." );
				return 0;
			}
			CRenderToTexture * other = (CRenderToTexture*)createInterface(); 
			copyNode( other );
			other-> _tex =  _tex; E_ADDREF( _tex );
			return other;
		}											
		virtual uint setRenderTarget( Texture * tex ) 
		{
			E_ADDREF( tex );
			E_RELEASE( _tex );
			_tex = tex;
			return 1;
		}
		virtual Texture * getRenderTarget() const 
		{
			E_ADDREF( _tex );
			return _tex;
		}
	};



	//------------- ------------------ ----------------- --------------
	//
	// CWorld
	//
	//------------- ------------------ ----------------- --------------

	struct CWorld
		: Class<World,CWorld>
	{

		Node * _root;

		e6::CMapContainer< Texture > _tex;
		e6::CMapContainer< Shader >  _sha;
		e6::CMapContainer< Effect >  _eff;
		
		CWorld()
		{
			_root = new CFreeNode;
			_root->setName( "Gaja" );
		}


		virtual ~CWorld()
		{
			E_RELEASE( _root );
		}

		virtual Node * findRecursive( const char *name ) 
		{
			return _root->findRecursive( name );
		}

		virtual uint link( Node * node, const char * parent ) 
		{
			Node * p = _root->findRecursive( parent );
			if ( ! p  )
			{
				p = this->getRoot();
			}

			uint r = p->link( node );

			E_RELEASE(p);
			return r;
		}

		virtual e6::Container<Texture>  & textures()   
		{
			return _tex;
		}


		virtual e6::Container< Shader >  & shaders()  
		{
			return _sha;
		}

		virtual e6::Container< Effect >  & effects()  
		{
			return _eff;
		}

		virtual void scaleTo( Node * start, float radius )
		{
			float factor = radius / start->getSphere();
			start->scaleBy( factor );
		}

		virtual uint save( e6::Engine * e, const char * orig, Node * start_node=0 ) 
		{
			char path[200];
			char name[200];
			char ext[20];

			if ( ! e->chopPath( orig, path, name ) )
			{
				e6::sys::alert( __FUNCTION__, "path(%s) contains no name!", orig );
				return 0;
			}

			if ( ! e->chopExt( orig, path, ext ) )
			{
				e6::sys::alert( __FUNCTION__, "path(%s) contains no extension!", orig );
				return 0;
			}
			// find an exporter:
			char expName[30] = "Export";
			strcat( expName, ext );

			Exporter * exp = (Exporter*)e->createInterface( expName, "Core.Exporter" );
			if ( ! exp )
			{
				e6::sys::alert( __FUNCTION__,  "Exporter(%s) not found for extension(%s)", expName, ext );
				return 0;
			}
			Core::Node *start = start_node ? start_node : _root;
			//E_ADDREF( start );
			//E_ADDREF( this );
			//E_ADDREF( e );
			uint r = exp->save( e, this, orig, start );
			//E_RELEASE( start );
			//E_RELEASE( this );
			//E_RELEASE( e );
			E_RELEASE( exp );
			return r;
		}

		virtual uint save( e6::Engine * e, const char * orig, Texture * tex ) 
		{
			char path[200];
			char name[200];
			char ext[20];

			if ( ! e->chopPath( orig, path, name ) )
			{
				e6::sys::alert( __FUNCTION__, "path(%s) contains no name!", orig );
				return 0;
			}

			if ( ! e->chopExt( orig, path, ext ) )
			{
				e6::sys::alert( __FUNCTION__, "path(%s) contains no extension!", orig );
				return 0;
			}
			// find an exporter:
			char expName[30] = "Export";
			strcat( expName, ext );

			Exporter * exp = (Exporter*)e->createInterface( expName, "Core.Exporter" );
			if ( ! exp )
			{
				e6::sys::alert( __FUNCTION__,  "Exporter(%s) not found for extension(%s)", expName, ext );
				return 0;
			}

			E_ADDREF( tex );
			//E_ADDREF( this );
			//E_ADDREF( e );
			uint r = exp->save( e, this, orig, tex );
			E_RELEASE( exp );
			//E_RELEASE( this );
			//E_RELEASE( e );
			E_RELEASE( tex );
			return r;
		}



		virtual uint load( e6::Engine * e, const char * orig, Node * start ) 
		{
			char path[200];
			char name[200];
			char ext[20];

			if ( ! e->chopExt( orig, path, ext ) )
			{
				e6::sys::alert( __FUNCTION__, "path(%s) contains no extension!", orig );
				return 0;
			}

			if ( ! e->chopPath( orig, path, name ) )
			{
				e6::sys::alert( __FUNCTION__, "path(%s) contains no name!", orig );
				return 0;
			}

			char sep = e6::sys::fileSeparator();
			sprintf( path, "%s%cres%c%s%c%s", e->getPath(), sep, sep, ext, sep, name );

			// check if it's already loaded:
			//~ _tex.dump("tex");
			//~ _sha.dump("sha");
			Texture * tex = _tex.get(name);
			if ( tex )
			{
				printf( "found in texture cache : %s\n", name );
				E_RELEASE( tex );
				return 1;
			}
			Shader * sh = _sha.get(name);
			if ( sh )
			{
				printf( "found in shader cache : %s\n", name );
				E_RELEASE( sh );
				return 1;
			}

			// find an importer:
			char impName[30] = "Import";
			strcat( impName, ext );

			Importer * imp = (Importer*)e->createInterface( impName, "Core.Importer" );
			if ( ! imp )
			{
				e6::sys::alert( __FUNCTION__,  "importer(%s) not found for extension(%s)", impName, ext );
				return 0;
			}


//			e6::sys::setCurrentDir( path );

//			E_ADDREF( e );
//			E_ADDREF( this );
			uint r = imp->load( e, this, path );
			if ( ! r )
			{
				e6::sys::alert( __FUNCTION__, " %s not found \n on path(!%s)", path, e6::sys::getCurrentDir() );
			}
			E_RELEASE( imp );
//			E_RELEASE( e );
//			E_RELEASE( this );

			//e6::sys::setCurrentDir( e->getPath() );
			return r;
		}

		//virtual RenderToTexture * createRenderToTexture()
		//{
		//	RenderToTexture * rt = (RenderToTexture *)engine->createInterface( "Core", "Core.RenderToTexture" );
		//	if ( ! rt ) 
		//	{
		//		return 0;
		//	}
		//	Texture * t = (Texture *) engine->createInterface( "Core", "Core.Texture" );
		//	if ( ! t )
		//	{
		//		E_RELEASE(rt);
		//		return 0;
		//	}
		//	t->setRenderTarget( e6::PF_X8B8G8R8, 512, 512, 1 );
		//	rt->setRenderTarget(t);
		//	textures().add( t->getName(), t );
		//	E_RELEASE(t);
		//	return rt;
		//}

		virtual void clear()
		{
			_root->clearChildren();
			_tex.clear();
			_sha.clear();
		}

		virtual Node * getRoot() 
		{
			E_ADDREF(_root);
			return _root;
		}
	};			


	struct CShader
		: Core::CResource< Shader, CShader >
	{
		e6::CString code;
		
		virtual void setCode( const char * str ) 
		{
			code.set( str );
		}
		virtual const char * getCode() const 
		{
			return code.get();
		}
		
	}; //CShader

	
};

#include "CoreString.h"

extern ClassInfo ClassInfo_MeshString; 
extern ClassInfo ClassInfo_MeshVSString; 
extern ClassInfo ClassInfo_MeshPSString; 
extern ClassInfo ClassInfo_MeshRSString; 
extern ClassInfo ClassInfo_CameraString; 
extern ClassInfo ClassInfo_LightString; 
extern ClassInfo ClassInfo_NodeString; 
extern ClassInfo ClassInfo_RenderString; 

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.World",		"Core",			Core::CWorld::createSingleton,	Core::CWorld::classRef		},
		{	"Core.Texture",		"Core",			Core::CTexture::createInterface, Core::CTexture::classRef	},
		{	"Core.FreeNode",	"Core",			Core::CFreeNode::createInterface,Core::CFreeNode::classRef	},
		{	"Core.Mesh",		"Core",			Core::CMesh::createInterface,	Core::CMesh::classRef		},
		{	"Core.Light",		"Core",			Core::CLight::createInterface,	Core::CLight::classRef		},
		{	"Core.Camera",		"Core",			Core::CCamera::createInterface,	Core::CCamera::classRef		},
		{	"Core.Shader",		"Core",			Core::CShader::createInterface,	Core::CShader::classRef		},
		{	"Core.VertexBuffer","Core",			Core::CVertexBuffer::createInterface,	Core::CVertexBuffer::classRef		},
		{	"Core.RenderToTexture","Core",		Core::CRenderToTexture::createInterface,	Core::CRenderToTexture::classRef		},

		ClassInfo_MeshString,
		ClassInfo_MeshVSString,
		ClassInfo_MeshPSString,
		ClassInfo_MeshRSString,
		ClassInfo_CameraString,
		ClassInfo_LightString,
		ClassInfo_NodeString,
		ClassInfo_RenderString,
		
		{	0, 0, 0	}
	};

	*ptr = (ClassInfo *)_ci;

	return 17; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Core 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

