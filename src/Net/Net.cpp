#include <assert.h>

#define MAXBUF 0xfffe

#ifdef _WIN32  
#include <winsock.h>
#include <windows.h>
#include <time.h>
#ifdef _MBCS /* mvs 6 defines this */
#include <malloc.h>
#define PORT         unsigned short
#else        /* any other win32 compiler */
#define PORT         unsigned long
#endif       /* _MCBS */
#define ADDRPOINTER   int*
#else          /* ! win32 */
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT         unsigned short
#define SOCKET       int
#define HOSTENT      struct hostent
#define SOCKADDR     struct sockaddr
#define SOCKADDR_IN  struct sockaddr_in
#define ADDRPOINTER  unsigned int*
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#endif /* _WIN32 */

#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"

#ifdef WIN32
 #include "../e6/sys/w32/e6_thread.h"
#else
 #include "../e6/sys/nix/e6_thread.h"
#endif

#include "Net.h"
#include "../Script/Script.h"



using e6::uint;
using e6::ClassInfo;


namespace Net
{
    //-------Helper------------------------------------------------------------------------------------------------------------------------


	#ifdef WIN32
		
		struct _INIT_W32DATA
		{
			WSADATA w;
			_INIT_W32DATA() {	WSAStartup( MAKEWORD( 2, 1 ), &w ); }
		} _init_once;

	#endif

	






    //--------TcpSocket-----------------------------------------------------------------------------------------------------------------------


    struct CSocket 
		: public Socket
		//: public e6::Class < Socket, CTcpSocket >
    {
 	    SOCKET        me;
	    SOCKADDR_IN   address;

		CSocket::CSocket( uint type=SOCK_STREAM, uint proto=IPPROTO_TCP )
			: me(INVALID_SOCKET)
		{
			$X();
			me = socket (AF_INET, type, proto) ;
			if ( ( me ) == INVALID_SOCKET )
			{
				printf( "%s() error : couldn't create socket !", __FUNCTION__ );
			}
		}

		CSocket::CSocket(const SOCKET m, const  SOCKADDR_IN  & addr )
			: me(m)
			, address(addr)
		{
			$X();
		}

		virtual CSocket::~CSocket( )
		{
			$X();
			if ( me != INVALID_SOCKET )
				::shutdown( me, 2 );
		}

		static void dumpAddress( const SOCKADDR_IN & ad )
		{
			printf("[%s:%d]\n", inet_ntoa( ad.sin_addr ) , ntohs(ad.sin_port) );
		}

		static unsigned long addressFromString( const char * name )  
		{
			if ( ! name ) return INADDR_NONE;

			unsigned long i_addr = inet_addr( name );
			if ( i_addr == INADDR_NONE ) {   // else : it was already an address
				HOSTENT *hostentry  = gethostbyname( name );
				if ( hostentry )
					i_addr =  *(unsigned long *)hostentry->h_addr_list[0];
			}		
			if ( i_addr == INADDR_NONE )
			{
				printf( "%s() error : couldn't resolve hostname '%s' !", __FUNCTION__, name );
			}
			return i_addr;
		}


		uint setAddress( const char *hostname, uint port )
		{
			address.sin_family      =  AF_INET;
			address.sin_addr.s_addr =  addressFromString( hostname );
			address.sin_port        =  htons(port);
			return 1;
		}

		uint prepareServer( uint port, bool reuseAddress=false )
		{
			if ( reuseAddress  )
			{
				int optval = 1; 
				if ( ::setsockopt( me, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof (optval) ) == SOCKET_ERROR )
				{
					printf( "%s() error : couldn't setsockopt  for sock %x on port %d !", __FUNCTION__, me, port);
					return 0;
				}
			}

			address.sin_family      =  AF_INET;
			address.sin_addr.s_addr =  INADDR_ANY;
			address.sin_port        =  htons(port);

			if ( ::bind( me, (SOCKADDR*) &address, sizeof(SOCKADDR_IN) ) == SOCKET_ERROR )
			{
				printf( "%s() error : couldn't bind sock %x to port %d !", __FUNCTION__, me, port);
				return 0;
			}

			//if ( ::listen( me, 10 ) == SOCKET_ERROR )
			//{
			//	printf( "%s() error : couldn't listen on sock %x on port %d !", __FUNCTION__, me, port);
			//	return 0;
			//}
			return 1;
		}

		uint listen( uint port )
		{
			if ( ::listen( me, 10 ) == SOCKET_ERROR )
			{
				printf( "%s() error : couldn't listen on sock %x on port %d !", __FUNCTION__, me, port);
				return 0;
			}
			return 1;
		}

    };



