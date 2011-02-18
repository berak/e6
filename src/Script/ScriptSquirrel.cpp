#include "../e6/e6_impl.h"
#include "Script.h"

#include <stdarg.h> 
#include <map> 

extern "C"
{
	#include <squirrel.h> 
	#include <sqstdio.h> 
	#include <sqstdaux.h> 
	#include <sqstdstring.h> 
	#include <sqstdmath.h> 
	#include <sqstdblob.h> 
	#include <sqstdsystem.h> 
};

// global:
e6::Engine * engine = 0;
std::map<int,const char*> clsMap;

typedef HSQUIRRELVM VM;
typedef int RetType;
typedef RetType (*SCRIPT_CALLBACK)( VM vm );
const int classMemberOffset = 1;
const int freeFunctionOffset = 2;

#include "StackMachine.h"
namespace StackMachine
{
	int dbg(VM vm);

	int retValue( int r )
	{
		return r;
	}

	template <class T>
	int Constructor_prohibited( VM vm )
	{
  		return sq_throwerror(vm, "you cannot create an instance of this!" );
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
		return 1;
	}
	template <> 
	RetType put<float3>(VM vm, float3 f)
	{
		put_arr( vm, &(f.x), 3 );
		return 1;
	}
	template <> 
	float3 get(VM vm, int &i)
	{
		float3 f;
		f[0] = getFloat( vm, i );
		f[1] = getFloat( vm, i );
		f[2] = getFloat( vm, i );
		return f;
	}
	template <> 
	const float3 & get(VM vm, int &i)
	{
		static float3 f; ///PPP uuuuuuuuuuaaaaaaaaa
		f[0] = getFloat( vm, i );
		f[1] = getFloat( vm, i );
		f[2] = getFloat( vm, i );
		return f;
	}

