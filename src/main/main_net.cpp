#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../Net/Net.h"
#include "../e6/sys/w32/e6_thread.h"
#include "../Application/Application.h"
#include "../Audio/Audio.h"
#include "../Physics/Physics.h"
#include "../Speech/Speech.h"



using e6::uint;

struct Console : Net::Connection
{
	virtual uint handle(  Net::Socket & socket ) 
	{
		char b[1024], *bp=b;
		*bp=0;
		while ( (*bp = fgetc( stdin )) )
		{
			if (*bp=='\r') break;
			if (*bp=='\n') break;
			bp ++;
		}
		bp ++;
		*bp=0;
		uint n = strlen( b );
		printf( "%i> %s\n",n,b );
		n = socket.write( b, n );

		if ( n = socket.read( b, 1024 ) )
		{
			b[n]=0;
			printf( "<%i %s\n",n,b );
		}
		return 1;
	}
} con;


struct Echo : Net::Connection
{
	virtual uint handle( Net::Socket & socket ) 
	{
		char b[1024];
		uint n = socket.read( b, 1024 );
		b[n]=0;
		printf( "<%i %s\n",n,b );
		if ( n )
		{
			n = socket.write( b, n );
			printf( ">%i %s\n",n,b );
			return 1;
		}
		return 0;
	}
} echo;



struct UdpConsole : Net::Connection
{
	virtual uint handle(  Net::Socket & socket ) 
	{
		uint n = 0;
		char b[1024];
		if ( n = socket.read( b, 1024 ) )
		{
			b[n]=0;
			printf( "<%i %s\n",n,b );
		}
		return n;
	}

	virtual uint loop(  Net::UdpClient * client  ) 
	{
		for ( int ever=1; ever; ever )
		{
			char b[1024], *bp=b;
			*bp=0;
			while ( (*bp = fgetc( stdin )) )
			{
				if (*bp=='\r') break;
				if (*bp=='\n') break;
				bp ++;
			}
			bp ++;
			*bp=0;
			uint n = strlen( b );
			printf( "%i> %s\n",n,b );

			n = client->write( b, n );
			if ( ! n ) 
				return 0;

			if ( ! strncmp( b, "ok.",3 ) )
				break;
		}
		return 1;
	}
} async;

