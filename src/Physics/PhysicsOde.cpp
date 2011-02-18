//#include "e6.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../e6/e6_math.h"

#include "Physics.h"
#include <ode/ode.h>

#include <time.h>
#include <map>


using e6::uint;
using e6::ClassInfo;
using e6::float3;




namespace PhysicsOde
{
	using Physics::Simulation;

	struct CSimulation 
		: public e6::CName<Simulation, CSimulation >
	{

		std::map< Core::Node*, dGeomID > objects;
		typedef std::map< Core::Node*, dGeomID >::iterator Iter;

		dWorldID world;
		dSpaceID space[2];
		dJointGroupID contactgroup;

		float timeStep;

		CSimulation() 
			: world(0)
			, timeStep( 0.04f )
		{
			dInitODE();
			dRandSetSeed (time(0));
			world = dWorldCreate();
			contactgroup = dJointGroupCreate (0);
			dWorldSetGravity( world, 0,0,-9.8 );
			space[GT_MOVING] = dSimpleSpaceCreate(0);
			space[GT_STATIC] = dSimpleSpaceCreate(0);
		}

		~CSimulation() 
		{
			dJointGroupEmpty (contactgroup);
			dSpaceDestroy( space[GT_MOVING] );
			dSpaceDestroy( space[GT_STATIC] );
			dWorldDestroy (world);
			dCloseODE();
		}

		virtual uint remObject( Core::Node * ptr )
		{
			Iter it = objects.find( ptr );
			if ( it != objects.end() )
			{
				dGeomID geom = it->second;
				if ( geom )	
				{
					dGeomDestroy(geom);
				}
				objects.erase( it );
				return 1;
			}
			return 0;
		}

		uint addObject( dGeomID geom, Core::Node * ptr )
		{
			E_ASSERT(geom);
			E_ASSERT(ptr);
			dBodyID body = dBodyCreate (world);
			dGeomSetBody( geom,body );
			const float3 & p = ptr->getPos();
			dGeomSetPosition ( geom, p[0],p[1],p[2] );
			dGeomSetData ( geom, (void*)ptr );
			objects[ ptr ] = geom;
			return objects.size();
		}

		virtual uint addCapsule( uint geomType, Core::Node * ptr, float length, float radius )
		{
			$();
			dGeomID geom = dCreateCapsule( space[geomType], radius, length );
			return addObject( geom, ptr );
		}

		virtual uint addBox( uint geomType, Core::Node * ptr, float ex, float ey, float ez ) 
		{
			$();
			E_ASSERT(ptr);
			dGeomID geom = dCreateBox( space[geomType], ex,ey,ez );
			return addObject( geom, ptr );
		}
		virtual uint addSphere( uint geomType, Core::Node * ptr )
		{
			$();
			E_ASSERT(ptr);
			dGeomID geom = dCreateSphere( space[geomType], ptr->getSphere() );
			return addObject( geom, ptr );
		}
		virtual uint addPlane( uint geomType, Core::Node * ptr, float x, float y, float z, float d )
		{
			$();
//			E_ASSERT(ptr);
//		    dBodyID body = dBodyCreate (world);
			dGeomID geom = dCreatePlane( space[geomType], x,y,z,d );
//			dGeomSetBody( geom,body );
			dGeomSetData ( geom, (void*)ptr );
			objects[ ptr ] = geom;
			return objects.size();
		}


		virtual uint setMass( Core::Node * ptr, float m ) 
		{
			dBodyID body = dGeomGetBody( objects[ ptr ] );
			if ( ! body )
			{
				printf( "no body for %s\n", ptr->getName() ) ;
				return 0;
			}
			dMass dm ;
			dBodyGetMass( body, &dm );

			dm.mass = m;
			dBodySetMass( body, &dm );
			return 1;
		}
		virtual float getMass( Core::Node * ptr ) 
		{
			dBodyID body = dGeomGetBody( objects[ ptr ] );
			{
				printf( "no body for %s\n", ptr->getName() ) ;
				return 0;
			}

			dMass dm;
			dBodyGetMass( body, &dm );

			return dm.mass;
		}


		virtual e6::float3 getVelocity( Core::Node * ptr )
		{
			e6::float3 f3;
			dBodyID body = dGeomGetBody( objects[ ptr ] );
			if ( ! body ) 
			{
				printf( "no body for %s\n", ptr->getName() ) ;
				return f3;
			}
			const dReal * df = dBodyGetLinearVel(body);
			if ( df ) 
			{
				f3[0]=df[0];f3[1]=df[1];f3[2]=df[2];
			}
			return f3;
		}

		virtual uint setVelocity( Core::Node * ptr, float x, float y, float z )
		{
			dBodyID body = dGeomGetBody( objects[ ptr ] );
			dBodySetLinearVel( body, x, y, z );
			return 1;
		}

		virtual uint setForce( Core::Node * ptr, float x, float y, float z )
		{
			dBodyID body = dGeomGetBody( objects[ ptr ] );
			dBodySetForce( body, x, y, z );
			return 1;
		}
		virtual uint addForce( Core::Node * ptr, float x, float y, float z )
		{
			dBodyID body = dGeomGetBody( objects[ ptr ] );
			dBodyAddForce( body, x, y, z );
			return 1;
		}

		virtual uint setTorque( Core::Node * ptr, float x, float y, float z )
		{
			dBodyID body = dGeomGetBody( objects[ ptr ] );
			dBodySetTorque( body, x, y, z );
			return 1;
		}
		virtual uint addTorque( Core::Node * ptr, float x, float y, float z )
		{
			dBodyID body = dGeomGetBody( objects[ ptr ] );
			dBodyAddTorque( body, x, y, z );
			return 1;
		}

		virtual void setGravity( float gx, float gy, float gz )
		{
			dWorldSetGravity( world, gx,gy,gz );
		}
		virtual void setTimeStep( float dt )
		{
			timeStep = dt;
		}


		struct _CBArgs
		{
			Callback * cb;
			dWorldID world;
			dJointGroupID contactgroup;
		};

		virtual uint run( Callback * cb, float timeNow )
		{
			_CBArgs args;
			args.cb = cb;
			args.world = world;
			args.contactgroup = contactgroup;

			static float _t=0; 
			float dt = timeNow - _t;
			int nsteps = (dt > 1.0f) ? 1 : (int)ceil( dt / timeStep );
			_t = timeNow;

			updateGeoms();

			// run sim steps
			for ( int i=0; i<nsteps; i++ )
			{
				//printf( "-- sim %i/%i %3.3f --\n", i,nsteps, timeNow );

			    dSpaceCollide( space[GT_MOVING], &args, &_nearCallback );
			    dSpaceCollide2( (dGeomID)space[GT_MOVING], (dGeomID)space[GT_STATIC], &args, &_nearCallback );

				dWorldStep( world, timeStep );
				// remove all contact joints
				dJointGroupEmpty( contactgroup );
			}

			updateNodes();

			return 1;
		}

		void updateNodes()
		{
			// update moving nodes:
			int n = dSpaceGetNumGeoms( space[GT_MOVING] );
			for ( int o=0; o<n; ++o )
			{
				dGeomID geom = dSpaceGetGeom( space[GT_MOVING], o );
				Core::Node * node = (Core::Node *) dGeomGetData( geom );
				node->setPos( dGeomGetPosition( geom ) ); 
			}
		}
		void updateGeoms()
		{
			// update moving nodes:
			int n = dSpaceGetNumGeoms( space[GT_MOVING] );
			for ( int o=0; o<n; ++o )
			{
				dGeomID geom = dSpaceGetGeom( space[GT_MOVING], o );
				Core::Node * node = (Core::Node *) dGeomGetData( geom );
				e6::float3 p = node->getPos(); 
				dGeomSetPosition( geom, p.x,p.y,p.z );
			}
		}
		static void _nearCallback(void *data, dGeomID o1, dGeomID o2)
		{
			_CBArgs *args = (_CBArgs*)data;

			dBodyID b1 = dGeomGetBody(o1);
			dBodyID b2 = dGeomGetBody(o2);

			const int N = 4;
			dContact contact[N];
			int n = dCollide (o1,o2,N,&(contact[0].geom),sizeof(dContact));
			for (int i=0; i<n; i++) 
			{
				contact[i].surface.mode = dContactBounce ; //| dContactSoftCFM;
				contact[i].surface.mu = 1.0;//dInfinity;
				//contact[i].surface.mu2 = 1.0;
				contact[i].surface.bounce = 1.0;
				contact[i].surface.bounce_vel = 0.1;
				//contact[i].surface.soft_cfm = 0.01;

				dJointID c = dJointCreateContact( args->world, args->contactgroup, contact+i );
				dJointAttach (c,b1,b2);

				if ( args->cb )
				{
					float ip[3], in[3];
					ip[0] = contact[i].geom.pos[0];
					ip[1] = contact[i].geom.pos[1];
					ip[2] = contact[i].geom.pos[2];

					in[0] = contact[i].geom.normal[0];
					in[1] = contact[i].geom.normal[1];
					in[2] = contact[i].geom.normal[2];
					float depth = contact[i].geom.depth;

					args->cb->collide( (Core::Node*)dGeomGetData(contact[i].geom.g1), (Core::Node*)dGeomGetData(contact[i].geom.g2), ip, in, depth );
					//args->cb->collide( (Core::Node*)dGeomGetData(o1), (Core::Node*)dGeomGetData(o2), ip, in, depth );
				}
			}
		}

	}; // CSimulation

} //namespace PhysicsOde

ClassInfo ClassInfo_PhysicsOde_Simulation = 
{	"Physics.Simulation",	 "Physics",	PhysicsOde::CSimulation::createInterface,PhysicsOde::CSimulation::classRef	};