	template <> 
	RetType put<const float4& >(VM vm, const float4 & f)
	{
		put_arr( vm, &(f.x), 4 );
		return 1;
	}
	template <> 
	RetType put<float4>(VM vm, float4 f)
	{
		put_arr( vm, &(f.x), 4 );
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

	
	template <class T>
	static SQInteger __releasehook(SQUserPointer p, SQInteger size)
	{
		T *self = static_cast<T*>(p);
		if ( ! self ) 
			return 0;
		E_RELEASE( self );
		return 1;
	}
	
	
	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
		bool r = SQ_SUCCEEDED( sq_getinstanceup( vm, index, (SQUserPointer*)(pptarget), 0 /*getClassType<X>()*/ ) );
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

	int dbg(VM vm)
	{
		int n = sq_gettop(vm);
		printf( "stack : %d elems.\n",n );
		for ( int i=0; i<=n; i++ )
		{
			HSQOBJECT o;
			sq_getstackobj( vm, i, &o );
			uint t = sq_gettype(vm,i);
			switch(t)
			{
				case OT_INSTANCE:
					printf( "%d INSTANCE\t%x\n",i,  o._unVal.pInstance );
					break;
				case OT_CLASS:
					printf( "%d CLASS\t%x\n",i,  o._unVal.pClass );
					break;
				case OT_CLOSURE:
					printf( "%d CLOSURE\t%x\n",i, o._unVal.pClosure );
					break;
				case OT_NATIVECLOSURE:
					printf( "%d NCLOSURE\t%x\n",i, o._unVal.pNativeClosure );
					break;
				case OT_INTEGER:
					printf( "%d INTEGER\t%i\n",i, o._unVal.nInteger );
					break;
				case OT_FLOAT:
					printf( "%d FLOAT\t%f\n",i, o._unVal.fFloat );
					break;
				case OT_TABLE:
					printf( "%d TABLE\t%x\n",i, o._unVal.pTable );
					break;
				case OT_ARRAY:
					printf( "%d ARRAY\t%x\n",i, o._unVal.pArray );
					break;
				default:
					printf( "%d %x %s\n",i, t, sq_objtostring(&o) );
					break;
			}		
		}
		return n;
	}

	template< class T >
	int newInstance(VM vm)
	{
		int clsID = getClassID<T>();
		const char * clsName = clsMap[ clsID ];
		sq_pushroottable(vm);
		sq_pushstring(vm,clsName,-1);
		sq_rawget(vm,-2); // push classobject
		sq_createinstance(vm,-1);
		return -1;
	}

	template < class X >
	int   putObject( VM vm, X * x, int pos )
	{
		if ( ! x )
		{
			sq_pushnull(vm); // don't allow 'empty' instances.
			return 1;
		}
		if ( pos != 1 )
		{
			pos = newInstance<X>(vm);
		}
		if(SQ_FAILED(sq_setinstanceup(vm,pos,x))) 
		{
			assert( !"setinstance failed!" );
			return sq_throwerror(vm, ("cannot put instance"));
		}
	
		sq_setreleasehook(vm,pos,__releasehook<X>);
	
		while( sq_gettype(vm,sq_gettop(vm)) != OT_INSTANCE )
		{
			sq_poptop(vm);
		}
		return 1;
	}

	float getFloat( VM  vm, int & index )
	{
		float i=0;
		sq_getfloat(vm,index,&i);
		index ++;
		return i;
	}
	int   putFloat( VM  vm, float f )
	{
		sq_pushfloat(vm,f);
		return 1;
	}
	int   getInt( VM  vm,  int & index )
	{
		int target=0;
		sq_getinteger(vm,index,&target);
		index ++;
		return target;
	}

	int   putInt( VM  vm, int t )
	{
		sq_pushinteger(vm,t);
		return 1;
	}
	const char * getString( VM vm, int & index )
	{
		const char * target=0;
		sq_getstring(vm,index,(const SQChar **)&target);
		index ++;
		return target;
	}
	int   putString( VM vm, const char * str )
	{
		sq_pushstring(vm,str,-1);
		return 1;
	}

	void module_start(HSQUIRRELVM v, const char * modName)
	{
		sq_pushstring(v,(modName),-1);
		sq_newtable(v);
	}

	void class_start(HSQUIRRELVM v, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
		sq_pushstring(v,(clsName),-1);
		if ( superName )
		{
			sq_pushstring(v,(superName),-1);
			sq_get(v,-3);
		}
		sq_newclass(v,(superName!=0));
		clsMap[ classID ] = clsName;
		push_method( v, "constructor", ctor, 1 );
	}


	void push_variable(HSQUIRRELVM v, const char * vName, int i)
	{
		sq_pushstring(v,vName,-1);
		sq_pushinteger(v,i);
		sq_newslot(v,-3,0);
	}

	void push_method(HSQUIRRELVM v, const char * fuName, SQFUNCTION func, bool isStatic)
	{
		sq_pushstring(v,fuName,-1);
		sq_newclosure(v,func,0);
		sq_setnativeclosurename(v,-1,fuName);
		sq_newslot(v,-3,isStatic);
	}


	void class_end(HSQUIRRELVM v)
	{
		sq_newslot(v,-3,true);
	}

	void module_end(HSQUIRRELVM v)
	{
		sq_newslot(v,-3,true);
	}
	
	bool eval_code(HSQUIRRELVM v, const char * code )
	{
		sq_compilebuffer(v,(code),0,"-code-",true);
		int retval=0;
		sq_push(v,-2);
		if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue))) 
		{
			sq_remove(v,retval?-2:-1); //removes the closure
			return 1;
		}
		sq_pop(v,1); //removes the closure
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
			sq_close(v);
			E_RELEASE( engine );
			$1("~CInterpreter()");
		}

		virtual bool setup( e6::Engine * ngin ) 
		{
			E_ADDREF( ngin );
			E_RELEASE( engine );
			engine = ngin;

			v = sq_open(1024); // creates a VM with initial stack size 1024 
			sq_setprintfunc(v, printfunc); //sets the print function

			//sq_setcompilererrorhandler(v,compilererror);
			//sqstd_seterrorhandlers(v); //registers the default error handlers

			sq_pushroottable(v); //push the root table were to register the lib function

			//sqstd_register_bloblib(v);
			//sqstd_register_iolib(v);
			//sqstd_register_systemlib(v);
			//sqstd_register_mathlib(v);
			//sqstd_register_stringlib(v);

			return 	Script::addModules( v );
		}				

		void dumpError( const char * pre )
		{
			// push error on top and retrieve it:
			const SQChar * c = 0;
			sq_getlasterror( v );
			sq_getstring( v, sq_gettop(v), &c );

			char buf[400];
			sprintf( buf, "\r\n%s : [ %s ]\r\n", pre, c );
			l_err->printLog( buf );

			sq_pop(v,1); //removes the string
		}

		bool call(int retval=0) 
		{
			sq_push(v,-2);
			if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue))) 
			{
				sq_remove(v,retval?-2:-1); //removes the closure
				return true;
			}
			sq_pop(v,1); //removes the closure
			dumpError( "Call Error" );
			return false;
		}	

		// load & run
		virtual bool exec( const char *code, const char * marker ) 
		{
			size_t l = strlen(code);
			if ( SQ_SUCCEEDED( sq_compilebuffer(v,(code),l,marker,true) ) )
			{
				return call(0);
			}
			dumpError( "Compile Error" );
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

