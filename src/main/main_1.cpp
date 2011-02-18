#include "e6_sys.h"
#include "e6_impl.h"
#include "e6_enums.h"
#include "e6_math.h"
//#include "../dl_0/dl_0.h"
#include "../Core/Core.h"
#include "../Physics/Physics.h"

using e6::float3;


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

struct CB : Physics::Simulation::Callback
{
	virtual uint collide( Core::Node * o1, Core::Node * o2, float *ipoint, float *inormal, float depth )
	{
		const float3 &p = o1->getPos();
		printf( "[%3.3f %3.3f %3.3f] %8s -> %-8s [%3.3f %3.3f %3.3f]\t[%3.3f %3.3f %3.3f]\t[%3.3f]\n", p[0],p[1],p[2], o1->getName(), o2->getName(), ipoint[0], ipoint[1], ipoint[2], inormal[0], inormal[1], inormal[2], depth );
	}
};

void physTest( Physics::Simulation * phys, e6::Engine * engine, Core::Node * root )
{
	CB cb;
	float t = 0;
	Core::Node * o1 = (Core::Node *)engine->createInterface( "Core", "Core.FreeNode" );
	Core::Node * o2 = (Core::Node *)engine->createInterface( "Core", "Core.FreeNode" );
	Core::Node * p1 = (Core::Node *)engine->createInterface( "Core", "Core.FreeNode" );
	Core::Node * p2 = (Core::Node *)engine->createInterface( "Core", "Core.FreeNode" );

	o1->setName("o1");	o1->setPos( float3(0,2,0) );	o1->setSphere(1);
	o2->setName("o2");	o2->setPos( float3(0,2,3) );	o2->setSphere(1);

	p1->setName("p1");
	p2->setName("p2");

	phys->addSphere( 1, o1 );
	phys->addSphere( 1, o2 );
	phys->addPlane( 0, p1, 0,1,0,0 );
	phys->addPlane( 0, p2, 0,-1,0,-5 );

		phys->addForce( o1, 0, 5,  10 );
		phys->addForce( o2, 0, 7, -10 );

	for ( int i=0; i<400; i++ )
	{
		t += 0.04;
		printf( "--------- step %2i -----------\n", i );

		phys->run( &cb, t );

		float3 p;
		p = o1->getPos();
		printf( " -- o1 [%3.3f %3.3f %3.3f]\n", p[0],p[1],p[2]);
		p = o2->getPos();
		printf( " -- o2 [%3.3f %3.3f %3.3f]\n", p[0],p[1],p[2]);
	}
	E_RELEASE(o1);	
	E_RELEASE(o2);	
	E_RELEASE(p1);	
	E_RELEASE(p2);	
}

int main(int argc, char **argv)
{

	e6::Engine * e = e6::CreateEngine();	

	Core::World * world  = (Core::World *)e->createInterface( "Core", "Core.World" );	
	Core::Node  * root   = world->getRoot();	
	
	for ( int i=1; i<argc; i++ )
	{
		world->load( e, argv[i] );
	}
	root->synchronize();

	Physics::Simulation * phys  = (Physics::Simulation *)e->createInterface( "Physics", "Physics.Simulation" );	
	phys->setGravity(0,0,0);
	physTest( phys, e, root );

	//printRec( root );

	E_RELEASE(phys);	
	E_RELEASE(world);	
	E_RELEASE(root);	
	E_RELEASE(e);	
	printf("!\n");

	return 0;
}
