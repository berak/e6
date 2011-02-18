#include "../e6/e6_impl.h"
#include "Script.h"

#include <stdarg.h> 
#include <map> 
#include <vector> 

//extern "C"
//{
	#include <Python.h> 
//};

// global:
e6::Engine * engine = 0;

struct PythonVM
{
	PyObject * self;
	PyObject * args;
	PyObject * ret;
	PythonVM() : self(0), args(0), ret(0) {}
};


typedef PythonVM & VM;


typedef int RetType;
typedef PyObject * (*SCRIPT_CALLBACK) ( PyObject *self, PyObject *args );
const int classMemberOffset  = 1; // self
const int freeFunctionOffset = 1; // args


struct  py_Object 
{
    PyObject_HEAD
    e6::Base * base;
};

struct py_Class
{
	char name[64];
	PyTypeObject * type;
	PyMethodDef * members;	
	SCRIPT_CALLBACK   ctor;

	py_Class( const char * n, SCRIPT_CALLBACK c=0, PyMethodDef * m=0 ) 
		: type(0), members(m), ctor(c) 
	{
		name[0]=0;
		if (n) strcpy(name,n);
	}
};


PyMethodDef _g_members[512];	
int _n_members = 0;	
PyMethodDef _g_globals[512]; 	
int _n_globals = 0;	
PyTypeObject _g_types[128];	
int _n_types = 0;	

std::map< int, py_Class * > clsMap;
PyObject * _module = 0;


#include "StackMachine.h"
namespace StackMachine
{
	int retValue( int r )
	{
		return r;
	}

	template <class T>
	PyObject *  Constructor_prohibited( PyObject *self, PyObject *args )
	{
		assert( !"you cannot create an instance of this!" );
		return 0; //error(vm, "you cannot create an instance of this!" );
	}

	template <class T>
	PyObject *  Constructor( PyObject *self, PyObject *args )
	{
		int i=1;
		PythonVM vm;
		vm.self = self;
		vm.args = args;
		const char * modName = getString(vm,i);
		const char * interfaceName = getString(vm,i);
		T * t = (T*)engine->createInterface( modName, interfaceName );
		int r = putObject(vm, t, 1);
		if ( ! r ) 
		{
			Py_INCREF(Py_None);
			return Py_None;
		}
		return vm.ret;
	}

	template < class Func, Func ptr > 
	static PyObject * Function ( PyObject *self, PyObject *args )
	{
		PythonVM vm;
		vm.self = self;
		vm.args = args;
		int r = StackMachine::function( vm, ptr );
		if ( ! r ) 
		{
			Py_INCREF(Py_None);
			return Py_None;
		}
		return vm.ret;
	}

