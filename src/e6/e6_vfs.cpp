
#include <map>
#include <string>
#include <iostream>
#include "e6_vfs.h"
namespace e6
{

	class VfsMap 
		: e6::CName< Vfs , VfsMap > 
	{

	    multimap<string,Name*> mmap;
	    typedef multimap<string,Name*>::iterator MI;

		struct Get : VfsIterCallback
		{
			const char * _name;
			Name * _obj ;

			Get( const char * name )
				: _name(name)
				, _obj(0)
			{
			}
			virtual bool process( const char * path, Name * object )
			{
				if ( ! strcmp( object->getName(), _name ) )
				{
					_obj = object;
					return 1;
				}
			}
			const Name * get() { return _obj; }
		} ;
	public:
	    
	    bool process( const char* key, VfsIterCallback & cb ) 
		{
	        pair<MI,MI> p = mmap.equal_range(key);
			if ( p == mmap.end() ) 
			{
				// key not found
				return 0;
			}
	        for ( MI it = p.first; it != p.second; ++it ) 
			{
	            Name *b = it->second;
				if ( cb.process( key, b ) )
					return true;
	        }
			return false;
	    }
		const Name * get( const char * path, const char * name )
		{
			Get g(name);
			if ( process(path,g) )
				return g.get();
			return 0;
		}

	    bool addEntry( const char* key, Name *name ) 
		{
			mmap[ key ] = name;
			return 1;
	    }

	    void erase( Name *n ) 
		{
	        MI vic, it = mmap.begin();
	        while( it != mmap.end() ) 
			{
	            Name *c = it->second;
	            vic = it++;
	            if ( n == c  ) 
				{
					c->release();
	                mmap.erase(vic);
				}
	        }
	    }

	    void print() 
		{
	        cout << "----- " << mmap.size() << " elems -----" << endl;
	        for ( MI it = mmap.begin(); it != mmap.end(); ++it ) 
			{
	            Base *e = it->second;
	            cout  << it->first << " " << e->getName() << endl;
	        }
	        cout << endl;
	    }
	};

} //namespace e6
