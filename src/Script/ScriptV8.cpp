#include "../e6/e6_impl.h"
#include "Script.h"
#include "v8.h"

#include <stdarg.h> 
#include <map> 

//
// the callback type for v8:
//
//  v8::Handle<v8::Value> function(const v8::Arguments& args) ;



// global:
e6::Engine * engine = 0;
v8::Handle<v8::ObjectTemplate> global_v8;


namespace StackMachine
{
	typedef const v8::Arguments & VM;
	typedef v8::Handle<v8::Value> RetType;
	typedef RetType (*SCRIPT_CALLBACK)( VM vm );
	RetType retValue( int r )
	{
		return v8::Undefined();
	}
}
const int classMemberOffset = 1;
const int freeFunctionOffset = 0;

#include "StackMachine.h"
namespace StackMachine
{

	template <class T>
	RetType Constructor_prohibited( VM vm )
	{
  		return v8::Undefined(); 
	}

	template <class T>
	RetType Constructor( VM vm )
	{
		int i=0;
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
	}
	template <> 
	RetType put<const float3& >(VM vm, const float3 & f)
	{
		put_arr( vm, &(f.x), 3 );
		// fprintf( stderr, "%s(const float3&(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		//return retValue(1);
		return v8::Undefined();
	}
	template <> 
	RetType put<float3>(VM vm, float3 f)
	{
		put_arr( vm, &(f.x), 3 );
		// fprintf( stderr, "%s(float3(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		//return retValue(1);
		return v8::Undefined();
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
		return v8::Undefined();
	}
	template <> 
	RetType put<float4>(VM vm, float4 f)
	{
		put_arr( vm, &(f.x), 4 );
		// fprintf( stderr, "%s(float3(%2.2f %2.2f %2.2f))\n",__FUNCTION__, f[0],f[1],f[2] );
		return v8::Undefined();
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
	
	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
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
	RetType  putObject( VM vm, X * x, int pos )
	{
		return v8::Undefined();
	}

	float getFloat( VM  vm, int & index )
	{
		float i=0;
		// fprintf( stderr, "%s<float>(%i)(%f)\n", __FUNCTION__, index, i );
		//dbg(vm);
		index ++;
		return i;
	}
	RetType  putFloat( VM  vm, float f )
	{
		//dbg(vm);
		// fprintf( stderr, "%s<float>(%f)\n", __FUNCTION__,f );
		//return 1;
		return v8::Undefined();
	}
	int   getInt( VM  vm,  int & index )
	{
		int target=vm[index]->Int32Value();
		// fprintf( stderr, "%s<int>(%i)(%i)\n", __FUNCTION__, target, index );
		index ++;
		return target;
	}

	RetType   putInt( VM  vm, int t )
	{
		// fprintf( stderr, "%s<int>(%i)\n", __FUNCTION__, t );
		//return 1;
		return v8::Undefined();
	}
	const char * getString( VM vm, int & index )
	{
		const char * target=0;
		// fprintf( stderr, "%s<const char*>(%s)(%i)\n", __FUNCTION__, target, index );
		index ++;
		return target;
	}
	RetType   putString( VM vm, const char * str )
	{
		//return 1;
		return v8::Undefined();
	}

	void module_start(VM v, const char * modName)
	{
	}

	void class_start(VM v, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
	}


	void push_variable(VM v, const char * vName, int i)
	{
	}

	void push_method(VM v, const char * fuName, SCRIPT_CALLBACK func, bool isStatic)
	{
	}


	void class_end(VM v)
	{
	}

	void module_end(VM v)
	{
	}
	
	bool eval_code(VM v, const char * code )
	{
		return 0;
	}
}	

namespace ScriptV8
{
// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}
	v8::Handle<v8::Value> demo(const v8::Arguments& args) {
	  bool first = true;
	  for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope;
		if (first) {
		  first = false;
		} else {
		  printf(" ");
		}
		v8::String::Utf8Value str(args[i]);
		const char* cstr = ToCString(str);
		printf("%s", cstr);
	  }
	  printf("\n");
	  fflush(stdout);
	  return v8::Undefined();
	}

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

	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{
		v8::Handle<v8::Context> context;

		CInterpreter()
		{
			$1("CInterpreter()");
			setName( "e6_V8" );
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

			v8::ObjectTemplate::New();
			Script::addModules( v8::Undefined() );
			//// Bind the global 'print' function to the C++ Print callback.
			global_v8->Set(v8::String::New("print"), v8::FunctionTemplate::New(demo));
			//// Bind the global 'read' function to the C++ Read callback.
			//global->Set(v8::String::New("read"), v8::FunctionTemplate::New(Read));
			//// Bind the global 'load' function to the C++ Load callback.
			//global->Set(v8::String::New("load"), v8::FunctionTemplate::New(Load));
			//// Bind the 'quit' function
			//global->Set(v8::String::New("quit"), v8::FunctionTemplate::New(Quit));
			//// Bind the 'version' function
			//global->Set(v8::String::New("version"), v8::FunctionTemplate::New(Version));
			// Create a new execution environment containing the built-in
			// functions
			context = v8::Context::New(NULL, global_v8);
			// Enter the newly created execution environment.
			v8::Context::Scope context_scope(context);
			//bool run_shell = (argc == 1);
			//for (int i = 1; i < argc; i++) {
			//const char* str = argv[i];
			//if (strcmp(str, "--shell") == 0) {
			//run_shell = true;
			//} else if (strcmp(str, "-f") == 0) {
			//// Ignore any -f flags for compatibility with the other stand-
			//// alone JavaScript engines.
			//continue;
			//} else if (strncmp(str, "--", 2) == 0) {
			//printf("Warning: unknown flag %s.\nTry --help for options\n", str);
			//} else if (strcmp(str, "-e") == 0 && i + 1 < argc) {
			//// Execute argument given to -e option directly
			//v8::HandleScope handle_scope;
			//v8::Handle<v8::String> file_name = v8::String::New("unnamed");
			//v8::Handle<v8::String> source = v8::String::New(argv[i + 1]);
			//if (!ExecuteString(source, file_name, false, true))
			//return 1;
			//i++;
			//} else {
			//// Use all other arguments as names of files to load and run.
			//v8::HandleScope handle_scope;
			//v8::Handle<v8::String> file_name = v8::String::New(str);
			//v8::Handle<v8::String> source = ReadFile(str);
			//if (source.IsEmpty()) {
			//printf("Error reading '%s'\n", str);
			//return 1;
			//}
			//if (!ExecuteString(source, file_name, false, true))
			//}
			//}
			return 1;
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
			return "js";	
		}

	}; // CInterpreter


} // ScriptV8



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptV8",		ScriptV8::CInterpreter::createSingleton, ScriptV8::CInterpreter::classRef		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptV8 00.000.0061 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

