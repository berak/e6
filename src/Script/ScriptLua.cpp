#include "../e6/e6_impl.h"
#include "Script.h"

#include <stdarg.h> 
#include <map> 

extern "C"
{
	#include <lua.h> 
	#include "lualib.h"
	#include "lauxlib.h"
};

// global:
e6::Engine * engine = 0;
std::map<int,const char*> clsMap;
int clsTop = 0;

typedef lua_State * VM;
typedef int RetType;
typedef RetType (*SCRIPT_CALLBACK)( VM vm);
const int classMemberOffset = 1;
const int freeFunctionOffset = 1;

#include "StackMachine.h"

namespace StackMachine
{
	int retValue( int r )
	{
		return r;
	}

	template <class T>
	RetType Constructor( VM vm )
	{
		int i = 1;
		const char * modName = getString(vm,i);
		const char * interfaceName = getString(vm,i);
		T * t = (T*)engine->createInterface( modName, interfaceName );
		return putObject(vm, t, 1);
	}

	template <class T>
	RetType Constructor_prohibited( VM vm )
	{
		error( vm, "you cannot create an instance of this !");
		return 0;
	}

	template < class Func, Func ptr > 
	RetType Function ( VM vm )
	{
		return StackMachine::function( vm, ptr );
	}

	//
	// handle float3 as 3 separate floats:
	//
	template <> 
	RetType put<float3>(VM vm, float3 f)
	{
		putFloat(vm,f[0]);
		putFloat(vm,f[1]);
		putFloat(vm,f[2]);
		return 3;
	}
	template <> 
	RetType put<const float3& >(VM vm, const float3 & f)
	{
		putFloat(vm,f[0]);
		putFloat(vm,f[1]);
		putFloat(vm,f[2]);
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
	// handle float4 as 4 separate floats:
	//
	template <> 
	RetType put<float4>(VM vm, float4 f)
	{
		putFloat(vm,f[0]);
		putFloat(vm,f[1]);
		putFloat(vm,f[2]);
		putFloat(vm,f[3]);
		return 3;
	}
	template <> 
	RetType put<const float4& >(VM vm, const float4 & f)
	{
		putFloat(vm,f[0]);
		putFloat(vm,f[1]);
		putFloat(vm,f[2]);
		putFloat(vm,f[3]);
		return 3;
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
		lua_pushstring(vm,e);
		return lua_error(vm); 
	}

	char * _tostring (lua_State *L, int l) 
	{
	  static char buff[228] = {0};
	  buff[0] = 0;

	  switch (lua_type(L, l)) {
	    case LUA_TNUMBER:
	      sprintf(buff, "number   : %s", lua_tostring(L, l));
	      break;
	    case LUA_TSTRING:
	      sprintf(buff, "string   : %s", lua_tostring(L, l));
	      break;
	    case LUA_TBOOLEAN:
	      sprintf(buff, "bool     : %s", (lua_toboolean(L, l) ? "true" : "false"));
	      break;
	    case LUA_TTABLE:
	      sprintf(buff, "table    : %p", lua_topointer(L, l));
	      break;
	    case LUA_TFUNCTION:
	      sprintf(buff, "function : %p", lua_topointer(L, l));
	      break;
	    case LUA_TUSERDATA:
	      sprintf(buff, "userdata : %p", lua_unboxpointer(L, l));
	      break;
	    case LUA_TLIGHTUSERDATA:
	      sprintf(buff, "luserdata: %p", lua_touserdata(L, l));
	      break;
	    case LUA_TTHREAD:
	      sprintf(buff, "thread   : %p", (void *)lua_tothread(L, l));
	      break;
	    case LUA_TNIL:
	      sprintf(buff, "(nil)" );
	      break;
	    default:
		  sprintf(buff, "(no)" );
	      break;
	  }
	  return buff;
	}
	
	int _dbg(VM vm, const char * xtra="")
	{
		int t = lua_gettop(vm);
		printf( "%s -- stack: %i elems.\r\n",xtra,t );
		for ( int i=1; i<=t; i++ )
		{
			printf( "%i %s\r\n", i, _tostring(vm,i) );
		}
		return 1;
	}



	void * _getuser( lua_State *L, int stackPos=1 )
	{
	    void *ptr=0;
	    int tp = lua_type(L,stackPos); // self should be 1st arg
	    if ( tp == LUA_TTABLE ) {        // self is list
	        lua_rawgeti(L,stackPos,1); // this-ptr is first elem, pushed to top
	        int t = lua_gettop(L); 
	        if ( lua_isuserdata(L,t) ) 
			{
	            ptr = lua_unboxpointer( L, t );
	        }
	        lua_pop(L,1);       // rawgeti did a push
	    }
	    return ptr;             // return 'this'
	}


	bool _pushuser( lua_State* L, void *ctx, const char * clsName ) 
	{
	    lua_newtable(L);

	    lua_pushnumber( L, 1 );     // new object, new table.
		lua_boxpointer( L, ctx );   // box 'this'
		// Get our deleter metatable
		lua_pushstring(L,"e6_objectmeta");
		lua_gettable(L,LUA_REGISTRYINDEX);
		lua_setmetatable(L,-2);
		// set the pointer
		lua_rawset(L, -3);
		
		// now set the object-metatable :
		lua_pushstring( L, clsName );
		lua_gettable( L, LUA_REGISTRYINDEX );
		lua_setmetatable( L, -2 );
		lua_settop(L, 1 );
		return 1;
	}



	int _newmetatable (lua_State *L, const char *tname) 
	{
		luaL_newmetatable(L, tname); 
		int metatable = lua_gettop(L); 

		lua_pushstring(L, tname);
		lua_pushvalue(L, -2);
		lua_rawset(L, LUA_REGISTRYINDEX);  // registry.name = metatable 

		lua_pushliteral(L, "__index");
		lua_pushvalue(L, -2);  // push metatable 
		lua_rawset(L, -3);     // metatable.__index = metatable 

		return 1;
	}



	template < class X >
	bool getObject( VM vm, X**pptarget, int &index )
	{
		*pptarget = static_cast< X* >( _getuser( vm, index ) );
		index ++;
		return ((*pptarget)!=0);
	}
	template < class X >
	X * getThis( VM vm, int &index )
	{
		X* target = static_cast< X* >( _getuser( vm, index ) );
		index ++;
		return target;
	}

	template < class X >
	int   putObject( VM vm, X * x, int pos )
	{
		lua_settop(vm,0); // remove all
		if ( ! x )
		{
			lua_pushnil(vm);
			return 1;
		}
		const char * clsName = clsMap[ getClassID<X>() ];
		_pushuser(vm,x,clsName); 
		return 1;
	}

	float getFloat( VM  vm, int & index )
	{
		float i= (float)lua_tonumber(vm,index);
		index ++;
		return i;
	}
	int   putFloat( VM  vm, float f )
	{
	    lua_pushnumber( vm, f );
		return 1;
	}
	int   getInt( VM  vm,  int & index )
	{
		int target=(int)lua_tonumber( vm, index );
		index ++;
		return target;
	}

	int   putInt( VM  vm, int t )
	{
	    lua_pushnumber( vm, (double)t );
		return 1;
	}
	const char * getString( VM vm, int & index )
	{
		const char * target= lua_tostring(vm,index);
		index ++;
		return target;
	}
	int   putString( VM vm, const char * str )
	{
	    lua_pushstring( vm, str );
		return 1;
	}

	void module_start(lua_State * L, const char * modName)
	{
	}

	void class_start(lua_State * L, const char * clsName, const char * superName, int classID, SCRIPT_CALLBACK ctor)
	{
		clsTop = lua_gettop(L);
		clsMap [ classID ] = clsName;

		push_method( L, clsName, ctor, 1 );

		_newmetatable( L, clsName );
	
		if ( superName ) // copy all inherited members to 'this' metatable
		{
			int c = lua_gettop(L);
			lua_pushstring( L, superName );
			lua_gettable( L, LUA_REGISTRYINDEX );

		    lua_pushnil(L);  // first key 
			while (lua_next(L, -2) != 0) 
			{
				// don't copy __index & __gc, etc:
				if ( strstr( lua_tostring(L,-2), "__" ) )
				{
			        lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration 
					continue;
				}
				lua_pushvalue( L,  c ); // the table
				lua_pushvalue( L, -3 ); // the key
				lua_pushvalue( L, -3 ); // the value
				lua_settable( L, -3 );  // set table
		        lua_pop(L, 1);  // removes table
		        lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration 
		    }
			lua_settop(L,c);
		}
	}


	void push_variable(lua_State * L, const char * vName, int i)
	{
	    lua_pushstring( L, vName );
	    lua_pushboolean( L, i );
	    lua_rawset(L, -3);
	}

	void push_method(lua_State * L, const char * fuName, SCRIPT_CALLBACK func, bool isStatic)
	{
	    lua_pushstring( L, fuName );
	    lua_pushcclosure( L, func, 0 );
		int index = isStatic ? LUA_GLOBALSINDEX : -3; 
	    lua_rawset(L, index);
	}


	void class_end(lua_State * L)
	{
		lua_settop( L, clsTop );
		clsTop=0;
	}

	void module_end(lua_State * L)
	{
	}
		
}	

namespace ScriptLua
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

