#include "../e6/e6_impl.h"
#include "Script.h"

#include <stdarg.h> 
#include <map> 
#include "jsapi.h"

	//
	// this is the function to wrap:
	//
	////static JSBool
	//// doit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	//// {
	////     /*
	////      * Look in argv for argc actual parameters, set *rval to return a
	////      * value to the caller.
	////      */
	////     ...
	//// }




// global:
e6::Engine * engine = 0;

JSObject * currentProto = 0;

std::map<int,JSClass> classes;
std::map<int,JSObject*> protos;
std::map<const char*,JSObject*> supers;

JSFunctionSpec specs[512];
JSFunctionSpec *theSpec = 0;
int _n_specs = 0;

//! params to 'doit' in a struct
//! i need them in a single chunk, so i can sell 
//! a reference to it to the stackmachine-templytes.
struct JSVM
{
	JSContext *cx;
	JSObject *obj;
	uintN argc;
	jsval *argv;
	jsval *rval;
	JSObject *global;
};


typedef JSVM & VM;
typedef JSBool RetType;
typedef RetType (*SCRIPT_CALLBACK)(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

//! js args start at 0:
const int freeFunctionOffset = 0;
//! 'this' is not on the stack:
const int classMemberOffset = 0;


#include "StackMachine.h"
namespace StackMachine
{

	JSBool retValue( int r )
	{
		return 1; //(r?1:0); // make it bool
	}



	template < class Func, Func ptr > 
	static RetType Function (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		JSVM vm = {cx,obj,argc,argv,rval,0};
		return StackMachine::function( vm, ptr );
	}



	template <class T>
	JSBool Constructor_prohibited( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
  		return 0; //sq_throwerror(vm, "you cannot create an instance of this!" );
	}

	template <class T>
	JSBool Constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		int i=0;
		JSVM vm = {cx,obj,argc,argv,rval,0};
		const char * modName = getString(vm,i);
		const char * interfaceName = getString(vm,i);
		T * t = (T*)engine->createInterface( modName, interfaceName );
		return putObject(vm, t, 1);
	}

	void Dtor(JSContext *cx, JSObject *obj)
	{
		e6::Base * ptr = static_cast< e6::Base * >( JS_GetPrivate( cx, obj ) );
		E_RELEASE(ptr);
		JS_SetPrivate( cx, obj, 0 );
	}

	////! returns prototype
	//JSBool getProp(JSContext *cx, JSObject *obj, jsval id, jsval *rval)
	//{
	//	int x = JSVAL_TO_INT(id);
	//	if (! JSVAL_IS_INT(id)) return JS_FALSE;

	//	switch (x)
	//	{
	//		case 0: 
	//			{
	//				JSObject *proto = JS_GetPrototype(cx, obj);
	//				*(rval) = OBJECT_TO_JSVAL(proto);
	//			}
	//			break;
	//		default: return JS_FALSE;
	//	}
	//	return JS_TRUE;
	//}


	//
	//! handle float3, float4 as arrays:
	//
	void put_arr( VM vm, const float*f, int n )
	{
		JSObject *arr = JS_NewArrayObject( vm.cx, n, 0 );
		for ( int i=0; i<n; i++ )
		{
			jsval vp; 
			JS_NewDoubleValue(vm.cx, f[i], &vp);
			JS_SetElement(vm.cx, arr, i, &vp);
		}
		*(vm.rval) = OBJECT_TO_JSVAL(arr);
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
		return 0;
	}
	
	
	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
		JSObject * obj = 0;
		bool ok = JS_ValueToObject( vm.cx, vm.argv[index], &obj );
		if ( ok )
		{
			*pptarget = static_cast< X* >( JS_GetPrivate( vm.cx, obj ) );
		}
		index ++;
		return ok;
	}

	template < class X >
	X * getThis( VM vm, int &index )
	{
		return static_cast< X* >( JS_GetPrivate( vm.cx, vm.obj ) );
	}

	template < class X >
	int   putObject( VM vm, X * x, int pos )
	{
		if ( ! x )
		{
			// we have to return 'null' at least, since it is a valid answer.
			*(vm.rval) = JSVAL_VOID;  
			return 1;
		}
		int clsID = getClassID<X>();
		JSObject * obj = JS_NewObject( vm.cx, &classes[clsID], protos[clsID], vm.obj );
		JS_SetPrivate( vm.cx, obj, x );
		*(vm.rval) = OBJECT_TO_JSVAL(obj);
		return 1;
	}

	
	float getFloat( VM  vm, int & index )
	{
		jsdouble target=0;
		JS_ValueToNumber(vm.cx, vm.argv[index], &target);
		index ++;
		return target;
	}
	int   putFloat( VM  vm, float f )
	{
        return JS_NewDoubleValue(vm.cx, f, vm.rval);
	}
	
	
	int   getInt( VM  vm,  int & index )
	{
		int32 target=0;
		JS_ValueToInt32(vm.cx, vm.argv[index], &target);
		index ++;
		return target;
	}
	int   putInt( VM  vm, int t )
	{
		return JS_NewNumberValue( vm.cx, t, vm.rval );
	}


	const char * getString( VM vm, int & index )
	{
		JSString * str = JS_ValueToString(vm.cx, vm.argv[index]);
		const char * target = JS_GetStringBytes( str );
		index ++;
		return target;
	}
	int   putString( VM vm, const char * str )
	{
		JSString * jstr = JS_NewString( vm.cx, (char*)str, strlen(str) );
		*(vm.rval) = STRING_TO_JSVAL(jstr);
		return (1 );
	}

	
	void module_start(VM v, const char * modName)
	{
	}


	void class_start(VM vm, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
		JSClass my_class = {
			clsName, JSCLASS_HAS_PRIVATE,
			JS_PropertyStub,JS_PropertyStub,/*StackMachine::getProp*/JS_PropertyStub,JS_PropertyStub,
			JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
			StackMachine::Dtor // JS_FinalizeStub
		};;
		classes[ classID ]= my_class;

		// setup inheritance:
		JSObject * parent   = 0;	
		if ( ! supers.empty() )
		{
			std::map<const char*,JSObject*>::iterator it = supers.find( superName );
			if ( it!=supers.end() )
			{
				parent = it->second;
			}
		}

		theSpec = &specs[_n_specs];
		//static JSPropertySpec e6_properties[] = {
		//	{"prototype", 0,   JSPROP_EXPORTED, StackMachine::getProp},
		//	{0}
		//};

		currentProto = JS_InitClass(
			vm.cx, vm.global, parent, &classes[ classID ],
			// native constructor function and min arg count 
			ctor, 2,
			// prototype object properties and methods
			0, theSpec,
			// static properties and methods 
			0/*e6_properties*/, 0
		);

		supers[ clsName ] = currentProto;
		protos[ classID ] = currentProto;
	}


	void push_variable(VM v, const char * vName, int i)
	{
	}

	void push_method(VM v, const char * fuName, SCRIPT_CALLBACK func, bool isStatic)
	{
	    if ( isStatic )
		{
			JS_DefineFunction(v.cx, v.global, fuName, func, 7, 0);
		}
		else
		{
			JSFunctionSpec spec = {fuName,func,7,0};
			specs[_n_specs++] = spec;
		}
	}


	void class_end(VM vm)
	{
		// terminate spec list:
		static JSFunctionSpec zero = {0,0,0,0};
		specs[_n_specs++] = zero;

		JS_DefineFunctions(vm.cx, currentProto, theSpec);
		currentProto = 0;
	}

	void module_end(VM v)
	{
	}
	
	bool eval_code(VM v, const char * code )
	{
		return 0;
	}
}	

