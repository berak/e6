#include <cstdio>
#include <cstring>
#include <cmath>
#include <map>
#include <vector>
#include "TimeLine.h"
#include "Curve.h"
#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"

namespace TimeLine
{
	using e6::Class;
	using e6::CName;
	using e6::CString;

	// fwd:
	struct CFilter;

	struct Pin
	{
		CFilter	  * cfilter;
		uint 		pinin;
		uint 		pinout;
		~Pin() {}
		Pin() : cfilter(0), pinin(666), pinout(666) {printf("THIS WILL NEVER HAPPEN!  %s\n", __FUNCTION__);}
		Pin( CFilter * f, uint i, uint o ) : cfilter(f), pinin(i), pinout(o) {}
	};

	// ----- ----- ----- ----- ----- ----- ----- ----- ----- 
	//
	// FilterBase
	//
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- 

	struct CFilter
	{
		typedef std::map< const char*, Pin*, e6::StrLess > PinMap;  
		typedef PinMap::iterator PinIter;  
		typedef PinMap::const_iterator PinIterC;  

		PinMap		con;
		Filter	*	filter;
		int			distance;
		float		timeOffset;
		float		timeScale;
		
		CFilter(Filter * f)
			: filter(f)
			, distance(0)
			, timeOffset(0)
			, timeScale(1.0f)
		{
			//$X1("CFilter()");
		}

		virtual ~CFilter()
		{
			for ( PinIter pi = con.begin(); pi != con.end(); pi++ )
			{
				E_DELETE( pi->second );
			}
			//$X1("~CFilter()");
		}

		bool update(float t)
		{
			bool _debug = 0;
			const char * fn = filter->getName();
			if ( _debug )
			{
				printf(" %s.update(%f)\n", fn, t );
			}
			for ( PinIter pi = con.begin(); pi != con.end(); pi++ )
			{
				Pin * p = pi->second;
				float v = p->cfilter->filter->getOutput( p->pinout );
				filter->setInput( p->pinin, v );
				if ( _debug )
				{
					printf( " %s.%s %3.3f\n", fn, filter->getInputName( p->pinin ) , v );
				}
			}

			float localTime = t * timeScale + timeOffset;
			uint r = filter->update(localTime);

			if ( _debug )
			{
				for ( uint o=0; o<filter->numOutputs(); o++ )
				{
					float v = filter->getOutput(o);
					printf( " %s.%s %3.3f\n", fn, filter->getOutputName(o) , v );
				}
			}
			return r;
		}

		bool disconnect(const char * inpin)
		{
			PinIter pi = con.find(inpin);
			if ( pi != con.end() )
			{
				E_DELETE( pi->second );
				con.erase(pi);
				return 1;
			}
			e6::sys::alert( __FUNCTION__, "could not disconnect Pin (%s) !", inpin );
			return 0;
		}
		bool connect( const char * inname, CFilter * cf, const char * outname )
		{
			uint ni = filter->numInputs();
			uint no = cf->filter->numOutputs();
			for ( uint i=0; i<ni; i++ )
			{
				const char * fin = filter->getInputName(i);
				if ( ! strcmp( inname, fin ) )
				{
					for ( uint o=0; o<no; o++ )
					{
						const char * fon = cf->filter->getOutputName(o);
						if ( ! strcmp( outname, fon ) )
						{
							con [ fin ] = new Pin(cf,i,o);
							return 1;
						}
					}
					e6::sys::alert( __FUNCTION__, "no output(%s) in filter (%s)(%i) !", outname, cf->filter->getName(), no );
					return 0;
				}
			}
			e6::sys::alert( __FUNCTION__, "could not connect %s(%i).%s to %s(%i).%s !", filter->getName(),ni, inname, cf->filter->getName(), no, outname );
			return 0;
		}

		bool getConnection( const char * inpin, char * outFilterName, char * outpin )
		{
			PinIter pi = con.find(inpin);
			if ( pi != con.end() )
			{
				Pin * p = pi->second;
				strcpy( outFilterName, p->cfilter->filter->getName() );
				strcpy( outpin, p->cfilter->filter->getOutputName( p->pinout ) );
				return 1;
			}
			//e6::sys::alert( __FUNCTION__, "could not find inpin (%s) !", inpin );
			//  don't bark, there IS no connection.
			return 0;
		}
	};

			
		





