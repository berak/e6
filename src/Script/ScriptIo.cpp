#include "../e6/e6_impl.h"
#include "Script.h"

#include <stdarg.h> 
#include <map> 

extern "C"
{
	//#include "IoState.h"
	//#include "IoObject.h"
	#include "IoVM.h"
};

// global:
e6::Engine * engine = 0;

IoState * e6_io_state = 0;

struct IoMachine 
{
	IoObject * self;
	IoObject * locals;
	IoMessage * mess;
	IoObject * ret;

	IoMachine(IoObject * s=0, IoObject * l=0, IoMessage * m=0, IoObject * r=0 )
		: self(s), locals(l), mess(m), ret(r) 
	{}
};

typedef IoMachine * VM;
typedef int RetType;
typedef IoObject * (*SCRIPT_CALLBACK)(IoObject *self, IoObject *locals, IoMessage *m);
const int classMemberOffset = 0;
const int freeFunctionOffset = 0;

//
// our custom C functions:
// IoObject *IoTrivialObject_returnSelf(IoObject *self, IoObject *locals, IoMessage *m);
//
//

	IoObject * dp( IoObject *self, IoObject *locals, IoMessage * mess )
	{
		void * ptr = IoObject_dataPointer( self );
		printf( "ptr :%8x : %8x\n", self, ptr );
  		return IONIL(self); //sq_throwerror(vm, "you cannot create an instance of this!" );
	}


#include "StackMachine.h"
namespace StackMachine
{

	int retValue( int r )
	{
		return r;
	}

	template <class T>
	IoObject * Constructor_prohibited( IoObject *self, IoObject *locals, IoMessage * mess )
	{
		IoState_error_(e6_io_state, mess, "you cannot create an instance of this!" );
  		return IONIL(self); //sq_throwerror(vm, "you cannot create an instance of this!" );
	}

	template <class T>
	IoObject *  Constructor( IoObject *self, IoObject *locals, IoMessage * mess )
	{
		const char * modName = IoMessage_locals_cStringArgAt_(mess, locals, 0);
		const char * interfaceName = IoMessage_locals_cStringArgAt_(mess, locals, 1);

		T * t = (T*)engine->createInterface( modName, interfaceName );

		IoObject * obj = IoObject_rawClonePrimitive(self);
		IoObject_setDataPointer_( obj, t );
		return obj;
	}

	template < class Func, Func ptr > 
	static IoObject * Function ( IoObject *self, IoObject *args, IoObject * mess )
	{
		IoMachine vm;
		vm.self   = self;
		vm.locals = args;
		vm.mess   = mess;
		int r = StackMachine::function( &vm, ptr );
		return vm.ret;
	}


	void * e6_IoObject_free(IoObject *self)
	{
		void * ptr = IoObject_dataPointer( self );
		if ( ptr )
		{
			e6::Base * obj = static_cast<e6::Base *>(ptr);
			E_RELEASE(obj);
		}
		IoObject_setDataPointer_( self, 0 );
		return self;
	}