	int _print( lua_State *L )
	{
		int n = 1 + lua_gettop(L);
		for ( int i=1; i<n; i++ ) 
		{
			const char *s = lua_tostring(L,i);
			if ( s )
			{
				l_out->printLog( s );
			}
		}
		return 0;
	}
	template< int pos >
	int _getTable( lua_State *L ) 
	{
		const char *s = lua_tostring(L,1);
		if ( s ) {
			lua_pushstring( L, s );
			lua_gettable( L, pos);
		} else {
			lua_pushnil(L); 
		}
		return 1;
	}

	template< int level >
	int _listvars (lua_State *L) 
	{
	    lua_pushnil(L);  // first key 
		while (lua_next(L, level) != 0) 
		{
	        // `key' is at index -2 and `value' at index -1 
			char buf[200];		
			sprintf( buf, "%-20s %-20s\r\n", lua_tostring(L,-2),StackMachine::_tostring(L,-1) );		
			l_out->printLog( buf );
	        lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration 
	    }
		return 0;
	}

	int _foreach (lua_State *L) 
	{
		if ( lua_type(L, 1) != LUA_TTABLE     ) return 0;
		if ( lua_type(L, 2) != LUA_TFUNCTION  ) return 0;
		lua_pushnil(L);            // first key 
		for (;;) 
		{
			if (lua_next(L, 1) == 0)
				return 0;
			lua_pushvalue(L, 2);   // function 
			lua_pushvalue(L, -3);  // key 
			lua_pushvalue(L, -3);  // value 
			lua_call(L, 2, 1);
			if (!lua_isnil(L, -1))
				return 1;
			lua_pop(L, 2);         // remove value and result 
		}
		return 0;
	}
	int _typename(lua_State *L) 
	{
		int t = lua_type( L, 1 );
		lua_pushstring( L, lua_typename(L,t) );
		return 1;
	}

