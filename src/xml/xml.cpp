//#define _DEBUG

#include "../e6/e6_impl.h"
#include "xml.h"
#include "xmlParser.h"



using e6::uint;
using e6::ClassInfo;


namespace xml
{

	//struct CNode
	//	: public e6::CName< Node, CNode >
	//{
	//	XMLNode node;

	//	CNode( XmlNode & n )
	//	{
	//		node(n);
	//	}

	//	~CNode()
	//	{
	//	}

	//	virtual const char * name() const
	//	{
	//		if ( ! node.isEmpty() )
	//		{
	//			return node.getName();
	//		}
	//		return 0;
	//	}
	//	virtual const char * data() const 
	//	{
	//		if ( ! node.isEmpty() )
	//		{
	//			return node.getText();
	//		}
	//		return 0;
	//	}
	//	virtual const char * type() const 
	//	{
	//		if ( node.isEmpty() )
	//		{
	//			return "Empty";
	//		}
	//		return "Node";
	//	}
	//};

	struct Tokenizer
	{
		const char * start;
		char * pos, split, res[255];

		Tokenizer( const char * text, char splitter )
			: start(text)
			, pos((char*)text)
			, split(splitter)
		{
			res[0]=0;
		}

		const char * next()
		{
			int l = 0;
			char * p = pos;
			while ( *p )
			{
				if ( *p == split )
				{
					pos = p+1;
					break;
				}
				p++;
				l++;
			}
			if ( l )
			{
				strncpy( res, pos, l );
				res[l] = 0;
				return res;
			}
			return 0;
		}
	};

	struct CDocument
		: public e6::CName< Document, CDocument >
	{
		XMLNode root, node;

		CDocument()
		{
		}

		~CDocument()
		{
			root.deleteNodeContent();
		}

		virtual const char * tagName() const
		{
			//if ( ! node.isEmpty() )
			{
				return node.getName();
			}
			return 0;
		}
		virtual const char * tagData() const 
		{
			if ( ! node.isEmpty() )
			{
				return node.getText();
			}
			return 0;
		}
		virtual const char * tagAttribute( int i ) const 
		{
			if ( i>=node.nAttribute() ) return 0;
			XMLAttribute attr = node.getAttribute( i );
			return attr.lpszValue;
		}
		virtual const char * tagAttributeName( int i ) const 
		{
			if ( i>=node.nAttribute() ) return 0;
			XMLAttribute attr = node.getAttribute( i );
			return attr.lpszName;
		}


		virtual bool clear() 
		{
			if ( root.isEmpty() )
				return 0;

			root.deleteNodeContent();
			return 1; 
		}
		virtual bool push( const char * name  ) 
		{
			if ( root.isEmpty() )
			{
				node = root = root.createXMLTopNode(name);
				return 1;
			}
			XMLNode child = node.addChild( name );
			if ( ! child.isEmpty() )
			{
				node = child;
				return 1;
			}
			return 0;
		}
		virtual bool pushData( const char * data ) 
		{
			const char * d = node.addText( data );
			return ( d != 0 );
		}
		virtual bool pushAttribute( const char * name, const char * value ) 
		{
			XMLAttribute * attr = node.addAttribute( name, value );
			return ( attr != 0 );
		}

		virtual bool pop() 
		{
			XMLNode prnt = node.getParentNode();
			if ( ! prnt.isEmpty() )
			{
				node = prnt;
				return 1;
			}
			return 0;
		}

		//
		// collada.library_geometries.geometry.mesh.triangles
		//
		virtual bool find( const char *name, int index=0 ) 
		{
			XMLNode tmp = node.getChildNode( name, index );
			if ( ! tmp.isEmpty() )
			{
				node = tmp;
				return 1;
			}
			//node = root;
			return 0;
		}

		virtual bool child( int i ) 
		{
			XMLNode tmp = node.getChildNode(i);
			if ( ! tmp.isEmpty() )
			{
				node = tmp;
				return 1;
			}
			return 0;
		}

		virtual bool load(const char * fName) 
		{
			XMLNode tmp = XMLNode::openFileHelper(fName,0);
			if ( ! tmp.isEmpty() )
			{
				node = tmp;
				if ( root.isEmpty() )
				{
					root = node;
				}
				return 1;
			}
			return 0;
		}

		virtual bool save(const char * fName) const 
		{
			XMLError err = root.writeToFile( fName );
			return (err == 0); // NO err
		}
	};

};

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		//{	"xml.Node",		 "xml",	xml::CNode::createInterface,		xml::CNode::classRef		},
		{	"xml.Document",	 "xml",	xml::CDocument::createInterface, xml::CDocument::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 2; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("xml 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