	struct CTcpSocket 
		: public CSocket
		//: public e6::Class < Socket, CTcpSocket >
    {

		CTcpSocket::CTcpSocket( )
			: CSocket( SOCK_STREAM, IPPROTO_TCP)
		{
			$X();
		}

		CTcpSocket::CTcpSocket(const SOCKET m, const  SOCKADDR_IN  & addr )
			: CSocket(m,addr)
		{
			$X();
		}

		virtual CTcpSocket::~CTcpSocket( )
		{
		}

		virtual uint read(  char *buffer, uint numBytes )  
		{
			assert(me!=INVALID_SOCKET);
			buffer[0] = 0;
			int res = ::recv( me, buffer, numBytes, 0 );
			return (res!=SOCKET_ERROR) ? res : 0;
		}


		virtual uint write( const char *buffer, uint numBytes )  
		{
			assert(me!=INVALID_SOCKET);
			if ( numBytes )
			{
				return ::send( me, buffer, numBytes, 0 );
			}
			return 0;
		}


		uint connectClient( const char *hostname, uint port, Connection & connection )
		{
			CSocket::setAddress( hostname, port );

			int res = ::connect( me, (SOCKADDR*) &address, sizeof (SOCKADDR_IN) );
			if ( res ) // connect returns 0 on success !
			{
				printf( "%s() error : couldn't connect to '%s:%d' !", __FUNCTION__,hostname , port);
				return 0;
			}

			while ( connection.handle( *this ) )
			{
			}

			return 1; 
		}

		uint startServer(  uint port, Connection & connection )
		{

			CSocket::prepareServer( port, true );

			if ( ::listen( me, 10 ) == SOCKET_ERROR )
			{
				printf( "%s() error : couldn't listen on sock %x on port %d !", __FUNCTION__, me, port);
				return 0;
			}


			for ( ;; )
			{
				int addrlen = sizeof(SOCKADDR_IN);
				SOCKET client = ::accept( me,  (SOCKADDR*)&address, &addrlen );
				if ( client == SOCKET_ERROR )
				{
					printf( "%s() error : couldn't accept connection on sock %x on port %d !", __FUNCTION__, me, port);
					return 0;
				}

				if ( 1 ) // multithreaded
				{
					static int nThreads = 0;

					struct ClientThread : e6::sys::Thread, CTcpSocket
					{
						Connection & connection;

						ClientThread( uint client, SOCKADDR_IN & address, Connection & con ) 
							: CTcpSocket(client,address)
							, connection(con)
						{
							nThreads++;
							$X1(nThreads);
						}
						virtual ~ClientThread() 
						{
							nThreads--;
							$X1(nThreads);
						}
						virtual void run()
						{
							while ( connection.handle( *this ) )
							{
							}
							delete this;
						}
					};

					ClientThread *cli = new ClientThread(client,address,connection);
					cli->start();
				} 
				else // ! multithreaded
				{
					CTcpSocket cli(client,address);
					while ( connection.handle( cli ) )
					{
					}

				}
				//::shutdown( client, 2 );

			}

			return me!=SOCKET_ERROR;
		}


    };