namespace ScriptJS
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

	//! The error reporter callback. 
	void reportError(JSContext *cx, const char *message, JSErrorReport *report)
	{
		char buf[2048];
		sprintf(buf, "%s:%u:%s\r\n",
				report->filename ? report->filename : "<no filename>",
				(unsigned int) report->lineno,
				message);
		l_err->printLog( buf );
	}


	//! print to e6::Logger
	JSBool print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		for ( uintN i=0; i<argc; i++ )
		{
			JSString * str = JS_ValueToString(cx, argv[i]);
			if (!str)
				return JS_FALSE;
			l_out->printLog( JS_GetStringBytes(str) );
			if ( i < argc-1 ) l_out->printLog(" ");
		}
		return 1;
	}




	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{
		JSVM vm;
		JSRuntime * rt ;

		CInterpreter()
			: rt(0)
		{
			$1("CInterpreter()");
			setName( "e6_JS" );
			classes.clear();
			protos.clear();
			supers.clear();
			_n_specs = 0;
		}

		~CInterpreter()
		{
			$1("~CInterpreter()");
			JS_DestroyContext(this->vm.cx);
			JS_DestroyRuntime(this->rt);
		    JS_ShutDown();

			E_RELEASE( engine );
		}

		virtual bool setup( e6::Engine * ngin ) 
		{
			E_ADDREF( ngin );
			E_RELEASE( engine );
			engine = ngin;

			static JSClass global_class = {
				"global",0,
				JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
				JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
			};

			this->rt = JS_NewRuntime(0x100000L);
			if ( ! this->rt ) { printf("no rt !\n"); return 0; }
			this->vm.cx = JS_NewContext(rt, 0x1000);
			if ( ! this->vm.cx ) { printf("no cx !\n"); return 0; }
			this->vm.global =  JS_NewObject(this->vm.cx, &global_class, NULL, NULL);  //JS_GetGlobalObject(cx);
			if ( ! this->vm.global ) { printf("no global !\n"); return 0; }
			
		    JS_SetErrorReporter(this->vm.cx, reportError);
			JS_InitStandardClasses(this->vm.cx, this->vm.global);
			// overwrite builtin 'print'
		    JS_DefineFunction(this->vm.cx, this->vm.global, "print", print, 7, 0);

			int ok = Script::addModules( this->vm );
			return ok;
		}				


		// load & run
		virtual bool exec( const char *code, const char * marker ) 
		{	
			uint nBytes = strlen(code);
			jsval rval;
			int ok = JS_EvaluateScript( this->vm.cx, this->vm.global,
									code, nBytes,
									marker, 13, &rval  );

			// trigger gc on every 2000 bytes of input code:
			static uint gcBytes = 0;
			gcBytes += nBytes;
			if ( gcBytes > 2000 )			
			{
				JS_GC( this->vm.cx );
				gcBytes = 0;
			}
			return ok;
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
			return "js";	
		}

	}; // CInterpreter


} // ScriptJS



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptJS",		ScriptJS::CInterpreter::createSingleton, ScriptJS::CInterpreter::classRef		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptJS 00.000.0061 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

