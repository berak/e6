#include "../e6/e6_impl.h"
#include "Script.h"

#include "angelscript.h"

#include <stdarg.h> 
#include <map> 

// global:
e6::Engine * engine = 0;

typedef asIScriptGeneric * VM;
typedef int RetType;
typedef RetType (*SCRIPT_CALLBACK)( VM vm );
const int classMemberOffset = 0;
const int freeFunctionOffset = 0;

asIScriptEngine *asvm = 0;


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
		int i=classMemberOffset;
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
		return 0;//sq_throwerror( vm, e );
	}

	
	
	
	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
		void * ptr = vm->GetArgAddress(index);
		*pptarget = static_cast<X*>(ptr);
		index ++;
		return 1;
	}

	template < class X >
	X * getThis( VM vm, int &index )
	{
		X* target = static_cast<X*>(vm->GetObject());
		return target;
	}

	///PPP create obj first!!
	template < class X >
	int   putObject( VM vm, X * x, int pos )
	{
		return vm->SetReturnObject(x);;
	}

	float getFloat( VM  vm, int & index )
	{
		float f = vm->GetArgFloat(index);
		index ++;
		return f;
	}
	int   putFloat( VM  vm, float f )
	{
		vm->SetReturnFloat(f);
		return 1;
	}
	int   getInt( VM  vm,  int & index )
	{
		int target = vm->GetArgDWord(index);
		index ++;
		return target;
	}

	int   putInt( VM  vm, int t )
	{
		vm->SetReturnDWord(t);
		return 1;
	}
	const char * getString( VM vm, int & index )
	{
		const char * target = static_cast<char*>(vm->GetArgAddress(index));
		index ++;
		return target;
	}
	int   putString( VM vm, const char * str )
	{
		vm->SetReturnAddress((void*)str);
		return 1;
	}

	void module_start(VM v, const char * modName)
	{
	}

char * theClass = 0;

	void class_start(VM v, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
		theClass = (char*)clsName;
	}


	void push_variable(VM v, const char * vName, int i)
	{
	}

	void push_method(VM v, const char * fuName, SCRIPT_CALLBACK func, bool isStatic)
	{
		if ( isStatic )
		{
			asvm->RegisterGlobalFunction(fuName, asFUNCTION(func), asCALL_GENERIC);
		}
		else
		{
			asvm->RegisterObjectMethod( theClass, "string@ opAdd(uint) const", asFUNCTION(func), asCALL_GENERIC);
		}
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

namespace ScriptAngel
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

	//void compilererror( VM v, const SQChar *dest, const SQChar *src, SQInteger line, SQInteger col  )
	//{
	//	char buf[400];
	//	sprintf( buf, "\r\nCOMPILE ERROR %s [%s, line(%i),col(%i)]\r\n", dest,src,line,col );
	//	l_err->printLog( buf );
	//}

	void MessageCallback(const asSMessageInfo *msg, void *param)
	{
		const char *type = "ERR ";
		if( msg->type == asMSGTYPE_WARNING ) 
			type = "WARN";
		else if( msg->type == asMSGTYPE_INFORMATION ) 
			type = "INFO";

		static char wbuf[8*1024];
		sprintf( wbuf, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
		
		l_out->printLog( wbuf );
	}


	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{

		CInterpreter()
			: asvm(0)
		{
			$1("CInterpreter()");
			setName( "e6_Angel" );
		}
		~CInterpreter()
		{
			$1("~CInterpreter()");

			asvm->Release();
		}

		virtual bool setup( e6::Engine * ngin ) 
		{
			//E_ADDREF( ngin );
			//E_RELEASE( engine );
			engine = ngin;

			asvm = asCreateScriptEngine(ANGELSCRIPT_VERSION);
			if( asvm == 0 )
			{
				return 0;
			}

			// The script compiler will write any compiler messages to the callback.
			asvm->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);


			return 	Script::addModules( 0 );
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


} // ScriptAngel



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptAngel",		ScriptAngel::CInterpreter::createSingleton, ScriptAngel::CInterpreter::classRef		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptAngel 00.000.0061 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

