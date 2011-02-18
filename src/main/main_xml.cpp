#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../xml/xml.h"



using e6::uint;


int main(int argc, char **argv)
{
	char * in = argc>1 ? argv[1] : "my.xml";
	e6::Engine * e = e6::CreateEngine();	
	xml::Document * doc = (xml::Document*) e->createInterface( "xml", "xml.Document" );
	bool r = ( doc != 0 );
	if ( r )
	{
		printf( "<!-- start -->\n" );
		doc->push("Foo");
		printf( "<%s ", doc->tagName() );
		doc->pushData( __FILE__ );
		doc->pushAttribute("name","foo00");
		doc->pushAttribute("tid","Fooo");
			doc->push("Foo.bar");
			doc->pushAttribute("tid","Bare");
				doc->push("Foo.bar.carol");
				doc->pushAttribute("tid","Care");
				doc->pop();
			doc->pop();
			doc->push("Foo.baz");
			doc->pushAttribute("tid","Bare");
			doc->pop();
			doc->push("Foo.baloo");
			doc->pushAttribute("tid","Bare");
			doc->pop();
		printf( "/>\n" );
		doc->pop();
		// r = doc->load( in );
		r = doc->save( in );
	}

	E_RELEASE(doc);	
	E_RELEASE(e);	
	return r!=0;
}
