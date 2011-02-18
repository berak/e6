#include "../e6/e6_impl.h"
#include "Script.h"

#include <stdarg.h> 
#include <map> 

// global:
e6::Engine * engine = 0;

typedef HSQUIRRELVM VM;
typedef int RetType;
typedef RetType (*CALLBACK)( VM vm );
const int classMemberOffset = 1;
const int freeFunctionOffset = 2;

#include "StackMachine.h"
namespace StackMachine
{

	int retValue( int r )
	{
		return r;
	}

	template <class T>
	int Constructor_prohibited( VM vm )
	{
  		return 0; //sq_throwerror(vm, "you cannot create an instance of this!" );
	}

	template <class T>
	int Constructor( VM vm )
	{
		int i=2;
		const char * modName = getString(vm,i);
		const char * interfaceName = getString(vm,i);
		T * t = (T*)engine->createInterface( modName, interfaceName );
		return putObject(vm, t, 1);
	}

	template < class Func, Func ptr > 
	static RetType Function ( VM vm )
	{
		return StackMachine::function( vm, ptr );
	}

	//
	// handle float3, float4 as arrays:
	//
	void put_arr( VM vm, const float*f, int n )
	{
		sq_newarray( vm, 0 );
		int pos = sq_gettop(vm);
		for ( int i=0; i<n; i++ )
		{
			sq_pushfloat( vm, f[i] );
			sq_arrayappend( vm, -2 );
			sq_settop(vm,pos); // thefloat
		}
	}
	template <> 
	RetType put<const float3& >(VM vm, const float3 & f)
	{
		put_arr( vm, &(f.x), 3 );
		// fprintf( stderr, "%s(const float3&(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return 1;
	}
	template <> 
	RetType put<float3>(VM vm, float3 f)
	{
		put_arr( vm, &(f.x), 3 );
		// fprintf( stderr, "%s(float3(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return 1;
	}
	template <> 
	float3 get(VM vm, int &i)
	{
		float3 f;
		f[0] = getFloat( vm, i );
		f[1] = getFloat( vm, i );
		f[2] = getFloat( vm, i );
		// fprintf( stderr, "%s(float3(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return f;
	}
	template <> 
	const float3 & get(VM vm, int &i)
	{
		static float3 f; ///PPP uuuuuuuuuuaaaaaaaaa
		f[0] = getFloat( vm, i );
		f[1] = getFloat( vm, i );
		f[2] = getFloat( vm, i );
		// fprintf( stderr, "%s(const float3&(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return f;
	}

	template <> 
	RetType put<const float4& >(VM vm, const float4 & f)
	{
		put_arr( vm, &(f.x), 4 );
		// fprintf( stderr, "%s(const float3&(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return 1;
	}
	template <> 
	RetType put<float4>(VM vm, float4 f)
	{
		put_arr( vm, &(f.x), 4 );
		// fprintf( stderr, "%s(float3(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return 1;
	}
	template <> 
	float4 get(VM vm, int &i)
	{
		float4 f;
		f[0] = getFloat( vm, i );
		f[1] = getFloat( vm, i );
		f[2] = getFloat( vm, i );
		f[3] = getFloat( vm, i );
		// fprintf( stderr, "%s(float3(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return f;
	}
	template <> 
	const float4 & get(VM vm, int &i)
	{
		static float4 f; ///PPP uuuuuuuuuuaaaaaaaaa
		f[0] = getFloat( vm, i );
		f[1] = getFloat( vm, i );
		f[2] = getFloat( vm, i );
		f[3] = getFloat( vm, i );
		// fprintf( stderr, "%s(const float3&(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return f;
	}
}



#include "ScriptBind.h"


