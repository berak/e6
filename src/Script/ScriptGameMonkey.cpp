#include "../e6/e6_impl.h"
#include "../e6/e6_math.h"
#include "Script.h"

#include <stdarg.h> 
#include <map> 

#ifdef WIN32
 // '_CRT_SECURE_NO_DEPRECATE': Makro-Neudefinition :
 #pragma warning (disable:4005)
 // 'const void *' zu 'gmuint' :
 #pragma warning (disable:4311)
 // gmptr' in größeren Typ 'gmStringObject *' :
 #pragma warning (disable:4312)
#endif

#include "gmThread.h" // game monkey script
#include "gmDebug.h"



using e6::float3;
using e6::float4;

// global:
e6::Engine * engine = 0;

///PPP global, since the thread is not yet created while registering functions
gmMachine * machine = 0;

// clsMap[ classID ] = gmtype 
std::map<int,int> clsMap;

// superMap[ supername ] = gmtype;
std::map<const char*,int> superMap;

int _type = 0;
char *_mName = 0;
char *_cName = 0;


typedef gmThread * VM;
typedef int RetType;
typedef RetType (*SCRIPT_CALLBACK)( VM vm);
const int classMemberOffset = 0;
const int freeFunctionOffset = 0;

#include "StackMachine.h"

//
// here comes all the stuff that has to be defined
// before including "scriptbind.h":
//
namespace StackMachine
{
	int retValue( int r )
	{
		return GM_OK;
	}


	template <class T>
	RetType Constructor( VM vm )
	{
		int i = 0;
		const char * modName = getString(vm,i);
		const char * interfaceName = getString(vm,i);
		T * t = (T*)engine->createInterface( modName, interfaceName );
		return putObject(vm, t, 1);
	}

	template <class T>
	RetType Constructor_prohibited( VM vm )
	{
		return error( vm, "you cannot create an instance of this !");
	}

	template < class Func, Func ptr > 
	RetType Function ( VM vm )
	{
		return StackMachine::function( vm, ptr );
	}