	void * e6_IoObject_mark(IoObject *self)
	{
		return self;
	}
	void * e6_IoObject_rawClone(IoObject *self)
	{
		IoObject *clone = IoObject_rawClonePrimitive(self);
		return clone;
	}
	//
	// handle float3, float4 as arrays:
	//
	void put_arr( VM vm, const float*f, int n )
	{
		IoList * l = IoList_new( e6_io_state );
		for ( int i=0; i<n; i++ )
		{
			IoList_rawAppend_( l, IoNumber_newWithDouble_(e6_io_state, f[i]) );
		}
		vm->ret = l;
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
		return 0; //sq_throwerror( vm, e );
	}

	
	
	
	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
		IoObject * obj = IoMessage_locals_valueArgAt_(vm->mess, vm->locals, index);
		*pptarget = static_cast<X*>(IoObject_dataPointer( obj ));
//		bool r = SQ_SUCCEEDED( sq_getinstanceup( vm, index, (SQUserPointer*)(pptarget), 0 /*getClassType<X>()*/ ) );
		// fprintf( stderr, "%s<%s>(%i)(%x)\n", __FUNCTION__,typeid(*pptarget).name(), index, *pptarget );
		index ++;
		return 1;
	}

	template < class X >
	X * getThis( VM vm, int &index )
	{
		return static_cast<X*>(IoObject_dataPointer( vm->self ));
	}


	///PPP return a new object or self???
	template < class X >
	int   putObject( VM vm, X * x, int pos )
	{
		if ( x )
		{
			IoStateProtoFunc * ptr = (IoStateProtoFunc*)getClassID<X>();
			IoDate *proto = IoState_protoWithInitFunction_(e6_io_state, ptr );
			vm->ret = IoObject_rawClonePrimitive( proto );
			IoObject_setDataPointer_( vm->ret, x );
		}
		else
		{
			vm->ret = e6_io_state->ioNil;
		}
		return 1;
	}

	float getFloat( VM  vm, int & index )
	{
		float target = IoMessage_locals_floatArgAt_(vm->mess, vm->locals, index);
		// fprintf( stderr, "%s<float>(%i)(%f)\n", __FUNCTION__, index, target );
		//dbg(vm);
		index ++;
		return target;
	}
	int   putFloat( VM  vm, float f )
	{
		// fprintf( stderr, "%s<float>(%f)\n", __FUNCTION__,f );
		vm->ret = IoNumber_newWithDouble_(e6_io_state, f);
		return 1;
	}
	int   getInt( VM  vm,  int & index )
	{
		int target = IoMessage_locals_intArgAt_(vm->mess, vm->locals, index);
		// fprintf( stderr, "%s<int>(%i)(%i)\n", __FUNCTION__, target, index );
		index ++;
		return target;
	}

	int   putInt( VM  vm, int t )
	{
		// fprintf( stderr, "%s<int>(%i)\n", __FUNCTION__, t );
		vm->ret = IoNumber_newWithDouble_(e6_io_state, t);
		return 1;
	}
	const char * getString( VM vm, int & index )
	{
		const char * target = IoMessage_locals_cStringArgAt_(vm->mess, vm->locals, index);
		// fprintf( stderr, "%s<const char*>(%s)(%i)\n", __FUNCTION__, target, index );
		index ++;
		return target;
	}
	int   putString( VM vm, const char * str )
	{		
		vm->ret = IoSeq_newWithCString_(e6_io_state, str);
		return 1;
	}

	void module_start(VM v, const char * modName)
	{
	}

	//IoObject * e6_create_dummy_proto( void * state )
	//{
	//	assert(!"we should never get called!");
	//	return 0;
	//}

	void class_start(VM v, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
		// Tag handling
		IoTag *tag = IoTag_newWithName_(clsName);
		IoTag_state_(tag, e6_io_state);
		IoTag_freeFunc_(tag, (IoTagFreeFunc *)e6_IoObject_free);
		IoTag_markFunc_(tag, (IoTagMarkFunc *)e6_IoObject_mark);
		IoTag_cloneFunc_(tag, (IoTagCloneFunc *)e6_IoObject_rawClone);

		// proto
		IoObject *proto = IoObject_new(e6_io_state);
		IoObject_tag_(proto, tag);
		IoObject_setDataPointer_(proto, 0);
		// classID is used as hashkey
		IoStateProtoFunc * ptr = (IoStateProtoFunc*)classID;
		//printf( "%-20s : %08x\n", clsName, ptr );
		IoState_registerProtoWithFunc_(e6_io_state, proto, ptr );
		
		IoSymbol * slotName = IoState_symbolWithCString_( e6_io_state, clsName );
		IoObject_setSlot_to_(e6_io_state->lobby, slotName, proto );
		
		IoSymbol * ctName = IoState_symbolWithCString_( e6_io_state, "new" );
		IoObject_addMethod_(proto, ctName, ctor);

		IoSymbol * dpName = IoState_symbolWithCString_( e6_io_state, "dp" );
		IoObject_addMethod_(proto, dpName, dp);

		if ( superName )
		{
			IoObject * super = IoState_protoWithName_( e6_io_state, superName );
			IoObject_rawAppendProto_( proto, super );
		}

		v->self = (proto);
	}


	void push_variable(VM v, const char * vName, int i)
	{
	}

	void push_method(VM vm, const char * fuName, SCRIPT_CALLBACK func, bool isStatic)
	{
		IoObject * who = vm->self;
		if ( isStatic )
		{
			who = e6_io_state->lobby;
		}
		IoSymbol * slotName = IoState_symbolWithCString_( e6_io_state, fuName );
		IoObject_addTaglessMethod_(who, slotName, func);
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

namespace ScriptIo
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


	void printfunc(void *machine, const UArray *wbuf)
	{
		static char buf[8064];
		buf[0]=0;
		int i = 0;
		char * b_in = (char*)(wbuf->data);
		char * b_out = (char*)(buf);
		for ( ;  (*b_in) && (i<wbuf->size); i++, b_in++ )
		{
			if (
				( *b_in == '\r' ) ||
				( *b_in == '\n' && *(b_in-1) != '\r' )
				)
			{
				*b_out++ = '\r';
				*b_out++ = '\n';
				*b_out   = 0;
			}
			else
			{
				*b_out++ = *b_in;
			}
		}
		*b_out++ = '\r';
		*b_out++ = '\n';
		*b_out++ = 0;
		l_out->printLog( buf );
	}


	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{
		VM v;

		CInterpreter()
			: v(0)
		{
			$1("CInterpreter()");
			setName( "e6_ScriptIo" );
			v = new IoMachine;
		}
		~CInterpreter()
		{
			$1("~CInterpreter()");
			//IoState_done(e6_io_state);
			IoState_free(e6_io_state);
			delete v;
		}

		virtual bool setup( e6::Engine * ngin ) 
		{
			//E_ADDREF( ngin );
			//E_RELEASE( engine );
			engine = ngin;

			e6_io_state = IoState_new();
			IoState_printCallback_(e6_io_state, printfunc);

			//assert(!"catch me !");
			return 	Script::addModules( v );
		}				


		// load & run
		virtual bool exec( const char *code, const char * marker ) 
		{
			IoObject * o = IoState_doCString_(e6_io_state, code);
			return (o!=0);
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
			return "io";	
		}

	}; // CInterpreter


} // ScriptIo



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptIo",		ScriptIo::CInterpreter::createSingleton, ScriptIo::CInterpreter::classRef		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptIo 00.000.0123 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

