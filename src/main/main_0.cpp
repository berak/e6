#include "e6_sys.h"
#include "e6_impl.h"
#include "e6_enums.h"
//#include "../dl_0/dl_0.h"
#include "../Core/Core.h"
#include "../TimeLine/TimeLine.h"



void printNode( Core::Node * node, int indent )
{
	while ( -- indent > 0)
	{
		printf( "\t" );
	}
	uint r = node->addref();
	r = node->release();
	printf( "[%s][%d](%i)\t", node->getName(), r, node->numChildren() );
	printf( "[%3.3f]\t", node->getSphere() );
	e6::float4x4 m;
	node->getWorldMatrix(m);
	print( m.getPos() );
}
void printRec( Core::Node * node, int indent=0 )
{
	printNode(node, indent);
	++indent;

	for ( int i=0; 1; ++i )
	{
		Core::Node * n = node->getChild(i);
		if ( !n )
			break;
		printRec( n, indent );
		n->release();
	}
}

/*
void dl0_test( e6::Engine * e )
{
	dl_0::Foo * f = (dl_0::Foo *)e->createInterface( "dl_0", "dl_0.Foo" );	
	f->foo0();
	f->setName("foo0");

	dl_0::Obj * o = (dl_0::Obj *)e->createInterface( "dl_0", "dl_0.Obj"  );	
	o->f0();

	dl_0::Obj * o2 = (dl_0::Obj *)e->createInterface(  "dl_0", "dl_0.Obj" );	
	o2->f0();
	E_RELEASE(f);	
	E_RELEASE(o);	
	E_RELEASE(o2);	

}
*/

void tl_test( e6::Engine * e )
{
	struct XNoize : TimeLine::ValueUpdate
	{
		uint update( TimeLine::Value * v, float t )
		{
			float r = 1.23456 * t;
			v->setOutput(0, r);
			return 1;
		}
	} nzu;


	TimeLine::TimeLine * tl = (TimeLine::TimeLine *) e->createInterface( "TimeLine", "TimeLine.TimeLine" );	

	TimeLine::Sinus  * tsin = (TimeLine::Sinus *)e->createInterface( "TimeLine", "TimeLine.Sinus"  );	
	TimeLine::Scaler * tsc = (TimeLine::Scaler *)e->createInterface( "TimeLine", "TimeLine.Scaler"  );	
	TimeLine::Value  * tv_in = (TimeLine::Value *)e->createInterface( "TimeLine", "TimeLine.Value"  );	
	TimeLine::Value  * tv_res = (TimeLine::Value *)e->createInterface( "TimeLine", "TimeLine.Value"  );	
	TimeLine::Value  * tv_nz = (TimeLine::Value *)e->createInterface( "TimeLine", "TimeLine.Value"  );	

	printf( "## Created elements.\n" );
	tsin->setName( "Sinus" );
	tsc->setName( "Scaler" );
	tv_res->setNumInputs( 1 );
	tv_res->setName( "Result" );

	tv_in->setNumOutputs( 2 );
	tv_in->setName( "Input" );

	tv_nz->setNumOutputs( 1 );
	tv_nz->setName( "Noize" );
//	XNoize * nzu = new XNoize;
	tv_nz->setValueUpdate( &nzu );
	printf( "## Setup elements.\n" );

	tl->add( tsin );
	tl->add( tsc );
	tl->add( tv_in );
	tl->add( tv_res );
	tl->add( tv_nz );
	printf( "## Added elements.\n" );

	tl->connect( "Result", "i0", "Scaler", "result" );
	tl->connect( "Scaler","input", "Sinus", "sin" );
	tl->connect( "Scaler","scale",  "Input", "o0" );
	tl->connect( "Scaler","offset", "Noize", "o0" );
	tl->connect( "Sinus","omega", "Input", "o1" );
	printf( "## Connected elements.\n" );
	tl->resort();
	printf( "## Sorted elements.\n" );
	tl->print();

	tv_in->setOutput(0, 0.23);
	tv_in->setOutput(1, 0.63);
	tl->update( 7.43 );
	printf( "result : %f\n", tv_res->getInput(0) );
	printf( "## Updated 1.\n" );
	tv_in->setOutput(0, 0.43);
	tv_in->setOutput(1, 0.53);
	tl->update( 17.43 );
	printf( "result : %f\n", tv_res->getInput(0) );
	printf( "## Updated 2.\n" );

	E_RELEASE(tl);	
	E_RELEASE(tsin);	
	E_RELEASE(tsc);	
	E_RELEASE(tv_in);	
	E_RELEASE(tv_res);	
	E_RELEASE(tv_nz);	
	//E_RELEASE(nzu);	

}

void print_world( Core::World * w )
{
	printf( "Textures(%i):\n", w->textures().count() );
	e6::Iter<Core::Texture> *it = w->textures().first();
	while ( it->count() )
	{
		Core::Texture * t = it->next();
		printf( "\t%s\n", t->getName() );
		E_RELEASE( t );
	}
	E_RELEASE( it );

	printf( "Shaders(%i):\n", w->shaders().count() );
	e6::Iter<Core::Shader> *is = w->shaders().first();
	while ( is->count() )
	{
		Core::Shader * s = is->next();
		printf( "\t%s\n", s->getName() );
		E_RELEASE( s );
	}
	E_RELEASE( is );

}


int main(int argc, char **argv)
{
	printf("!\n%s\n", argv[0]);

	e6::Engine * e = e6::CreateEngine();	

	const char * v = e->getVersion();
	std::cout <<  "Core.Engine = <" << v << ">\n";




	Core::World * world  = (Core::World *)e->createInterface( "Core", "Core.World" );	
	Core::Node * root  = world->getRoot();	
	
	for ( int i=1; i<argc; i++ )
	{
		if ( ! strcmp( argv[i], "-tl" ) )
		{
			tl_test(e);
			continue;
		}
		world->load( e, argv[i] );
	}

	root->synchronize();
	printRec( root );
	print_world(world);

printf( "##ClearwoRLD\n");
//	root->addref();
	world->clear();
printf( "##show();\n");
	printRec( root );
	print_world(world);
printf( "##REIMPORT\n");

	for ( int i=1; i<argc; i++ )
	{
		if ( ! strcmp( argv[i], "-tl" ) )
		{
			continue;
		}
		world->load( e, argv[i] );
	}

printf( "##synchronize();\n");
	root->synchronize();
printf( "##show();\n");
	printRec( root );
	print_world(world);
printf( "##save();\n");
	world->save( e, "../res/e6/my.e6" );

	E_RELEASE(world);	
	E_RELEASE(root);	


	E_RELEASE(e);	
	printf("!\n");

	return 0;
}