struct UdpGame 
	: Net::Connection
	, Physics::Simulation::Callback
{

	Physics::Simulation * phys;
	Core::Node * box;
	Core::Node * ball;
	Core::Node * player[2];

	float timeStep;
	bool hit;
	bool verbose;


	//enum
	//{
	//	FIELD_X = 0,
	//	FIELD_Y = 0,
	//	FIELD_W = 640,
	//	FIELD_H = 480,

	//	PLAYER_HEIGHT = 60,
	//	PLAYER_OUT = -600
	//};
	//struct Ball
	//{
	//	int x, y;
	//	int dx,dy;
	//	Ball() : x(30),y(30),dx(6),dy(5) {}
	//	void move()
	//	{
	//		x += dx;
	//		y += dy;
	//	}
	//} ball;

	struct GameStatePacket
	{
		int frame;
		int event;
		int score_l;
		int score_r;
		e6::float3 ball;
		e6::float3 player[2];
		enum
		{
			EVENT_NONE			= 0,
			EVENT_ADDPLAYER		= 1,
			EVENT_SCORE_LEFT	= 2,
			EVENT_SCORE_RIGHT	= 3,
			EVENT_HIT_WALL		= 4,
			EVENT_HIT_PLAYER	= 5,
			EVENT_RESTART		= 6,
			EVENT_QUIT			= 7
		};

		GameStatePacket()
			: frame(0)
			, event(0)
			, score_l(0)
			, score_r(0)
		{
		}

		char * to_str( char* str )
		{
			str[0]=0;
			sprintf( str, "%i %i %i %i [%3.3f %3.3f %3.3f] [%3.3f %3.3f %3.3f] [%3.3f %3.3f %3.3f]", frame,event,score_l,score_r,ball.x,ball.y,ball.z,player[0].x,player[0].y,player[0].z,player[1].x,player[1].y,player[1].z);
			return str;
		}
		bool from_str( char* str )
		{
			if ( 13 != sscanf(str, "%i %i %i %i [%f %f %f] [%f %f %f] [%f %f %f]", &frame,&event,&score_l,&score_r,&ball.x,&ball.y,&ball.z,&player[0].x,&player[0].y,&player[0].z,&player[1].x,&player[1].y,&player[1].z) )
			{
				printf( "parse error [%s]\n", str );
				return 0;
			}
			return 1;
		}
	} state;

	struct PlayerPacket
	{
		int play_id;	
		int cmd;	

		PlayerPacket()
			: play_id(0)
			, cmd(0)
		{
		}

		char * to_str( char* str )
		{
			str[0]=0;
			sprintf( str, "%i %i", play_id,cmd);
			return str;
		}
		bool from_str( char* str )
		{
			if ( 2 != sscanf(str, "%i %i", &play_id,&cmd) )
			{
				printf( "parse error [%s]\n", str );
				return 0;
			}
			return 1;
		}
	}; // player[2];


	UdpGame() 
		: verbose(1) 
		, hit(0)
		, timeStep(0.04f)
	{
	}

	~UdpGame() 
	{
		E_RELEASE(phys);	
		E_RELEASE(ball);	
		E_RELEASE(box);	
		E_RELEASE(player[0]);	
		E_RELEASE(player[1]);	
	}

	virtual uint collide( Core::Node * o1, Core::Node * o2, float *ipoint, float *inormal, float depth )
	{
		const e6::float3 &p = o2->getPos();
		if ( o1 == box )
		{
			if ( inormal[0]==1 )
			{
				state.score_l ++;
				state.event = GameStatePacket::EVENT_SCORE_LEFT;
			}
			else
			if ( inormal[0]==-1 )
			{
				state.score_r ++;
				state.event = GameStatePacket::EVENT_SCORE_RIGHT;
			}
			else
			if (o2!=player[0] && o2!=player[1])
			{
				state.event = GameStatePacket::EVENT_HIT_WALL;
			}

			//if ( o2 == player[0] || o2 == player[1] )
			//{
			//	o2->addPos( e6::float3(inormal) * -0.5f );
			//	this->phys->setVelocity(o2,0,0,0);
			//}
		}
		if ( o1 == player[0] )
		{
			state.event = GameStatePacket::EVENT_HIT_PLAYER;
		}
		printf( "[%3.3f %3.3f %3.3f] %8s -> %-8s [%3.3f %3.3f %3.3f]\t[%3.3f %3.3f %3.3f]\t[%3.3f]\n", p[0],p[1],p[2], o2->getName(), o1->getName(), ipoint[0], ipoint[1], ipoint[2], inormal[0], inormal[1], inormal[2], depth );
		hit = true;
		return 1;
	}

	void physInit( e6::Engine * engine, Core::Node *)
	{
		phys  = (Physics::Simulation *)engine->createInterface( "Physics", "Physics.Simulation" );	
		box   = (Core::Node *)engine->createInterface( "Core", "Core.FreeNode" );
		ball  = (Core::Node *)engine->createInterface( "Core", "Core.FreeNode" );
		player[0] = (Core::Node *)engine->createInterface( "Core", "Core.FreeNode" );
		player[1] = (Core::Node *)engine->createInterface( "Core", "Core.FreeNode" );

		ball->setName("ball");	
		ball->setPos( e6::float3(0,0,0) );	
		ball->setSphere(1);
		phys->addSphere( 1, ball );
		phys->setVelocity( ball, 2.9f, 0.9f, 0.00f );

		box->setName("box");	
		box->setPos( e6::float3(0,0,0) );
		box->setSphere(30);

		phys->addPlane( 0, box, 0,-1,0,-15 );
		phys->addPlane( 0, box, 0,1,0, -15 );
		phys->addPlane( 0, box, 1,0,0, -15 );
		phys->addPlane( 0, box, -1,0,0,-15 );
		phys->addPlane( 0, box, 0,0,1, -15 );
		phys->addPlane( 0, box, 0,0,-1,-15 );

		player[0]->setName("p1"); 
		player[0]->setPos( e6::float3(-12,0,0) );
		phys->addBox( 1, player[0], 1,3,2 );

		player[1]->setName("p2"); 
		player[1]->setPos( e6::float3( 12,0,0) );
		phys->addBox( 1, player[1], 1,3,2 );

		phys->setTimeStep( timeStep );
		phys->setGravity(0,0,0);

		phys->setMass( ball, 1.0f );
		phys->setMass( player[0], 100.0f );
		phys->setMass( player[1], 100.0f );

	}

	void restart()
	{
		state.event = GameStatePacket::EVENT_RESTART;
		state.score_l = 0;
		state.score_r = 0;
		state.ball = e6::float3(0,0,0);
		state.player[0] = e6::float3(-12,0,0);
		state.player[1] = e6::float3(12,0,0);
		state.frame = 0;
		phys->setVelocity( ball, 2.8f, 0.8f, 0.0f );
	}

	virtual uint handle(  Net::Socket & socket ) 
	{
		char b[1024];
		uint n = socket.read( b, 1024 );
		b[n]=0;
		//printf( "<%i %s\n",n,b );
		if ( ! n ) 
			return 0;

		PlayerPacket from;
		from.from_str( b );
		if ( (from.play_id >= 0) && (from.play_id <= 3) ) 
		{
			float force = 1111.57f;
			switch ( from.cmd )
			{
				default:
					state.player[ from.play_id ].y = 3;
					state.event = state.EVENT_ADDPLAYER;		
					break;
				case 1:
					phys->addForce( player[from.play_id], 0,force,0 );
					//player[from.play_id]->addPos(e6::float3(0,force,0));
					break;
				case 2:
					phys->addForce( player[from.play_id], 0,-force,0 );
					//player[from.play_id]->addPos(e6::float3(0,-force,0));
					break;
				case 3:
					phys->addForce( player[from.play_id], 0,0,force );
					//player[from.play_id]->addPos(e6::float3(0,0,force));
					break;
				case 4:
					phys->addForce( player[from.play_id], 0,0,-force );
					//player[from.play_id]->addPos(e6::float3(0,0,-force));
					break;
				case 5:
					restart();
					break;
				case 6:
					//socket.write(0,0);
					//memset( &client_address[from.play_id], 0, sizeof(SOCKADDR_IN) );
					printf( "removed client %i\n", from.play_id );
					player[from.play_id]->setPos(e6::float3(0,-200,0));
					break;
			}
		}
		return n;
	}


	uint loop( Net::UdpServer * server )
	{
		char b[1024];
		float t = 0;

		while ( 1 )
		{
			// apply friction to players:
			float fric=0.94f;
			e6::float3 v = phys->getVelocity(player[0]);
			phys->setVelocity(player[0],v.x*fric,v.y*fric,v.z*fric);
			
			v = phys->getVelocity(player[1]);
			phys->setVelocity(player[1],v.x*fric,v.y*fric,v.z*fric);

			// see that the ball keeps flying in dir of the players:
			v = phys->getVelocity(ball);
			if ( fabs(v.x)<0.77f )
			{
				float f = ( v.x>0?35:-35 );
				phys->addForce(ball,f,0,0);
			}

			state.frame ++;
			state.event = state.EVENT_NONE;		

			t += timeStep;

			hit = false;
			phys->run( this, t );

			if ( (state.frame%500==0) )
			{
				printf( "--------- step %4i --- %3.3f --------\n", state.frame, t );
				e6::float3 p;
				p = ball->getPos();
				printf( " -- ball [%3.3f %3.3f %3.3f] ", p[0],p[1],p[2]);
				p = phys->getVelocity(ball);
				printf( " [%3.3f %3.3f %3.3f]\n", p[0],p[1],p[2]);
				p = player[0]->getPos();
				printf( " -- p1 [%3.3f %3.3f %3.3f]\n", p[0],p[1],p[2]);
				p = player[1]->getPos();
				printf( " -- p2 [%3.3f %3.3f %3.3f]\n", p[0],p[1],p[2]);
			}

			state.ball = ball->getPos();
			state.player[0] = player[0]->getPos();
			state.player[1] = player[1]->getPos();
			state.to_str(b);

			server->broadcast( b, strlen(b) );
			Sleep(15);
		}
		return 1;
	}
} game;