namespace StackMachine
{
	int   error(VM vm, const char *e )
	{
		return sq_throwerror( vm, e );
	}

	
	
	
	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
//		bool r = SQ_SUCCEEDED( sq_getinstanceup( vm, index, (SQUserPointer*)(pptarget), 0 /*getClassType<X>()*/ ) );
		// fprintf( stderr, "%s<%s>(%i)(%x)\n", __FUNCTION__,typeid(*pptarget).name(), index, *pptarget );
		index ++;
		return r;
	}

	template < class X >
	X * getThis( VM vm, int &index )
	{
		X* target = 0;
		getObject( vm, &target, index );
		return target;
	}

	template < class X >
	int   putObject( VM vm, X * x, int pos )
	{
		return 1;
	}

	float getFloat( VM  vm, int & index )
	{
		float i=0;
//		sq_getfloat(vm,index,&i);
		// fprintf( stderr, "%s<float>(%i)(%f)\n", __FUNCTION__, index, i );
		//dbg(vm);
		index ++;
		return i;
	}
	int   putFloat( VM  vm, float f )
	{
//		sq_pushfloat(vm,f);
		//dbg(vm);
		// fprintf( stderr, "%s<float>(%f)\n", __FUNCTION__,f );
		return 1;
	}
	int   getInt( VM  vm,  int & index )
	{
		int target=0;
//		sq_getinteger(vm,index,&target);
		// fprintf( stderr, "%s<int>(%i)(%i)\n", __FUNCTION__, target, index );
		index ++;
		return target;
	}

	int   putInt( VM  vm, int t )
	{
		// fprintf( stderr, "%s<int>(%i)\n", __FUNCTION__, t );
		return 1;
	}
	const char * getString( VM vm, int & index )
	{
		const char * target=0;
//		sq_getstring(vm,index,(const SQChar **)&target);
		// fprintf( stderr, "%s<const char*>(%s)(%i)\n", __FUNCTION__, target, index );
		index ++;
		return target;
	}
	int   putString( VM vm, const char * str )
	{
		return 1;
	}

	void module_start(HSQUIRRELVM v, const char * modName)
	{
	}

	void class_start(HSQUIRRELVM v, const char * clsName, const char * superName, int classID, CALLBACK ctor)
	{
	}


	void push_variable(HSQUIRRELVM v, const char * vName, int i)
	{
	}

	void push_method(HSQUIRRELVM v, const char * fuName, SQFUNCTION func, bool isStatic)
	{
	}


	void class_end(HSQUIRRELVM v)
	{
	}

	void module_end(HSQUIRRELVM v)
	{
	}
	
	bool eval_code(HSQUIRRELVM v, const char * code )
	{
		return 0;
	}
}	

namespace ScriptSquirrel
{
	struct DefLogger : e6::Logger 
	{
		virtual bool printLog( const char * s )  
		{
			fprintf( stdout,  s );
			return 1;
		}
	} _defLog;

	e6::Logger * l_err = &_defLog;
	e6::Logger * l_out = &_defLog;

	//void compilererror( HSQUIRRELVM v, const SQChar *dest, const SQChar *src, SQInteger line, SQInteger col  )
	//{
	//	char buf[400];
	//	sprintf( buf, "\r\nCOMPILE ERROR %s [%s, line(%i),col(%i)]\r\n", dest,src,line,col );
	//	l_err->printLog( buf );
	//}

	void printfunc(HSQUIRRELVM v, const SQChar *format, ...) 
	{ 
		static SQChar wbuf[8*1024];
		wbuf[0] = 0;
		va_list vargs; 
		va_start( vargs, format );
		vsprintf( wbuf, format, vargs );
		va_end( vargs );
	
		l_out->printLog( wbuf );
	} 
	


	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{
		HSQUIRRELVM v;

		CInterpreter()
			: v(0)
		{
			$1("CInterpreter()");
			setName( "e6_Squirrel" );
		}
		~CInterpreter()
		{
			$1("~CInterpreter()");
		}

		virtual bool setup( e6::Engine * ngin ) 
		{
			//E_ADDREF( ngin );
			//E_RELEASE( engine );
			engine = ngin;


			return 	Script::addModules( v );
		}				


		// load & run
		virtual bool exec( const char *code, const char * marker ) 
		{
			return false;
		}

		// run  function in compiled code
		virtual bool call( const char *function, const char *marker  ) 	
		{
			return 0; //NOT_IMPL
		}
		// run function in compiled code
		virtual bool call( const char *function, float f, const char *marker  ) 
		{
			return 0; //NOT_IMPL
		}

		virtual void setOutlog( e6::Logger & lg )  		
		{ 
			l_out = &lg ;
		}
		virtual void setErrlog( e6::Logger & lg )  		
		{ 
			l_err = &lg ;
		}

		virtual const char * getFileExtension() const 	
		{	
			return "nut";	
		}

	}; // CInterpreter


} // ScriptSquirrel



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptSquirrel",		ScriptSquirrel::CInterpreter::createSingleton, ScriptSquirrel::CInterpreter::classRef		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptSquirrel 00.000.0061 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