	// ----- ----- ----- ----- ----- ----- ----- ----- ----- 
	//
	// TimeLine
	//
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- 
	struct CTimeLine 
		: e6::Class< TimeLine, CTimeLine >
	{
				
		static int cmp_dist( const void *a, const void *b )
		{
			CFilter * ta = *(CFilter**)a;
			CFilter * tb = *(CFilter**)b;
			//printf( "dist : %i %i\n",ta->distance, tb->distance );
			return ta->distance - tb->distance;
		}

		std::vector<CFilter*> elems;
		typedef std::vector<CFilter*>::iterator f_iter;
		typedef std::vector<CFilter*>::const_iterator f_iter_c;

		bool _needs_sort;


		CTimeLine()
			: _needs_sort(0)
		{}
				
		~CTimeLine() 
		{
			clear();
			//for ( f_iter it = elems.begin(); it != elems.end(); ++it )
			//{
			//	Filter *tm = (*it)->filter;
			//	E_RELEASE ( tm );
			//	CFilter *cf = *it;
			//	E_DELETE ( cf );
			//}
		}

		virtual bool clear()
		{
			if ( elems.empty() ) return 0;
			for ( f_iter it = elems.begin(); it != elems.end(); ++it )
			{
				Filter *tm = (*it)->filter;
				E_RELEASE ( tm );
				CFilter *cf = *it;
				E_DELETE ( cf );
			}
			elems.clear();
			return 1;
		}

		f_iter_c _find_c( const char * name ) const
		{
			f_iter_c it = elems.begin();
			while ( it != elems.end() )
			{
				if ( ! strcmp( (*it)->filter->getName(), name ) )
				{
					break;
				}
				++it;
			}
			return it;
		}
		f_iter _find( const char * name )
		{
			f_iter it = elems.begin();
			while ( it != elems.end() )
			{
				if ( ! strcmp( (*it)->filter->getName(), name ) )
				{
					break;
				}
				++it;
			}
			return it;
		}

		virtual Filter * get( const char * name ) 
		{
			f_iter it = _find(name);
			if ( it == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not get Filter (%s) !", name );
				return 0;
			}
			Filter * t = (*it)->filter;
			E_ADDREF ( t )
			return t;
		}

		virtual bool add( Filter *t ) 
		{
			f_iter it = _find(t->getName());
			if ( it != elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "Filter (%s) already exists !", t->getName() );
				return 0;
			}
		
			elems.push_back( new CFilter( t ) );
			E_ADDREF( t );

			_needs_sort = true;
			return 1;
		}

		virtual bool remove( const char * name) 
		{
			f_iter it = _find(name);
			if ( it == elems.end() )
			{					
				e6::sys::alert( __FUNCTION__, "could not find Filter (%s) !", name );
				return 0;
			}
			Filter * t = (*it)->filter;
			E_RELEASE ( t );
			E_DELETE ( *it );
			elems.erase(it);

			_needs_sort = true;
			return 1;
		}

		virtual bool update( float t ) 
		{	
			if ( elems.empty() )
				return 1;

			if ( _needs_sort )
			{
				resort();
			}
			//for ( f_iter it = elems.begin(); it != elems.end(); ++it )
			for ( uint i=0; i<elems.size(); i++ )
			{
				//(*it)->update( t );
				elems[i]->update( t );
			}
			return 1;
		}

		virtual bool connect ( const char * inFilterName, const char * inpin, const char * outFilterName, const char * outpin )
		{
			f_iter in = _find(inFilterName);
			if ( in == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not find input Filter (%s) !", inFilterName );
				return 0;
			}
			f_iter out = _find(outFilterName);
			if ( out == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not find output Filter (%s) !", outFilterName );
				return 0;
			}
			CFilter * ci = (*in); 
			CFilter * co = (*out); 
			_needs_sort = true;
			return ci->connect( inpin, co, outpin );
		}

		virtual bool disconnect( const char * filterName, const char * inpin )
		{
			f_iter it = _find(filterName);
			if ( it == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not find connected Filter (%s) !", filterName );
				return 0;
			}
			CFilter * in = (*it);
			_needs_sort = true;
			return in->disconnect( inpin );
		}

