//#include "e6.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_math.h"

#include "Physics.h"

#include <time.h>
#include <map>


using e6::uint;
using e6::ClassInfo;
using e6::float3;
using e6::float4x4;




namespace Physics
{
	const float piover180 = e6::Pi / 180.0;

	
	struct Velocity 
	{
		float3 v;
		float3 a;
		
		void reset()	{	v = a = float3(0,0,0);	}
	
		void accel( const float3 &_a ) {   a += _a;	}
		void accel_z( float _a )	{	a[2] += _a;	}
		void accel_y( float _a )	{	a[1] += _a;	}
		void accel_x( float _a )	{	a[0] += _a;	}
		
		void update( float dt, float friction )
		{
			v += dt * a;
			a *= dt * friction;
			v *= friction;
		}
	};

	struct CSpring : e6::Class<Spring,CSpring>
	{
		float dist;
		float damping;
		Velocity vel;
		
		CSpring() : dist(5), damping(0.4) {}
			
		virtual void setDistance( float d )
		{
			dist = d;
		}
		virtual void setDamping( float d )
		{
			damping = 1.0f - d;
		}
		virtual void move( const Core::Node * fixed, Core::Node * loose, float dt )
		{
			float3 d = fixed->getPos() - loose->getPos();
			float  f = d.length() - dist;
			float f2 = fabs( f );
			if ( f2>0.001 )
			{
				if ( f2 > 10.0 ) f = 10.0; ///PPP ???

				vel.accel( d * f );
				vel.update( damping, dt );

				loose->addPos( vel.v );
			}
		}
	};


	struct CMover : e6::Class<Mover,CMover>
	{
		Velocity _vel;
		Velocity _rot;
		
		void accel( const float3 & a ){	_vel.accel(a);		}
		void accel_z( float a )		{	_vel.accel_z(a);		}
		void accel_y( float a )		{	_vel.accel_y(a);		}
		void accel_x( float a )		{	_vel.accel_x(a);		}
		
		void rot( const float3 & a ){	_rot.accel(a);		}
		void rot_z( float a )		{	_rot.accel_z(a);		}
		void rot_y( float a )		{	_rot.accel_y(a);		}
		void rot_x( float a )		{	_rot.accel_x(a);		}
		
		void reset()
		{
			_vel.reset();
			_rot.reset();
		}
		void move( Core::Node * node, float dt )
		{
			float friction = 1.0 - node->getFriction();
			_vel.update(dt,friction);
			node->addPos( _vel.v );
			_rot.update(dt,friction);
			node->addRot( _rot.v );
		}
		void moveLocal( Core::Node * node, float dt )
		{
			float4x4 lm;
			node->getWorldMatrix(lm);
			float friction = 1.0 - node->getFriction();

			_vel.update(dt,friction);
			node->addPos( lm.getEx() * _vel.v.x );
			node->addPos( lm.getEy() * _vel.v.y );
			node->addPos( lm.getEz() * _vel.v.z );

			_rot.update(dt,friction);
			node->addRot( lm.getEx() * _rot.v.x );
			node->addRot( lm.getEy() * _rot.v.y );
			node->addRot( lm.getEz() * _rot.v.z );
		}
		
	}; //Mover


	struct CBillboard : e6::Class<Billboard,CBillboard>
	{
		void face( Core::Node * node, const Core::Node * orientZ  )
		{
			//~ D3DXVECTOR3 vDir = vLookatPt - vEyePt;
			//~ if( vDir.x > 0.0f )
				//~ D3DXMatrixRotationY( &m_matBillboardMatrix, -atanf(vDir.z/vDir.x)+D3DX_PI/2 );
			//~ else
				//~ D3DXMatrixRotationY( &m_matBillboardMatrix, -atanf(vDir.z/vDir.x)-D3DX_PI/2 );
			float4x4 lm;
			node->getWorldMatrix(lm);
			float4x4 lz;
			orientZ->getWorldMatrix(lz);
			float dot = lz.getEz() * lm.getEz();
			float friction = 1.0 - node->getFriction();
			node->addRot( -dot * friction * lz.getEz()  );
		}		
		
	}; 
	
} //namespace Physics


extern ClassInfo ClassInfo_PhysicsOde_Simulation;

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Physics.Mover",		"Physics",	Physics::CMover::createInterface,Physics::CMover::classRef	},
		{	"Physics.Billboard",	"Physics",	Physics::CBillboard::createInterface,Physics::CBillboard::classRef	},
		{	"Physics.Spring",		"Physics",	Physics::CSpring::createInterface,Physics::CSpring::classRef	},
		ClassInfo_PhysicsOde_Simulation,
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 4; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ( "Physics 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