	RetType putArray( VM vm, const float *arr, int n )
	{
		gmTableObject * gmt = vm->PushNewTable();
		for ( int i=0; i<n; i++ )
		{
			gmt->Set( machine, gmVariable( i ), gmVariable(arr[i]) );
		}
		return GM_OK;
	}
	//
	// handle float3 as float array:
	//
	template <> 
	RetType put<float3>(VM vm, float3 f)
	{
		return putArray( vm, &f[0], 3 );
	}
	template <> 
	RetType put<const float3& >(VM vm, const float3 & f)
	{
		return putArray( vm, &f[0], 3 );
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
	//
	// handle float4 as  float array:
	//
	template <> 
	RetType put<float4>(VM vm, float4 f)
	{
		return putArray( vm, &f[0], 4 );
	}
	template <> 
	RetType put<const float4& >(VM vm, const float4 & f)
	{
		return putArray( vm, &f[0], 4 );
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
	int   error(VM vm, const char *e )
	{
		vm->PushNewString( e );
		return GM_EXCEPTION;
	}

	void GCDestruct(gmMachine * a_machine, gmUserObject* a_object)
	{
		e6::Base* object = (e6::Base*)a_object->m_user;
		E_RELEASE( object );
	}
	
	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{

		*pptarget = static_cast<X*>(vm->ParamUser_NoCheckTypeOrParam(index));
		index ++;
		return GM_OK;
	}

	template < class X >
	X *getThis( VM vm, int &index )
	{
		X *x = static_cast<X*>(vm->ThisUser_NoChecks());
		return x;
	}

	template < class X >
	int putObject( VM vm, X * x, int pos )
	{
		int type = clsMap[ getClassID<X>() ];
		if ( x )
		{
			vm->PushNewUser(x,type);
		}
		else
		{
			vm->PushNull();
		}
		return GM_OK;
	}

	float getFloat( VM  vm, int & index )
	{
		float f = vm->ParamFloatOrInt(index);
		index ++;
		return f;
	}
	int   putFloat( VM  vm, float f )
	{
		vm->PushFloat(f);
		return retValue(1);
	}
	int   getInt( VM  vm,  int & index )
	{
		int i = vm->ParamInt(index);
		index ++;
		return i;
	}

	int   putInt( VM  vm, int i )
	{
		vm->PushInt(i);
		return GM_OK;
	}
	const char * getString( VM vm, int & index )
	{
		const char * target = vm->ParamString(index);
		index ++;
		return target;
	}
	int   putString( VM vm, const char * str )
	{
		if ( str ) 
		{
			vm->PushNewString(str);
		}
		else
		{
			vm->PushNull();
		}
		return GM_OK;
	}

}


#include "ScriptBind.h"

namespace StackMachine
{

	void module_start(VM vm, const char * modName)
	{
		_mName = (char*)modName;
	}

	void class_start(VM vm, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
		_cName = (char*)clsName;
		_type = machine->CreateUserType(clsName);
		clsMap[ classID ] = _type;
		superMap[ clsName ] = _type;

		machine->RegisterLibraryFunction( clsName, ctor, "e6" );	
		machine->RegisterUserCallbacks( _type, NULL, GCDestruct, NULL ); 

		if ( superName )
		{
			// clone superobject's metatable:
			int superType = superMap[ superName ];
			gmTableObject * myTable = machine->GetTypeTable(_type);
			gmTableObject * superTable = machine->GetTypeTable(superType);

			int it = 0;
			gmTableNode * node = superTable->GetFirst( it );
			for ( node; node; node = superTable->GetNext( it ) )
			{
				myTable->Set( machine, node->m_key, node->m_value );
			}
		}
	}


	void push_variable(VM vm, const char * vName, int i)
	{
		machine->GetGlobals()->Set(machine, "e6", gmVariable(i)); 
	}

	void push_method(VM vm, const char * fuName, SCRIPT_CALLBACK func, bool isStatic)
	{
		if ( isStatic )
		{
			machine->RegisterLibraryFunction( fuName, func, "e6" );	
		}
		else
		{
			machine->RegisterTypeLibraryFunction( _type, fuName, func );
		}
	}


	void class_end(VM vm)
	{
		_type  = 0;
		_cName = 0;
	}

	void module_end(VM vm)
	{
		_mName = 0;
	}
	
}	

namespace ScriptGameMonkey
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

	void checkErrors( const char * marker ) 
	{
		l_err->printLog(marker);
		l_err->printLog(": ");

		bool first = true;
		const char * message;
		while((message = machine->GetLog().GetEntry(first))) 
		{
			l_err->printLog(message);
		}
		machine->GetLog().Reset();
	}
	void printCallback(gmMachine * a_machine, const char * a_string)
	{ 
		l_out->printLog( a_string );
	} 

	bool machineCallback(gmMachine* a_machine, gmMachineCommand a_command, const void* a_context) 
	{ 
		switch(a_command) 
		{ 
			case MC_THREAD_EXCEPTION: 
			{ 
				// Dump runtime exceptions to logger 
				checkErrors( "runtime" );
				break; 
			} 
			case MC_COLLECT_GARBAGE: 
			{ 
				break;
			}
			case MC_THREAD_CREATE:
			{ 
				// Called when a thread is created.  a_context is the thread. 
				break; 
			} 
			case MC_THREAD_DESTROY: 
			{
				//a_machine->GetGC()->DestructAll();
				// Called when a thread is destroyed.  a_context is the thread that is about to die 
				break; 
			} 
		}
		return 0;
	}

	int printClasses( gmThread * vm )
	{
		for ( int type=0; type<100; type++ )
		{
			char buf[400];
			int it = 0;
			gmTableObject * table = machine->GetTypeTable(type);
			if ( ! table ) break;
			sprintf( buf, "[%s]\r\n", machine->GetTypeName(type) );
			l_out->printLog( buf );
			gmTableNode * node = table->GetFirst( it );
			for ( node; node; node = table->GetNext( it ) )
			{
				char s[200];
				gmVariable & k = node->m_key;
				gmVariable & v = node->m_value;
				k.AsStringWithType(machine, s, 200);
				sprintf( buf, "	%s = ",  s );
				l_out->printLog( buf );
				v.AsString(machine, s, 200);
				sprintf( buf, "%s\r\n", s );
				l_out->printLog( buf );
			}
		}
		return GM_OK;
	}


	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{

		gmDebugSession debugSession;

		CInterpreter()
		{
			$1("CInterpreter()");
			setName( "e6_GameMonkey" );
		}
		~CInterpreter()
		{
			E_DELETE( machine );
			E_RELEASE( engine );
			$1("~CInterpreter()");
		}

 		virtual bool setup( e6::Engine * ngin ) 
		{
			E_ADDREF( ngin );
			E_RELEASE( engine );
			engine = ngin;

			machine = new gmMachine;
			machine->s_printCallback = printCallback;
			machine->s_machineCallback = machineCallback; 
			machine->SetDebugMode(true);
			gmBindDebugLib(machine); // Register debugging library
			//m_debuggerIP = DEBUGGER_DEFAULT_IP;
			//m_debuggerPort = DEBUGGER_DEFAULT_PORT;
			//debugSession.m_sendMessage = DebuggerSendMessage;
			//debugSession.m_pumpMessage = DebuggerPumpMessage;
			//debugSession.m_user = &m_debugClient;

			//if(m_debugClient.Connect("127.0.0.1", ((short) 49001)))
			//{
			//	debugSession.Open(machine);
			////fprintf(stderr, "Debug session opened"GM_NL);
			//}
			machine->RegisterLibraryFunction( "printClasses", printClasses, "e6" );	

			return 	Script::addModules( 0 );
		}				


		// load & run
		virtual bool exec( const char *code, const char * marker ) 
		{
			// gm doesn't like empy code ..
			if ( ! code || ! code[0] || (code[0]=='\n'&&strlen(code)==1) )
			{
				// just go on, nothing happened...
				return true;
			}

			if( GM_OK != machine->ExecuteString( code, 0, true, marker ) )
			{
				// Dump compile time (parser) errors to output
				checkErrors(marker);
				return false;
			}

			return true;
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

		virtual void setOutlog( e6::Logger & lg )  { l_out = &lg ;}
		virtual void setErrlog( e6::Logger & lg )  { l_err = &lg ;}

		virtual const char * getFileExtension() const 
		{
			return "gm";
		}
	};
};



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptGameMonkey",		ScriptGameMonkey::CInterpreter::createSingleton,		ScriptGameMonkey::CInterpreter::classRef },
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptGameMonkey 00.000.0001 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

