
#include "../e6/e6_impl.h"
#include "Vfs.h"

#include <map>



using e6::uint;
using e6::ClassInfo;


namespace Vfs
{
	//struct CVfs
	//	: public e6::CName< Vfs::Node, CVfs >
	//{
	//	typedef std::map< const char * , e6::Name *, e6::StrLess > VfsMap;
	//	VfsMap vfs;

	//	virtual e6::Name  * getObject()
	//	{
	//		return 0;
	//	}
	//	virtual Vfs::Node * getChild(const char * name)
	//	{
	//		return 0;
	//	}
	//};

	//template < class  Key, class Interface, class Val >
	//struct CVfsSystemEntry
	//	: public e6::CName< Interface, CVfsSystemEntry >
	//{
	//	typedef std::map< const char * , Val, e6::StrLess > VfsMap;
	//	VfsMap vfs;
	//};

	struct Tuple	
	{	
		template < class A1 >
		void operator () (A1 a1)
		{
			//std::cout << __FUNCTION__  << "< class A1 >: " << a1 << "\n";
		}
		template < class A1, class A2 >
		void operator () (A1 a1, A2 a2)
		{
			//std::cout << __FUNCTION__  << "< class A1, class A2 >: " << a1 << ", " << a2 << "\n";
		}
		template < class A1, class A2, class A3 >
		void operator () (A1 a1, A2 a2, A3 a3)
		{
			//std::cout << __FUNCTION__  << "< class A1, class A2, class A3 >: " << a1 << ", " << a2 << ", " << a3 << "\n";
		}

		//~ template <>
		void operator () (void)
		{
			//std::cout << __FUNCTION__  << "()\n";
		}
		template <>
		void operator () (Tuple * t)
		{
			//std::cout << __FUNCTION__  "(Tuple * t)\n";
			t->operator()();
		}
	};
	

	template < class Val, class  Key=const char*, class Cmp=e6::StrLess >
	struct x_map
		: public std::map< Key , Val, Cmp >
	{
	};

	struct CVfsSystem
		: public e6::CName< Vfs::System, CVfsSystem >
	{
		typedef std::map< const char * , e6::Name *, e6::StrLess > VfsMap;
		VfsMap vfs;
		VfsMap::iterator cur;

		virtual e6::Name  * getObject( const char * name ) 
		{
			VfsMap::iterator it = vfs.find( name );
			if ( it == vfs.end() )
			{
				return 0;
			}
			e6::Name  * obj = it->second;
			obj->addref();
			return obj;
		}

		virtual e6::Name  * nextObject( const char * name ) 
		{
			VfsMap::iterator it = vfs.find(name);
			if ( it == vfs.end() )
			{
				it = vfs.begin();
				return 0;
			}
			it ++;
			if ( it == vfs.end() )
			{
				it = vfs.begin();
				return 0;
			}

			e6::Name  * obj = it->second;
			obj->addref();
			it ++;
			return obj;
		}

		virtual uint addObject( e6::Name *child, const char * name, const char * InterFaceName  ) 
		{
			vfs[ name ] = child;
			child->addref();
			return 1;
		}

		virtual uint remObject( const char * name ) 
		{
			VfsMap::iterator it = vfs.find( name );
			if ( it == vfs.end() )
			{
				return 0;
			}
			e6::Name  * obj = it->second;
			vfs.erase( it );
			E_RELEASE(obj);
			return 1;
		}
	};

};

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Vfs.System",	 "Vfs",	Vfs::CVfsSystem::createSingleton, Vfs::CVfsSystem::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Vfs 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