struct AutoPong
	: Net::Connection
{
	UdpGame::GameStatePacket state;
	uint id;
	Net::UdpClient * client;

	AutoPong(e6::Engine * engine,uint i=0) : id(i),client(0) 
	{
		client = (Net::UdpClient*) engine->createInterface( "Net", "Net.UdpClient" ) ;
		if (  client != 0 )
		{
			client->connect( "localhost", 5555, *this );
			char * _greeting[] = {"0 0","1 0","2 0","3 0"}; // 'id 0'
			client->write( _greeting[id], 3 ); // send 'wake up' message
		}

	}
	virtual ~AutoPong() 
	{
		E_RELEASE(client);
	}

	bool sendPlayer( int ev )
	{
		UdpGame::PlayerPacket packet;
		packet.play_id = id;
		packet.cmd = ev;
		char b[100];
		char *s = packet.to_str(b);
		return client->write( s, strlen(s) );
	};

	virtual uint loop() 
	{
		while( 1 )
		{
			e6::float3 p = state.player[id];
			e6::float3 b = state.ball;
			e6::float3 d = p - b;
			if ( d.y < -0.2f )
			{
				sendPlayer(1);
			}
			else
			if ( d.y > 0.2f )
			{
				sendPlayer(2);
			}
			else
			if ( d.z < -0.1f )
			{
				sendPlayer(3);
			}
			else
			if ( d.z > 0.1f )
			{
				sendPlayer(4);
			}
			Sleep(63);
		}
		return 1;
	}

	virtual uint handle(  Net::Socket & socket ) 
	{
		float scalex = 0.08f;
		float scaley = 0.08f;
		uint n = 0;
		char b[1024];
		if ( n = socket.read( b, 1024 ) )
		{
			b[n]=0;
		//	printf( "<%i %s\n",n,b );
			state.from_str( b );
			//switch(state.event)
			//{
			//	default:
			//	case 0:
			//		break;
			//	case state.EVENT_HIT_WALL: 
			//		break;
			//	case state.EVENT_HIT_PLAYER: 
			//		break;
			//	case state.EVENT_SCORE_LEFT:
			//		break;
			//	case state.EVENT_SCORE_RIGHT:
			//		break;
			//}
		}
		return n;
	}
};