		virtual bool getConnection( const char * filterName, const char * inpin, char * outFilterName, char * outpin )
		{
			f_iter it = _find(filterName);
			if ( it == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not find connected Filter (%s) !", filterName );
				return 0;
			}
			CFilter * in = (*it);
			return in->getConnection( inpin, outFilterName, outpin );
		}

		virtual void print()
		{
			if ( _needs_sort )
			{
				resort();
			}
			printf("<timeline>\n");
			for ( f_iter_c it = elems.begin(); it != elems.end(); ++it )
			{
				Filter *tm = (*it)->filter;
				const char * n = tm->getName();
				printf(" <%s in='%i' out='%i'>\n",n, tm->numInputs(), tm->numOutputs());

				/**
				for( uint i=0; i<tm->numInputs(); i++ )
				{
					printf("  <ipin name'%s'/>\n",tm->getInputName(i) );
				}
				for( uint o=0; o<tm->numOutputs(); o++ )
				{
					printf("  <opin name'%s' value='%3.3f'/>\n",tm->getOutputName(o), tm->getOutput(o) );
				}
				**/

				CFilter::PinIterC pit = (*it)->con.begin();
				for ( ; pit !=  (*it)->con.end(); ++pit )
				{
					const char * pname = pit->first;
					const Pin * p = pit->second;
					CFilter * cf = p->cfilter;					
					Filter * fil = cf->filter;					
					const char * pName = fil->getOutputName( p->pinout );
					printf("  <connection name='%s' to='%s.%s' />\n",pname,fil->getName(),pName );
				}
				printf("  <distance value='%i'/>\n",(*it)->distance );
				printf(" </%s>\n",n);
			}
			printf("</timeline>\n");
		}

		// sort filters by distance:
		bool resort()
		{
			// recalcDistance
			for ( f_iter it = elems.begin(); it != elems.end(); ++it )
			{
				(*it)->distance = calcDistance( *(it) );
			}
			// sortFilters()
			qsort( &(elems[0]), elems.size(), sizeof(CFilter*), cmp_dist );

			_needs_sort = false;
			return 1;
		}			

		//! count upstream filters
		int calcDistance( CFilter * cf )
		{
			int distance = 0;
		
			CFilter::PinIter pit = cf->con.begin();
			for ( ; pit !=  cf->con.end(); ++pit )
			{
				int d = 1 + calcDistance((pit->second->cfilter));
				distance = (distance>d ? distance : d);
			}

			return distance;
		}

		virtual void  setTimeOffset( const char * filterName, float to ) 
		{
			f_iter it = _find(filterName);
			if ( it == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not find Filter (%s) !", filterName );
				return;
			}
			CFilter * fil = (*it);
			fil->timeOffset = to;
		}
		virtual void  setTimeScale ( const char * filterName, float ts ) 
		{
			f_iter it = _find(filterName);
			if ( it == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not find Filter (%s) !", filterName );
				return;
			}
			CFilter * fil = (*it);
			fil->timeScale = ts;
		}
		virtual float getTimeOffset( const char * filterName ) const 
		{
			f_iter_c it = _find_c(filterName);
			if ( it == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not find Filter (%s) !", filterName );
				return 0;
			}
			CFilter * fil = (*it);
			return fil->timeOffset;
		}
		virtual float getTimeScale ( const char * filterName ) const 
		{
			f_iter_c it = _find_c(filterName);
			if ( it == elems.end() )
			{
				e6::sys::alert( __FUNCTION__, "could not find Filter (%s) !", filterName );
				return 0;
			}
			CFilter * fil = (*it);
			return fil->timeScale;
		}
		
