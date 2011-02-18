#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "Script.h"

extern "C"
{
	#include "tcl.h"
};

#include <stdarg.h> 
#include <map> 






int
Tcl_EvalFile(Tcl_Interp* interp, char* fileName)
{
	assert(!"unused!!");
    return TCL_ERROR;
}

// global:
e6::Engine * engine = 0;

std::map < e6::uint, Tcl_ObjType > types;
//int _n_types = 0;

// this is, what we're wrapping:
//int tcl_func(ClientData /*cdata*/, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])

struct TCLVM
{
	Tcl_Interp* interp;
	int argc;
	Tcl_Obj* const *argv;
	ClientData cdata;

//	TCLVM() : interp(0), argc(0), argv(0), cdata(0) {}
};

typedef TCLVM & VM;
typedef int RetType;

typedef RetType (*SCRIPT_CALLBACK)( ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] );

// the first arg in tcl is the function name, so args start at 1.
const int classMemberOffset = 1;
const int freeFunctionOffset = 1;

#include "StackMachine.h"
namespace StackMachine
{

	int retValue( int r )
	{
		return TCL_OK;
	}

	template < class Func, Func ptr > 
	static RetType Function ( ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]  )
	{
		TCLVM vm = {interp,objc,objv,cdata};

		return StackMachine::function( vm, ptr );
	}

	template <class T>
	int Constructor_prohibited(ClientData /*cdata*/, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] )
	{
  		return TCL_ERROR ; //sq_throwerror(vm, "you cannot create an instance of this!" );
	}

	template <class T>
	int Constructor( ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] )
	{
		int i=1;
		TCLVM vm = {interp,objc,objv,cdata};
		const char * modName = getString(vm,i);
		const char * interfaceName = getString(vm,i);
		T * t = (T*)engine->createInterface( modName, interfaceName );
		return putObject(vm, t, 1);
	}

	//
	// handle float3, float4 as arrays:
	//
	void put_arr( VM vm, const float*f, int n )
	{
		Tcl_Obj * list = Tcl_NewListObj( 0,0 );
		for ( int i=0; i<n; i++ )
		{
			Tcl_ListObjAppendElement(vm.interp, list, Tcl_NewDoubleObj(f[i]));
		}
		Tcl_SetObjResult(vm.interp,list);
	}
	template <> 
	RetType put<const float3& >(VM vm, const float3 & f)
	{
		put_arr( vm, &(f.x), 3 );
		return TCL_OK;
	}
	template <> 
	RetType put<float3>(VM vm, float3 f)
	{
		put_arr( vm, &(f.x), 3 );
		return TCL_OK;
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
		return TCL_OK;
	}
	template <> 
	RetType put<float4>(VM vm, float4 f)
	{
		put_arr( vm, &(f.x), 4 );
		return TCL_OK;
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



int e6_tcl_SetFromAnyProc( Tcl_Interp *interp, Tcl_Obj *objPtr)
{
	return 0;
}
void e6_tcl_UpdateStringProc( Tcl_Obj *objPtr )
{
	e6::Base * base = (e6::Base *)(objPtr->internalRep.otherValuePtr);
	const char * str = "none";
	Tcl_SetStringObj(objPtr,(char*)str,strlen(str));
}
void e6_tcl_DupInternalRepProc( Tcl_Obj *srcPtr, Tcl_Obj *dupPtr )
{
	e6::Base * src = (e6::Base *)(srcPtr->internalRep.otherValuePtr);
	E_ADDREF(src);
	dupPtr->internalRep.otherValuePtr = src;
}
void e6_tcl_FreeInternalRepProc( Tcl_Obj *objPtr )
{
	e6::Base * base = (e6::Base *)(objPtr->internalRep.otherValuePtr);
	E_RELEASE( base );
}


namespace StackMachine
{
	int   error(VM vm, const char *e )
	{
		Tcl_AddErrorInfo(vm.interp, e);
		return TCL_ERROR;
	}

	
	
	
	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
		*pptarget = 0;
		assert( index < vm.argc );

		Tcl_Obj * obj = vm.argv[index];
		if ( obj ) 
		{
			*pptarget = static_cast<X*>( obj->internalRep.otherValuePtr );
		}
		index ++;
		return (*pptarget != 0);
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
		vm.cdata = x;
		Tcl_Obj * obj = Tcl_NewObj();
		obj->typePtr = &types[ getClassID<X>() ];
		obj->internalRep.otherValuePtr = x;
		Tcl_SetObjResult( vm.interp, obj );
		return TCL_OK;
	}

	float getFloat( VM  vm, int & index )
	{
		double target=0;
        assert (Tcl_GetDoubleFromObj(vm.interp, vm.argv[index], &target) == TCL_OK);
		index ++;
		return (float)target;
	}
	int   putFloat( VM  vm, float f )
	{
		Tcl_SetObjResult(vm.interp, Tcl_NewDoubleObj(f));
		return TCL_OK;
	}
	int   getInt( VM  vm, int & index )
	{
		int target=0;
        assert (Tcl_GetIntFromObj(vm.interp, vm.argv[index], &target) == TCL_OK);
		index ++;
		return target;
	}

	int   putInt( VM  vm, int t )
	{
		Tcl_SetObjResult(vm.interp, Tcl_NewIntObj(t));
		return 1;
	}
	const char * getString( VM vm, int & index )
	{
		const char * target = Tcl_GetString(vm.argv[index]);
		index ++;
		return target;
	}
	int   putString( VM vm, const char * str )
	{
		Tcl_SetObjResult(vm.interp, Tcl_NewStringObj(str,strlen(str)));
		return TCL_OK;
	}

	void module_start(VM vm, const char * modName)
	{
	}

	void class_start(VM vm, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
		Tcl_ObjType type = 
		{ 
			(char*)clsName,
			e6_tcl_FreeInternalRepProc,
			e6_tcl_DupInternalRepProc,
			e6_tcl_UpdateStringProc,
			e6_tcl_SetFromAnyProc
		};

		
		types[classID] = type;
		Tcl_RegisterObjType( & types[classID] );

		push_method( vm, clsName, ctor, true );		
	}


	void push_variable( VM vm, const char * vName, int i)
	{
		char buf[100];
		sprintf( buf, "%i", i );
		Tcl_SetVar( vm.interp, (char*)vName, buf, TCL_GLOBAL_ONLY);
	}

	void push_method( VM vm, const char * fuName, SCRIPT_CALLBACK func, bool isStatic )
	{
		Tcl_Command cmd = Tcl_CreateObjCommand( vm.interp, (char*)fuName, func, vm.cdata, 0 );
		Tcl_CmdInfo info = {0};
		Tcl_GetCommandInfo( vm.interp, (char*)fuName, &info );
	}


	void class_end(VM vm)
	{
	}

	void module_end(VM vm)
	{
	}
	
	bool eval_code(VM vm, const char * code )
	{
		return 0;
	}
}	