struct GameApp 
	: Net::Connection
	, Application::EventHandler
{
	Application::Main * app;
	Audio::Player * audio;
	Net::UdpClient * client;
	UdpGame::GameStatePacket state;
	Core::Node * ball;
	Core::Node * player[2];

	uint id;

	GameApp(e6::Engine * engine,int id=0)
		: app(app)
		, audio(0)
		, client(client)
		, ball(0)
		, id(id)
	{
		for ( int i=0; i<2; i++ )
		{
			player[i] = 0;
		}
		do  // once
		{
			app = (Application::Main *)engine->createInterface( "Application", "Application.Main" ) ;
			if ( ! app )
			{
				std::cout <<  "Could not load host " << "Application::Main" << ".\n";
				break;
			}

			if ( ! app->init( engine ) )
			{
				std::cout <<  "Could not init Application .\n";
				break;
			}

			if ( ! app->loadRenderer( "RendererDx9" ) )
			{
				std::cout <<  "Could not load '" << "RendererDx9" << "'.\n";
				break;
			}

			audio = (Audio::Player *)engine->createInterface( "Audio", "Audio.Player" ) ;
			if ( ! audio )
			{
				std::cout <<  "Could not load host " << "Audio.Player" << ".\n";
				break;
			}
			audio->loadVoice( "E:/wav/gamespy/buddy_select.wav" );
			audio->loadVoice( "E:/wav/gamespy/key_type.wav" );
			audio->loadVoice( "E:/wav/pinball/sound5.wav" );

			audio->loadVoice( "E:/wav/ambient/Music_from_hell_1.wav" );
			audio->setState( 3, Audio::Player::LOOPED );

			client = (Net::UdpClient*) engine->createInterface( "Net", "Net.UdpClient" ) ;
			if (  client != 0 )
			{
				client->connect( "pedro", 5555, *this );
				char * _greeting[] = {"0 0","1 0","2 0","3 0"}; // 'id 0'
				client->write( _greeting[id], 3 ); // send 'wake up' message
			}

			//if ( ! app->loadScriptEngine( scr ) )
			//{
			//	std::cout <<  "Could not load interpreter " << scr << ".\n";
			//	break;
			//}

			//if ( fun )
			//{
			//	if ( ! app->bindScriptEvent( 1, fun ) )
			//	{
			//		std::cout <<  "Could not bind ScriptEvent ( " << fun << " ).\n";
			//		break;
			//	}
			//}

			//if ( ! app->startScriptServer( port ) )
			//{
			//	std::cout <<  "Could not start startScriptServer( " << port << " ) " << scr << ".\n";
			//	break;
			//}

			//if ( rsc )
			//{
			//	if ( ! app->loadResource( rsc ) )
			//	{
			//		std::cout <<  "Could not load resource '" << rsc << "'.\n";
			//		break;
			//	}
			//}
			//
			//! if the app was started successful, this will block for the app's lifetime
			//
			app->bindCppEvent( Application::ET_KEY + (id&1?VK_UP:VK_F5), this );
			app->bindCppEvent( Application::ET_KEY + (id&1?VK_DOWN:VK_F6), this );
			app->bindCppEvent( Application::ET_KEY + (id&1?VK_RIGHT:VK_F8), this );
			app->bindCppEvent( Application::ET_KEY + (id&1?VK_LEFT:VK_F7), this );
			app->bindCppEvent( Application::ET_KEY + VK_F12, this );
			app->bindCppEvent( Application::ET_KEY + VK_F11, this );

			//app->loadResource("box.e6");
			app->loadResource("pong2.e6");
			Core::World * world = app->getWorld();

			for ( int i=0; i<2; i++ )
			{
				char s[10];
				sprintf( s, "Stick%d", i );
				player[i] = world->findRecursive(s);
			}
			ball = world->findRecursive("Sphere");

			Core::Camera * cam = app->getCamera();
			float px[] = {-44,44};
			float pr[] = {e6::Pi/2,e6::Pi/2};
			cam->setPos( e6::float3(px[(id)],0,0) );
			cam->setRot( e6::float3(0,pr[(id)],0) );
			E_RELEASE(cam);

			E_RELEASE(world);
			if ( ! app->run( 0 ) )
			{
				std::cout <<  "Could not run Application .\n";
				break;
			}

		} while(0); // once	
	}

	virtual ~GameApp() 
	{
		E_RELEASE(client); // kill client thread first !!

		for ( int i=0; i<2; i++ )
		{
			E_RELEASE(player[i]);
		}
		E_RELEASE(ball);

		E_RELEASE(app);

		E_RELEASE(audio);
	}

	bool sendPlayer( int ev )
	{
		UdpGame::PlayerPacket packet;
		packet.play_id = id;
		packet.cmd = ev;
		char b[100];
		char *s = packet.to_str(b);
		return client->write( s, strlen(s) );
	};

	virtual bool handleEvent( int ev, float p0, float p1, float p2 )
	{
		if ( ev == Application::ET_KEY + ( id&1? VK_UP : VK_F6 ) )
		{
			sendPlayer(1);
		}
		else
		if ( ev == Application::ET_KEY + ( id&1? VK_DOWN : VK_F5 ) )
		{
			sendPlayer(2);
		}
		if ( ev == Application::ET_KEY + ( id&1? VK_LEFT : VK_F8 ) )
		{
			sendPlayer(3);
		}
		else
		if ( ev == Application::ET_KEY + ( id&1? VK_RIGHT : VK_F7 ) )
		{
			sendPlayer(4);
		}
		else
		if ( ev == Application::ET_KEY + VK_F9 )
		{
			sendPlayer(6);
		}
		else
		if ( ev == Application::ET_KEY + VK_F11 )
		{
			sendPlayer(5);
			audio->setState(0,2);
			audio->setState(1,2);
			audio->setState(2,2);
		}
		else
		if ( ev == Application::ET_KEY + VK_F12 )
		{
			char b[512];
			printf("%s\n", state.to_str(b));
		}
		else
			return 0;
		return 1;
	};

	virtual uint handle(  Net::Socket & socket ) 
	{
		float scalex = 0.08f;
		float scaley = 0.08f;
		uint n = 0;
		char b[1024];
		if ( n = socket.read( b, 1024 ) )
		{
			b[n]=0;
		//	printf( "<%i %s\n",n,b );
			state.from_str( b );
			switch(state.event)
			{
				default:
				case 0:
					break;
				case state.EVENT_HIT_WALL: 
					audio->setPan(0, -500 + ball->getPos().x * 1000);
					audio->setState(0,Audio::Player::PLAYING);
					break;
				case state.EVENT_HIT_PLAYER: 
					audio->setPan(1, -500 + ball->getPos().x * 1000);
					audio->setState(1,Audio::Player::PLAYING);
					break;
				case state.EVENT_SCORE_LEFT:
					audio->setPan(2,-2000);
					audio->setState(2,Audio::Player::PLAYING);
					break;
				case state.EVENT_SCORE_RIGHT:
					audio->setPan(2,2000);
					audio->setState(2,Audio::Player::PLAYING);
					break;
			}
			for ( int i=0; i<2; i++ )
			{
				if ( player[i] )
				{
					player[i]->setPos( state.player[i] );
				}
			}
			if ( ball )
			{
				ball->setPos( state.ball );
			}
		}
		return n;
	}

};