    struct CTcpClient 
		: public e6::CName< TcpClient, CTcpClient >
		, e6::sys::Thread
    {
 		CTcpSocket sock;
		Connection * connection;
		uint port;
		char hostname[300];

		//! connects to host and call connection.handle()
        virtual void run( )
		{
			if ( this->connection )
			{
				this->sock.connectClient( this->hostname, this->port, *(this->connection) );
			}
		}

		//! proxy, run() starts the main thing.
        virtual uint connect( const char *hostname, uint port,  Connection & connection   )
		{
			strcpy( this->hostname, hostname );
			this->port = port;
			this->connection = &connection;
			e6::sys::Thread::start();
			return 1;
		}
    };



	
	struct CTcpServer 
		: public e6::CName< TcpServer, CTcpServer >
		, e6::sys::Thread
    {
		CTcpSocket sock;
		Connection * connection;
		uint port;

		CTcpServer()  : connection(0) {}
		~CTcpServer() {}

		// the connection is called on each accepted socket until connection.handle() returns false;
        virtual uint start( uint port, Connection & connection )
		{
			this->port = port;
			this->connection = &connection;

			e6::sys::Thread::start();
			return 1;
		}

		virtual void run()
		{
			if ( this->connection )
			{
				this->sock.startServer( this->port, *(this->connection) );
			}
		}

    }; // CTcpServer



    struct CUdpSocket 
		: public CSocket
		//: public e6::Class < Socket, CTcpSocket >
    {

		CUdpSocket::CUdpSocket( )
			: CSocket( SOCK_DGRAM, IPPROTO_UDP )
		{
			$X();
		}

		CUdpSocket::CUdpSocket(const SOCKET m, const  SOCKADDR_IN  & addr )
			: CSocket(m,addr)
		{
			$X();
		}

		virtual CUdpSocket::~CUdpSocket( )
		{
		}

		// bolck for 30 secs.
		bool udpDataReady(uint timeout=30000000) const
		{
			fd_set rread;
			FD_ZERO( &rread );
			FD_SET( me, &rread );	

			struct timeval to;
			memset( &to, 0, sizeof(to) );
			to.tv_usec = timeout;

			if ( select( 128, &rread, NULL, NULL, &to ) > 0 && FD_ISSET( me, &rread ) )
				return true;
			return false;
		}

		virtual uint read(  char *buffer, uint numBytes )  
		{
			assert(me!=INVALID_SOCKET);
			int addrlen = sizeof(SOCKADDR_IN);
			int res = 0;
			buffer[0] = 0;
			//if ( udpDataReady() )  // block
			{
				res = ::recvfrom ( me, buffer, numBytes, 0, (SOCKADDR*) &address, (ADDRPOINTER) &addrlen );
			}
			return (res!=SOCKET_ERROR) ? res : 0;
		}


		virtual uint write( const char *buffer, uint numBytes )  
		{
			assert(me!=INVALID_SOCKET);
			if ( numBytes )
			{
				return ::sendto ( me, buffer, numBytes, 0, (SOCKADDR*) &address, sizeof(SOCKADDR_IN) );
			}
			return 0;
		}

	}; // CUdpSocket



	struct UdpListener : CUdpSocket, e6::sys::Thread 
	{
		//! server
		uint start(PORT p)
		{
	//		setTimeout(1000000);
			if ( CSocket::prepareServer( p, true ) )
			{
				Thread::start();
				return 1;
			}
			return 0;
		}

		//! client
		uint start( const char *host, PORT port ) 
		{
	//		setTimeout(1000);
			if ( CSocket::setAddress( host, port ) )
			{
				Thread::start();
				return 1;
			}		
			return 0;
		}

		virtual ~UdpListener() 
		{
			 Thread::stop();
		}

		void setTimeout( int t )
		{
			struct timeval to;
    		memset( &to, 0, sizeof(to) );
			to.tv_usec = t;

			if ( ::setsockopt( me, SOL_SOCKET, SO_RCVTIMEO, (char *)&to, sizeof (to) ) == SOCKET_ERROR )
			{
			   printf( "couldn't set timeout - sockopts!\n" );
			}
		}