	//
	// handle float3 as 3 separate floats for get, as a tuple for put:
	//
	template <> 
	RetType put<float3>(VM vm, float3 f)
	{
		vm.ret = Py_BuildValue( "fff", f[0],f[1],f[2] );
		return 3;
	}
	template <> 
	RetType put<const float3& >(VM vm, const float3 & f)
	{
		vm.ret = Py_BuildValue( "fff", f[0],f[1],f[2] );
		return 3;
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
	// handle float4 as 4 separate floats for get, as a tuple for put:
	//
	template <> 
	RetType put<float4>(VM vm, float4 f)
	{
		vm.ret = Py_BuildValue( "ffff", f[0],f[1],f[2],f[3] );
		return 1;
	}
	template <> 
	RetType put<const float4& >(VM vm, const float4 & f)
	{
		vm.ret = Py_BuildValue( "ffff", f[0],f[1],f[2],f[3] );
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
	//
	// release an object :
	//
	void _dealloc(PyObject *self)
	{
		py_Object * ppy = ((py_Object*)self);
		if ( ppy )
		{
			E_RELEASE( ppy->base );
			PyMem_DEL(ppy);
		}
	}

	int _is_gc(PyObject *self)
	{
		py_Object * ppy = ((py_Object*)self);
		if ( ppy )
		{
			return 1;
		}
		return 0;
	}


	//
	// create a typeobject :
	//
	PyTypeObject  new_TypeObject(const char *name, PyMethodDef * _methods, const char *docstring, SCRIPT_CALLBACK ctor, PyTypeObject * base = 0 ) 
	{
	 
		PyTypeObject my_type = 
		{
			PyObject_HEAD_INIT(&PyType_Type)
			0,			               //ob_size
			(char*)name,	           //tp_name
			sizeof(py_Object),	       //tp_basicsize
			0,			               //tp_itemsize
			(destructor)_dealloc,	   //tp_dealloc
		    0,                         //tp_print
		    0,                         //tp_getattr
		    0,                         //tp_setattr
		    0,                         //tp_compare
		    0,                         //tp_repr
		    0,                         //tp_as_number
		    0,                         //tp_as_sequence
		    0,                         //tp_as_mapping
		    0,                         //tp_hash 
		    0,                         //tp_call
		    0,                         //tp_str
		    0,						   //tp_getattro
		    0,                         //tp_setattro
		    0,                         //tp_as_buffer
			Py_TPFLAGS_DEFAULT  //tp_flags
			 | Py_TPFLAGS_BASETYPE  //tp_flags
			,// | Py_TPFLAGS_HAVE_GC, //tp_flags
		    (char*)docstring,          // tp_doc 
		    0,						   // tp_traverse 
		    0,						   // tp_clear 
		    0,		                   // tp_richcompare 
		    0,		                   // tp_weaklistoffset 
		    0,						   // tp_iter 
		    0,		                   // tp_iternext 
		    _methods,                  // tp_methods 
		    0,                         // tp_members 
		    0,                         // tp_getset 
		    base,                      // tp_base 
		    0,                         // tp_dict 
		    0,                         // tp_descr_get 
		    0,                         // tp_descr_set 
		    0,                         // tp_dictoffset 
		    0,// (initproc)ctor,       // tp_init 
		    0,                         // tp_alloc 
		    (newfunc)ctor,              // tp_new 
			(freefunc)_dealloc,         // tp_free; /* Low-level free-memory routine */
			0 //(inquiry) _is_gc /* For PyObject_IS_GC */

		};
		return my_type;
	}

	RetType  error(VM vm, const char *e )
	{
		return 0; // sq_throwerror( vm, e );
	}


	template < class X >
	X * getThis( VM vm, int &index )
	{
		PyObject * po = vm.self;
		if ( po )
		{
			return static_cast<X*>(((py_Object*)(po))->base);
		}
		return 0;
	}

	PyObject * _getObject( VM vm, int &index )
	{
		if ( index < 1 ) printf("ERROR : " __FUNCTION__ "(%i)\n", index );
		PyObject * po =  PyTuple_GET_ITEM( vm.args, index-1 );
		return po;
	}

	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
		*pptarget = 0;
		PyObject * po = _getObject( vm, index );
		if ( po )
		{
			*pptarget = static_cast<X*>(((py_Object*)(po))->base);
		}
		index ++;
		return (po!=0);
	}


	template < class X >
	RetType putObject( VM vm, X * x, int pos )
	{
		if ( x )
		{
			int clsID = getClassID<X>();
			py_Class * cls = clsMap[ clsID ];
			py_Object * rv = PyObject_NEW( py_Object, cls->type );
			if ( rv  ) 
			{
				rv->base = x;
///PPP				Py_INCREF(rv);
				vm.ret = (PyObject*)rv;
				return 1;
			}
		}
		return 0;
	}

	float getFloat( VM  vm, int & index )
	{
		float f = 0;
		PyObject * a = _getObject( vm, index );
		if ( a )
		{
			f = (float)PyFloat_AsDouble(a);
		}
		index ++;
		return f;
	}

	RetType putFloat( VM  vm, float target )
	{
		PyObject * a = PyFloat_FromDouble( target );
		Py_XINCREF(a);
		vm.ret = a;
		return 1;
	}

	int   getInt( VM  vm,  int & index )
	{
		int target=0;
		PyObject * a = _getObject( vm, index );
		if ( a )
		{
			target = PyInt_AsLong(a);
		}
		index ++;
		return target;
	}
	RetType  putInt( VM  vm, int target )
	{
		PyObject * a = PyInt_FromLong( target );
		Py_XINCREF(a);
		vm.ret = a;
		return 1;
	}


	const char * getString( VM vm, int & index )
	{
		const char * target=0;
		PyObject * a = _getObject( vm, index );
		if ( a )
		{
			target = PyString_AsString(a);
		}
		index ++;
		return target;
	}

	RetType    putString( VM vm, const char * target )
	{
		PyObject * a = PyString_FromString( target );
		Py_XINCREF(a);
		vm.ret = a;
		return 1;
	}


	void module_start(VM v, const char * modName)
	{
	}

	void class_start(VM v, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
		PyTypeObject * base = 0;
		if ( superName )
		{
			// try to find base-typeobject in classmap:
			std::map<int,py_Class*>::iterator it=clsMap.begin();
			for ( ; it != clsMap.end(); ++it )
			{
				if ( ! strcmp( it->second->name, superName ) )
				{
					base = it->second->type;
					break;
				}
			}
		}

		PyMethodDef * mem = &(_g_members[ _n_members ]);
		py_Class * cls = new py_Class( clsName, ctor, mem );

		PyTypeObject * tp = &(_g_types[ _n_types++ ]);
		(*tp) = new_TypeObject( clsName, mem, clsName, ctor, base );
		cls->type = tp;

		clsMap[ classID ] = cls;
	}


	void push_variable(VM v, const char * vName, int i)
	{
	}

	void push_method(VM v, const char * fuName, SCRIPT_CALLBACK func, bool isStatic)
	{
		PyMethodDef pm = { (char*)fuName, func, METH_VARARGS, (char*)fuName};
		if ( isStatic )
			_g_globals[ _n_globals++ ] = pm;
		else
			_g_members[ _n_members++ ] = pm;

	}


	void nuke_last(PyMethodDef * tail)
	{
		PyMethodDef t0 = {0};
		*tail = t0;
	}
	void class_end(VM v)
	{
		nuke_last (&_g_members[ _n_members++ ]);
	}

	void module_end(VM v)
	{
	}
	
}	

namespace ScriptPython
{
	