		virtual uint traverse( Visitor * filterWalker ) const
		{
			E_ASSERT(filterWalker); 
			$();
			for ( f_iter_c it = elems.begin(); it != elems.end();	++it )
			{
				const Filter * filter = (*it)->filter;
				//const Filter * filter = (const Filter*)(*it); // cast from CFilter
				if ( ! filter )
					return 0;
				if ( ! filterWalker->visitFilter( filter ) )
					return 0;
			}
			return 1;
		}
	}; // TimeLine

		

		
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- 
	//
	// IpoBase
	//
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- 
	template < class Super, class Impl, class T > 
	struct CIpoBase
		: e6::CName< Super,Impl >
	{
		T 			 r;
		Curve< T >   curve;
		uint         _ipolType, _expolType;
		bool		 _needs_sort;

		CIpoBase()  
			: _ipolType(0)
			, _expolType(0)
			, _needs_sort(0)
		{
			unique("Ipo");
		}

		virtual ~CIpoBase()  
		{
		}

		virtual uint numInputs() const
		{
			return 0;
		}
		virtual uint setInput( uint i, float v ) 
		{
			return 0;
		}
		virtual const char * getInputName( uint i ) const 
		{
			return 0;
		}

		virtual uint update( float t ) 
		{
			if ( _needs_sort )
			{
				sortPoints();
			}
			switch( _ipolType )
			{
				default:
				case e6::IM_SPLINE:			r = curve.spline(t);	break;
				case e6::IM_CONSTANT:		r = curve.constant(t);	break;
				case e6::IM_LINEAR:			r = curve.linear(t);	break;
			}
			return 1;
		}

		uint numPoints() const 
		{
			return curve.numPoints();
		}
		uint remPoint(float t) 
		{
			curve.delPoint(t,0.01f);
			_needs_sort = true;
			return curve.numPoints();
		}

		virtual void sortPoints()
		{
			curve.sortPoints();
			_needs_sort = false;
		}

		virtual uint getInterpolationType() const 
		{
			return _ipolType;
		}
		virtual void setInterpolationType( uint ipolType ) 
		{
			_ipolType = ipolType;
		}

		virtual uint getExtrapolationType() const
		{
			return _expolType;
		}
		virtual void setExtrapolationType( uint expolType )
		{
			_expolType = expolType;
		}
		virtual float getTime( uint i ) const 
		{
			if ( i>=curve.numPoints() ) return 0;
			return curve.pts[i].t;
		}
	};

	struct CIpo
		: CIpoBase< Ipo, CIpo, float >
	{
		virtual uint numOutputs() const 
		{
			return 1;
		}

		virtual float getOutput( uint i ) const 
		{
			return r;
		}

		virtual const char * getOutputName( uint i ) const 
		{
			return "val\0";
		}

		virtual uint addPoint(float t, float p0) 
		{
			curve.addPoint(t,p0);
			_needs_sort = true;
			return curve.numPoints();
		}

		virtual float getPoint( uint i ) const 
		{
			if ( i>=curve.numPoints() ) return 0;
			return curve.pts[i].v;
		}
			
		virtual void printPoints()
		{
			if ( _needs_sort )
			{
				sortPoints();
			}
			for ( uint i=0; i<curve.pts.size(); i++ )
			{
				printf( "%3.3f\t: ", curve.pts[i].t );
				printf( "%3.3f\t", curve.pts[i].v );
				printf( "\n");
			}
		}
		virtual void *cast( const char * str )
		{
			return _cast(str,"TimeLine.Ipo","TimeLine.Filter", "e6.Name");
		}
	};

	struct CIpo3
		: CIpoBase< Ipo3, CIpo3, e6::float3 >
	{

		virtual uint numOutputs() const 
		{
			return 3;
		}

		virtual float getOutput( uint i ) const 
		{
			if ( i>2 ) return 0;
			return r[i];
		}

		virtual const char * getOutputName( uint i ) const 
		{
			static char * _s[3] = {"x","y","z"};
			if ( i>2 ) return 0;
			return _s[i];
		}

		virtual uint addPoint(float t, e6::float3 p0) 
		{
			curve.addPoint(t,p0);
			_needs_sort = true;
			return curve.numPoints();
		}
		virtual uint addPoint( float t, float p0, float p1, float p2 ) 
		{
			return addPoint(t,e6::float3(p0,p1,p2));			
		}
		virtual float3 getPoint( uint i ) const 
		{
			if ( i>=curve.numPoints() ) return 0;
			return curve.pts[i].v;
		}
		virtual void printPoints()
		{
			if ( _needs_sort )
			{
				sortPoints();
			}
			for ( uint i=0; i<curve.pts.size(); i++ )
			{
				printf( "%3.3f\t: ", curve.pts[i].t );
				printf( "%3.3f\t", curve.pts[i].v[0] );
				printf( "%3.3f\t", curve.pts[i].v[1] );
				printf( "%3.3f\t", curve.pts[i].v[2] );
				printf( "\n");
			}
		}
		virtual void *cast( const char * str )
		{
			return _cast(str,"TimeLine.Ipo3","TimeLine.Filter", "e6.Name");
		}
	};

	struct CIpo4
		: CIpoBase< Ipo4, CIpo4, e6::float4 >
	{
		virtual uint numOutputs() const 
		{
			return 4;
		}

		virtual float getOutput( uint i ) const 
		{
			if ( i>3 ) return 0;
			return r[i];
		}

		virtual const char * getOutputName( uint i ) const 
		{
			static char * _s[4] = {"x","y","z","w"};
			if ( i>3 ) return 0;
			return _s[i];
		}

		virtual uint addPoint(float t, e6::float4 p0) 
		{
			curve.addPoint(t,p0);
			_needs_sort = true;
			return curve.numPoints();
		}
		virtual uint addPoint( float t, float p0, float p1, float p2, float p3 ) 
		{
			return addPoint( t, e6::float4(p0,p1,p2,p3) );
		}
		virtual float4 getPoint( uint i ) const 
		{
			if ( i>=curve.numPoints() ) return 0;
			return curve.pts[i].v;
		}
		virtual void printPoints()
		{
			if ( _needs_sort )
			{
				sortPoints();
			}
			for ( uint i=0; i<curve.pts.size(); i++ )
			{
				printf( "%3.3f\t: ", curve.pts[i].t );
				printf( "%3.3f\t", curve.pts[i].v[0] );
				printf( "%3.3f\t", curve.pts[i].v[1] );
				printf( "%3.3f\t", curve.pts[i].v[2] );
				printf( "%3.3f\t", curve.pts[i].v[3] );
				printf( "\n");
			}
		}
		virtual void *cast( const char * str )
		{
			return _cast(str,"TimeLine.Ipo4","TimeLine.Filter", "e6.Name");
		}
	};

	// ----- ----- ----- ----- ----- ----- ----- ----- ----- 
	//
	// Demo
	//
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- 


	struct CSinus 
		: e6::CName< Sinus , CSinus >
	{
		float o;
		float vs, vc;

		CSinus() 
			: o(0)
			, vs(0)
			, vc(0)
		{
			unique("SinCos");
		}

		virtual ~CSinus() 
		{}

		virtual uint numInputs() const
		{
			return 1;
		}
		virtual uint setInput( uint i, float v ) 
		{
			o = v;
			return 1;
		}
		virtual const char * getInputName( uint i ) const 
		{
			return "omega";
		}
		virtual uint numOutputs() const 
		{
			return 2;
		}
		virtual uint update( float t )
		{
			double ot = o * t;
			vs = sin ( ot );
			vc = cos ( ot );
			return 1;
		}
		virtual float getOutput( uint i ) const 
		{
			switch( i )
			{
				case 0:	return vs;
				case 1:	return vc;
			}
			return 0;
		}
		virtual const char * getOutputName( uint i ) const 
		{
			static const char *ss[] = { "sin", "cos" };
			if ( i<2 )
			{
				return ss[i];
			}
			return 0;
		}

		virtual void *cast( const char * str )
		{
			return _cast(str, "TimeLine.Sinus", "TimeLine.Filter", "e6.Name");
		}
	};
			
	struct CScaler
		: e6::CName< Scaler , CScaler >
	{
		float oval;
		float ival, ioff, iscale;

		CScaler() 
			: oval(0)
			, ival(0)
			, ioff(0)
			, iscale(1.0)
		{
			unique("Scaler");
		}

		virtual ~CScaler() 
		{}
		virtual uint numInputs() const
		{
			return 3;
		}
		virtual uint setInput( uint i, float v ) 
		{
			switch ( i )
			{
				case 0 : ival = v;	 	return 1;
				case 1 : ioff = v;		return 1;
				case 2 : iscale = v; 	return 1;
			}
			return 0;
		}
		virtual const char * getInputName( uint i ) const 
		{
			static const char *ss[] = { "input", "offset", "scale" };
			if ( i<3 )
			{
				return ss[i];
			}
			return 0;
		}
		virtual uint numOutputs() const 
		{
			return 1;
		}
		virtual uint update( float t )
		{
			oval = ioff + ival * iscale;
			return 1;
		}
		virtual float getOutput( uint i ) const 
		{
			switch( i )
			{
				case 0:	return oval;
			}
			return 0;
		}
		virtual const char * getOutputName( uint i ) const 
		{
			static const char *ss[] = { "result" };
			if ( i<1 )
			{
				return ss[i];
			}
			return 0;
		}
		virtual void *cast( const char * str )
		{
			return _cast(str, "TimeLine.Scaler", "TimeLine.Filter", "e6.Name");
		}
	};


	struct CValue
		: e6::CName< Value , CValue >
	{
		struct _Pin 
		{ 
			float v;
			char  n[32]; 
			_Pin( const char * n )
				: v(0)
			{
				strcpy(this->n,n);
			}
		};
		std::vector< _Pin* > vin;
		std::vector< _Pin* > vout;

		Value::Update * vup;

		CValue() 
			: vup(0)
		{
			unique("Value");
		}

		virtual ~CValue() 
		{
			for ( uint i=0; i<vin.size();  i++ ) E_DELETE(vin[i]);
			for ( uint i=0; i<vout.size(); i++ ) E_DELETE(vout[i]);
			//E_RELEASE( vup );
		}
		virtual uint addInput(const char * name)
		{
			vin.push_back( new _Pin(name) );
			return 1;
		}

		virtual uint addOutput(const char * name)
		{
			vout.push_back( new _Pin(name) );
			return 1;
		}

		virtual float getInput( uint i ) const 
		{
			if ( i<vin.size() )
			{
				return vin[i]->v;
			}
			return 0;
		}
		// Filter:
		virtual uint setOutput( uint i, float v ) 
		{
			if ( i<vout.size() )
			{
				vout[i]->v = v;
			 	return 1;
			}
			return 0;
		}
		virtual uint numInputs() const
		{
			return vin.size();
		}
		virtual uint setInput( uint i, float v ) 
		{
			if ( i<vin.size()  )
			{
				vin[i]->v = v;
			 	return 1;
			}
			return 0;
		}
		virtual const char * getInputName( uint i ) const 
		{
			if ( i<vin.size() )
			{
				return vin[i]->n;
			}
			return 0;
		}
		virtual uint numOutputs() const 
		{
			return vout.size();
		}
		virtual float getOutput( uint i ) const 
		{
			if ( i<vout.size() )
			{
				return vout[i]->v;
			}
			return 0;
		}
		virtual const char * getOutputName( uint i ) const 
		{
			if ( i<vout.size() )
			{
				return vout[i]->n;
			}
			return 0;
		}

		virtual uint update( float t )
		{
			if ( vup )
			{
				return vup->update( this, t );
			}
			return 1;
		}
		virtual uint setUpdate( Value::Update * v ) 
		{
			//E_ADDREF( v );
			//E_RELEASE( vup );
			vup = v;
			return 1;
		}
		virtual Value::Update * getUpdate() const 
		{
			//E_ADDREF( vup );
			return vup;
		}
		
		virtual void *cast( const char * str )
		{
			return _cast(str,"TimeLine.Value","TimeLine.Filter", "e6.Name");
		}

	}; //Value


} //TimeLine			


using e6::uint;
using e6::ClassInfo;

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"TimeLine.TimeLine",	 "TimeLine",	TimeLine::CTimeLine::createSingleton,	TimeLine::CTimeLine::classRef	},
		{	"TimeLine.Ipo",			 "TimeLine",	TimeLine::CIpo::createInterface,			TimeLine::CIpo::classRef		},
		{	"TimeLine.Ipo3",		 "TimeLine",	TimeLine::CIpo3::createInterface,		TimeLine::CIpo3::classRef		},
		{	"TimeLine.Ipo4",		 "TimeLine",	TimeLine::CIpo4::createInterface,		TimeLine::CIpo4::classRef		},
		{	"TimeLine.Scaler",	 	 "TimeLine",	TimeLine::CScaler::createInterface,		TimeLine::CScaler::classRef		},
		{	"TimeLine.Sinus",	 	 "TimeLine",	TimeLine::CSinus::createInterface,		TimeLine::CSinus::classRef		},
		{	"TimeLine.Value",	 	 "TimeLine",	TimeLine::CValue::createInterface,		TimeLine::CValue::classRef		},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 7; // classses
}



#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("TimeLine 00.000.039 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