		//void run() 
		//{
		//	char in[MAXBUF];
		//	if ( udpDataReady() )
		//	{
		//		int l=CUdpSocket::read(in,MAXBUF-7);
		//		in[l] = 0;
		//		update( in, l );
		//	}
		//}
		//virtual void update( char *s, int len ) {}

	}; // UdpListener


	
	struct CUdpClient 
		: public e6::CName< UdpClient, CUdpClient >
		, UdpListener
    {
		Connection * connection;

		CUdpClient() 
			: connection(0) 
		{}

		virtual ~CUdpClient() 
		{
		}

		virtual uint write( const char *buffer, uint numBytes ) 
		{
			// E6_CRITICAL;
			return CUdpSocket::write( buffer, numBytes );
		}

		//! async
        virtual uint connect( const char *hostname, uint port,  Connection & connection   )
		{
			this->connection = &connection;
			return UdpListener::start( hostname, port );
		}

		virtual void run() 
		{
			while ( connection && udpDataReady() )  // block
			{
				uint r = connection->handle( *this );
				if ( ! r ) 
				{
					printf( __FUNCTION__ " connection->handle failed.\n" );
					return;
				}
			}
			printf( __FUNCTION__ "%x udpDataReady failed.\n",connection );
		}

    };

    struct CUdpServer 
		: public e6::CName< UdpServer, CUdpServer >
		, UdpListener
    {
		SOCKADDR_IN clients[16];
		Connection * connection;
		uint nClients;
		bool running;

		CUdpServer() 
			: connection(0) 
			, nClients(0)
			, running(0)
		{
			memset( clients, 0, 16*sizeof(SOCKADDR_IN) );
		}

		virtual ~CUdpServer() 
		{}

		bool isEqual(const SOCKADDR_IN & sa, const SOCKADDR_IN & sb) const
		{
			return
				(
				( sa.sin_port == sb.sin_port ) && 
				( sa.sin_addr.s_addr == sb.sin_addr.s_addr ) &&
				( sa.sin_family == sb.sin_family )
				);
		}
		uint isClient( const SOCKADDR_IN & sa ) const
		{
			for( uint i=0; i<16; i++ )
			{
				if ( isEqual( clients[i], sa ) )
					return true;
			}
			return false;
		}
		
		uint addClient( const SOCKADDR_IN & sa ) 
		{
			if ( isClient( sa ) )
				return 1;

			uint r = 0;
			for( uint i=0; i<16; i++ )
			{
				if ( clients[i].sin_port )
					continue;
				memcpy(&(clients[i]),&sa,sizeof(SOCKADDR_IN));
				nClients++;
				printf("+ %3d %3d\t", i, nClients );
				dumpAddress( clients[i] );
				return true;
			}
			printf(__FUNCTION__ " failed\t"  );
			dumpAddress( sa );
			return r;
		}
		uint remClient( uint i ) 
		{
			if( i < 16 )
			{
				printf("- %3d %3d\t", i, nClients-1 );
				dumpAddress( clients[i] );
				memset(&(clients[i]),0,sizeof(SOCKADDR_IN));
				nClients --;
				return true;
			}
			return false;
		}

		uint remClient( const SOCKADDR_IN & sa ) 
		{
			for( uint i=0; i<16; i++ )
			{
				if ( isEqual( clients[i], sa ) )
				{
					return remClient(i);
				}
			}
			return false;
		}

    	virtual uint  read(  char *buffer, uint numBytes ) 
		{
			assert(me!=INVALID_SOCKET);
			int addrlen = sizeof(SOCKADDR_IN);
			int res = 0;
			buffer[0] = 0;
			//if ( udpDataReady() )  // block
			{
				res = ::recvfrom ( me, buffer, numBytes, 0, (SOCKADDR*) &address, (ADDRPOINTER) &addrlen );
			}
			if ( res < 1 ) // received 0 bytes or error
			{
				remClient( this->address );
			}
			return (res>0?res:0);
		}

		virtual uint broadcast( const char *buffer, uint numBytes ) 
		{
			if ( ! running ) return 0;

			uint r = 0;
			for( uint i=0; i<16; i++ )
			{
				if ( ! clients[i].sin_port )
					continue;
				r = ::sendto( me, buffer, numBytes, 0, (SOCKADDR*)&(clients[i]), sizeof(SOCKADDR_IN) );
				if ( r < numBytes ) 
				{
					printf("broadcast failed %d / %d\n", r, numBytes );
					remClient( i );
					break;
				}
			}
			return r;
		}

        virtual uint start( uint port,  Connection & connection   )
		{
			this->connection = &connection;
			return UdpListener::start( port );
		}

		virtual void run() 
		{
			running = true;
			while ( connection && udpDataReady(1000*1000*60*8) )  // block for 8 min
			{
				uint r = connection->handle( *this );
				if ( ! r ) 
				{
					continue;
				}

				r = addClient( this->address );
				if ( ! r ) 
				{
					printf( __FUNCTION__ " addClient failed \t"  );
					dumpAddress( address );
					running = false;
					return;
				}

				Thread::sleep(5);
			}
			printf( __FUNCTION__ " udpDataReady failed.\n" );
			running = false;
		}

		//virtual uint stop() 
		//{
		//}

    }; // CUdpServer