int main(int argc, char **argv)
{
	char * in = argc>1 ? argv[1] : 0;
	uint port = argc>2 ? atoi(argv[2]) : 5555;
	//char *topic = argc>3 ? argv[3] : 0;
	bool r = false;
	e6::Engine * e = e6::CreateEngine();	


//	//Speech::Voice * sp = (Speech::Voice*) e->createInterface( "Speech", "Speech.Voice" );
//	//sp->speak("hello world");
//	//E_RELEASE(sp);
//
//	struct SPK : Speech::Recognizer::Listener
//	{
//		virtual uint listen( const char * txt, uint ruleID, uint alternation ) 
//		{
//			if ( alternation == 9999 )
//				; //printf( "hyp.    %s\n", txt );
//			else
//			if ( ! ruleID )
//				printf( "%4d a%3d %s\n",ruleID, alternation, txt );
//			else
//				printf( "%4d ok.  %s\n",ruleID, txt );
//			return 1;
//		}
//		
//	} spk;
//
//	Speech::Recognizer * sr = (Speech::Recognizer*) e->createInterface( "Speech", "Speech.Recognizer" );
//
//	if ( in )
//		sr->loadGrammar( in );
//
//	if ( port < 5555 )
//		sr->enableNumAlternates( port );
//
//	if ( topic )
//		sr->showUI(topic);
//
////	sr->adapt( "foo" );
//
//	sr->start( &spk, false );
//	E_RELEASE(sr);
//
//return 0;
//
//
//
	uint id   = argc>3 ? atoi(argv[3]) : 0;
	uint autoplay   = argc>4 ? 1 : 0;

	if ( autoplay )
	{
		AutoPong app( e, id );
		app.loop();
	}
	else
	if ( in )
	{
		GameApp app( e, id );
	}
	else
	{
		Net::UdpServer * server = (Net::UdpServer*) e->createInterface( "Net", "Net.UdpServer" );
		r = ( server != 0 );
		if ( r )
		{
			game.physInit( e, 0 );

			r = server->start( port, game );
		}
		if ( r )
		{
			r = game.loop( server );
		}
		E_RELEASE(server);	
	}




	//if ( in )
	//{
	//	Net::TcpClient * client = (Net::TcpClient*) e->createInterface( "Net", "Net.TcpClient" );
	//	r = ( client != 0 );
	//	if ( r )
	//	{
	//		r = client->connect( argv[1], port, con );
	//	}
	//	if ( r )
	//	{
	//		for(int ever=1; ever; ever )
	//		{}
	//	}
	//	E_RELEASE(client);	
	//}
	//else
	//{
	//	Net::TcpServer * server = (Net::TcpServer*) e->createInterface( "Net", "Net.TcpServer" );
	//	r = ( server != 0 );
	//	if ( r )
	//	{
	//		server->start( port, echo );
	//		for(int ever=1; ever; ever )
	//		{}
	//	}
	//	E_RELEASE(server);	
	//}

	E_RELEASE(e);	
	return r!=0;
}