	int collectGarbage( lua_State * L )
	{
        int t = lua_gettop(L); 
        if ( ! lua_isuserdata(L,t) ) 
		{
			l_err->printLog( "error : no userdata in " __FUNCTION__ "\r\n" );
			return 0;
        }
        e6::Base *ptr = static_cast<e6::Base*>( lua_unboxpointer( L, t ) );
		if ( ptr )
		{
			ptr->release();
			return 1;
		}
		return 0;
	}

	struct CInterpreter
		: e6::CName< Script::Interpreter , CInterpreter > 
	{
		lua_State * _l;

		CInterpreter()
			: _l(0)
		{
			$1("CInterpreter()");
			setName( "ScriptLua" );
		}
		~CInterpreter()		
		{
			lua_close(_l);
			E_RELEASE( engine );
			$1("~CInterpreter()");
		}

		virtual bool setup( e6::Engine * ngin ) 
		{
			E_ADDREF( ngin );
			E_RELEASE( engine );
			engine = ngin;

			_l = lua_open();
			
			luaopen_base(_l);
			//luaopen_table(_l);
			//luaopen_io(_l);
			//lua_strlibopen(_l);
			//lua_mathlibopen(_l);
			//lua_dblibopen(_l);

			lua_register( _l, "print", _print ); // just overwrite global print
			lua_register( _l, "foreach", _foreach ); 
			lua_register( _l, "typename", _typename ); 
			lua_register( _l, "listglobals", _listvars<LUA_GLOBALSINDEX> ); 
			lua_register( _l, "listregistry", _listvars<LUA_REGISTRYINDEX> ); 
			lua_register( _l, "getglobal", _getTable<LUA_GLOBALSINDEX> ); 
			lua_register( _l, "getregistry", _getTable<LUA_REGISTRYINDEX> ); 



			// now set up a special metatable for our userpointers, so they can gc:
			int top = lua_gettop(_l);
			lua_pushstring(_l,"e6_objectmeta");		// Name of the metatable in the registry
			lua_newtable(_l);				//  Create the metatable
			lua_pushstring(_l,"__gc");			//  Name of the garbage collection function
			lua_pushcclosure(_l, collectGarbage, 0);	//  The action part of the garbage collector
			lua_settable(_l,-3);				//  Set the __gc entry to the garbage collector
			lua_settable(_l,LUA_REGISTRYINDEX);  		// Set the metatable to pfMessageMetatable in the registry
			lua_settop(_l, top );

			return Script::addModules( _l );
		}				

		bool call(int retval=0) 
		{
		    int res = lua_pcall(_l, 0, LUA_MULTRET, 0);  /* call main */
			if ( res ) 
			{
				l_err->printLog( lua_tostring(_l, -1) );
				l_err->printLog(  "\r\n" );
				lua_pop(_l, 1);
				return 0;
			}
			return true;
		}	

		// load & run
		virtual bool exec( const char *code, const char *marker ) 
		{
		    struct Handler 
		    {
		        const char *s;
		        size_t      size;
		  
		        Handler( const char *s, size_t z) : s(s),size(z) {}           
		      
		        static const char *get (lua_State *, void *ud, size_t *siz) 
				{
		            Handler *x = (Handler *)ud;
		            if (x->size == 0) return NULL;
		            *siz = x->size;
		            x->size = 0;
		            return x->s;
		        }
		    } xx( code, strlen(code) );

		    int res = lua_load(_l, Handler::get, &xx, marker);
			if ( res ) 
			{
				l_err->printLog( lua_tostring(_l, -1) );
				l_err->printLog( "\r\n" );
				lua_pop(_l, 1);
			    return 0;
			}
			return call(0);
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
			return "lua";
		}
	};
};



extern "C"
uint getClassInfo( e6::ClassInfo ** ptr )
{
	const static e6::ClassInfo _ci[] =
	{
		{	"Script.Interpreter",	"ScriptLua",		ScriptLua::CInterpreter::createSingleton, ScriptLua::CInterpreter::classRef		},
		{	0, 0, 0	}
	};

	*ptr = (e6::ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("ScriptLua 00.000.0010 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}