	namespace Tuio
	{
		struct Parser
		{

		public:

			//struct Printer : Callback
			//{
			//	void call( int mode, Object &o )
			//	{
			//		switch(mode)
			//		{ 
			//			case 1 : 
			//				printf( "set obj %2i %2i [%3.3f %3.3f %3.3f] [%3.3f %3.3f %3.3f] [%3.3f %3.3f]\n",o.id,o.fi,o.x,o.y,o.a,o.X,o.Y,o.A,o.m,o.r );
			//				break;
			//			case 2 : 
			//				printf( "set cur %2i [%3.3f %3.3f] [%3.3f %3.3f] [%3.3f]\n",o.id,o.x,o.y,o.X,o.Y,o.m );
			//				break;
			//		}
			//	}
			//};
			
			
			int parse( char * data, int n, Callback * cb )
			{
				int k=0,mode=0;
				char * b = data;
				char * tok = b;
				while ( k < n )
				{
					if ( ! b[k] )
					{
						k ++;
						continue;
					}
					else
					if ( tok = strstr( &b[k], "/tuio/2Dobj" ) )
					{
						mode = 1;
						k += 12;
					}
					else
					if ( tok = strstr( &b[k], "/tuio/2Dcur" ) )
					{
						mode = 2;
						k += 12;
					}
					else
					if ( tok = strstr( &b[k], "set" ) )
					{
						k += 4;
						Object o = {0};
						switch( mode )
						{
							case 1 : // /tuio/2Dobj 
								k += parseSetObj( tok + 4, &o );
								if  ( cb ) cb->call(mode,o);
								break;
							case 2 : // /tuio/2Dcur 
								k += parseSetCur( tok + 4, &o );
								if  ( cb ) cb->call(mode,o);
								break;
							default:
								break;
						}
					}
					else
					if ( tok = strstr( &b[k], "fseq" ) )
					{
						int s = 0;
						k += 5; // "fseq" + '0'
						k += 3; // pad to 4byte boundary
						k += getInt( &b[k], s );
						if ( seq > s )
						{
							fprintf( stderr, "!!!seq  %d < %d\n", seq,s );
						}
						seq = s;
					}
					else
					{
						k ++;
					}
					
				}
				return n;
			}
			
			static void debug( char * b, int n, FILE * str=stderr )
			{
				for ( int i=0; i<n; i++ )
				{
					if( b[i]>0x20 && b[i]<'z' )
					{
						fprintf( str, " %c", b[i] );
					}
					else
					{
						fprintf( str, " %02x", b[i] );
					}
				}
				fprintf( str,  "\n" );
			}