namespace ScriptTcl
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


	int print(ClientData /*cdata*/, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] )
	{ 
		for ( int i=1; i<objc; i++ )
		{
			l_out->printLog( Tcl_GetString(objv[i]) );
			if ( i < objc-1) l_out->printLog( " " );
		}
		return TCL_OK;
	} 
	int foo(ClientData /*cdata*/, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] )
	{ 
		l_out->printLog( "foo!!\r\n" );
		return TCL_OK;
	} 
	
	int getType(ClientData /*cdata*/, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] )
	{ 
		char * str = ( objv[1]->typePtr ? objv[1]->typePtr->name : "none" );
		Tcl_SetResult( interp, str, 0 );
		return TCL_OK;
	} 
	


	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{
		TCLVM vm;

		CInterpreter()
		{
			$1("CInterpreter()");
			setName( "e6_Tcl" );
			vm.argc = 0;
			vm.argv = 0;
			vm.cdata = 0;
			vm.interp = 0;
			types.clear();
		}
		~CInterpreter()
		{
			$1("~CInterpreter()");
	        Tcl_DeleteInterp(this->vm.interp);
			E_RELEASE( engine );
		}

		virtual bool setup( e6::Engine * ngin ) 
		{
			E_ADDREF( ngin );
			E_RELEASE( engine );
			engine = ngin;

			this->vm.interp = Tcl_CreateInterp();
			assert(this->vm.interp);

			Tcl_CreateObjCommand( vm.interp, "puts", ScriptTcl::print, 0, 0 );
			Tcl_CreateObjCommand( vm.interp, "foo",   ScriptTcl::foo, 0, 0 );
			Tcl_CreateObjCommand( vm.interp, "type",  ScriptTcl::getType, 0, 0 );

			return 	Script::addModules( vm );
		}				


		// load & run
		virtual bool exec( const char *code, const char * marker ) 
		{
			int result = Tcl_EvalEx( this->vm.interp, (char*)code, strlen(code), TCL_EVAL_DIRECT );
			if (result == TCL_ERROR)
			{
				char msg[200 + TCL_INTEGER_SPACE];
				sprintf( msg, "\nError    ( \"%.150s\" line %d)\n", marker, this->vm.interp->errorLine );
				Tcl_AddErrorInfo( this->vm.interp, msg );
				l_err->printLog( msg );
			}
			return (result==TCL_OK);
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
			return "tcl";	
		}

	}; // CInterpreter


} // ScriptSquirrel



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptTcl",		ScriptTcl::CInterpreter::createSingleton, ScriptTcl::CInterpreter::classRef		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptTcl 00.000.0061 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