	struct DefLogger : e6::Logger 
	{
		virtual bool printLog( const char * s )  
		{
			printf( s );
			return 1;
		}
	} _defLog;

	e6::Logger * l_err = &_defLog;
	e6::Logger * l_out = &_defLog;

	// print to e6.Logger:
	static PyObject * print ( PyObject *self, PyObject *args )
	{
		PyObject * a = PyTuple_GET_ITEM( args, 0 );
		if ( a )
		{
			char *s = PyString_AsString(a);
			l_out->printLog(s);
		}
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	
	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{
		PythonVM vm;

		CInterpreter()
		{
			_n_members = 0;	
			_n_globals = 0;	
			_n_types = 0;	
			_module = 0;
			clsMap.clear();
			setName( "ScriptPython" );
		}
		~CInterpreter()
		{
	        Py_Finalize();
			E_RELEASE( engine );
		}

		virtual bool setup( e6::Engine * ngin ) 
		{
			E_ADDREF( ngin );
			E_RELEASE( engine );
			engine = ngin;

		    Py_Initialize();
			clsMap[ 0 ] = new py_Class( "e6" );

			// vm is empty.
			// we're collecting all the classses here
			int res = Script::addModules( vm );

			// finally, add all global funcs:
			StackMachine::push_method( vm, "xprint", print, true );
			StackMachine::nuke_last (&_g_globals[ _n_globals++ ]);

			// start module:
			PyImport_AddModule("e6");
			_module = Py_InitModule( "e6", _g_globals );

			assert( _module );
			// add all classes:
			std::map<int,py_Class*>::iterator it=clsMap.begin();
			for ( ; it != clsMap.end(); ++it )
			{
				if ( ! it->first ) continue; // spare "e6"
				const char * name = it->second->name;
				PyTypeObject *tp  = it->second->type;
			    if (PyType_Ready(tp) < 0) 
				{
					printf( "Could not setup class %s !\n", name );
			        return 0;
				}
				PyModule_AddObject( _module, (char*)name, (PyObject *)tp );
			    Py_INCREF(tp);
			}
			res = (_module != 0);
			if ( res )
			{
				//  redirect stdout/stderr to e6.Logger:
				this->exec( "import sys\n"
							"import e6\n"
							"class X :\n"
							"    def write( self, s ) :\n"
							"        e6.xprint( s )\n"
							"sys.stdout = X()\n"
							"sys.stderr = X()\n",
							this->getName() );
			}
			return res;
		}				


		// load & run
		virtual bool exec( const char *code, const char * marker ) 
		{
			if ( !code ) return 0;
			if ( !code[0] ) return 0;
		   	int err = ( PyRun_SimpleString( code ) == -1 );
			if ( err )
			{
				char b[100];
				b[0]=0;
				sprintf( b, "[%s] Compile error(%i)!\r\n", marker, err );
				l_err->printLog( b );
			}
			return (!err);
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
			return "py";
		}
	};
};



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptPython",		ScriptPython::CInterpreter::createSingleton, ScriptPython::CInterpreter::classRef		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptPython 00.000.0001 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