			int getSequence()
			{
				return seq;
			}

		private:	
			int seq;
			
			int getString( char * data, char **output )
			{
				int l = strlen(data)+1;
				*output = data;
				return l;
			}

			//! big endian !
			int getFloat( char * data, float & output )
			{
				char d[4];
				d[0] = data[3];
				d[1] = data[2];
				d[2] = data[1];
				d[3] = data[0];
				float o = *((float*)(d));
				output = o;
				return 4;
			}

			//! big endian !
			int getInt( char * data, int & output )
			{
				int o =  (*(data  ) << 24)
					   + (*(data+1) << 16)
					   + (*(data+2) << 8)
					   + (*(data+3) );
				output = o;
				return 4;
			};

			int parseSetObj( char * data, Object *o )
			{
				int off = 0;
				off += getInt( data+off, o->id );
				off += getInt( data+off, o->fi );
				off += getFloat( data+off, o->x );
				off += getFloat( data+off, o->y );
				off += getFloat( data+off, o->a );
				off += getFloat( data+off, o->X );
				off += getFloat( data+off, o->Y );
				off += getFloat( data+off, o->A );
				off += getFloat( data+off, o->m );
				off += getFloat( data+off, o->r );
				//~ printf( "set obj %2i %2i [%3.3f %3.3f %3.3f] [%3.3f %3.3f %3.3f] [%3.3f %3.3f]\n",o->id,o->fi,o->x,o->y,o->a,o->X,o->Y,o->A,o->m,o->r );
				return off;
			}

			int parseSetCur( char * data, Object *o )
			{
				int off = 0;
				off += getInt( data+off, o->id );
				off += getFloat( data+off, o->x );
				off += getFloat( data+off, o->y );
				off += getFloat( data+off, o->X );
				off += getFloat( data+off, o->Y );
				off += getFloat( data+off, o->m );
				//~ printf( "set cur %2i [%3.3f %3.3f] [%3.3f %3.3f] [%3.3f]\n",o->id,o->x,o->y,o->X,o->Y,o->m );
				return off;
			}

		};
		
		struct CListener 
			: e6::CName< Net::Tuio::Listener, Net::Tuio::CListener >
			, Net::UdpListener 
			, Net::Tuio::Parser
		{
			Callback * cb;

			CListener()
				: cb(0)
			{}

			// Net::Tuio::Listener
			virtual uint start( uint port, Callback & kebab ) 
			{
				this->cb = &kebab;
				return UdpListener::start( port );
			}

			// Net::UdpListener
			virtual void run() 
			{
				char b[0xffff] = {0};
				while ( UdpListener::udpDataReady(1000*1000*60*20) )  // block 20 min
				{
					b[0]=0;
					int n = UdpListener::read(b,0xfffe);
					if ( n > 0 ) 
					{
						fprintf( stderr, "#fseq : %-5d %-3d\n", Parser::getSequence() , n );
						Parser::debug( b, n );
						Parser::parse( b, n, cb );
					}
				}
				printf("\nstopped.\n");
			}
		};
	
	} // Tuio

}; // Net


extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Net.TcpClient",	 "Net",	Net::CTcpClient::createInterface, Net::CTcpClient::classRef	},
		{	"Net.TcpServer",	 "Net",	Net::CTcpServer::createInterface, Net::CTcpServer::classRef	},
		{	"Net.UdpClient",	 "Net",	Net::CUdpClient::createInterface, Net::CUdpClient::classRef	},
		{	"Net.UdpServer",	 "Net",	Net::CUdpServer::createInterface, Net::CUdpServer::classRef	},
		{	"Net.Tuio.Listener", "Net",	Net::Tuio::CListener::createInterface, Net::Tuio::CListener::classRef	},
		//{	"Net.ScriptHost",	 "Net",	Net::CScriptHost::createInterface, Net::CScriptHost::classRef},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 5; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Net 00.000.0027 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
