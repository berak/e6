// psock.cpp 15:51 06.10.00

#include "sock.h"

namespace w32
{
	Socket::Socket( int t, int p ) {
	#ifdef _WIN32
		WSADATA w;
		WSAStartup( MAKEWORD( 2, 1 ), &w );
	#endif
		s_type  = t;
		s_proto = p;
	
		if ( ( me = socket (AF_INET, s_type, s_proto) ) == SOCKET_ERROR )
			die( "couldn't create socket !" );
	}
	
	Socket::~Socket() {
		close(me);
	}
	
	PORT Socket::getPort() {
		return htons(address.sin_port); 
	}
	
	unsigned long Socket::getAddress() {
		return htonl(address.sin_addr.s_addr); 
	}
	
	
	void Socket::setAddress( unsigned long a, PORT p ) {
		address.sin_family        = AF_INET;
		address.sin_addr.s_addr   = a;
		address.sin_port          = htons(p);
	}
	
	unsigned long Socket::addressFromString( char * name )  {
		unsigned long i_addr = inet_addr( name );
		if ( i_addr == INADDR_NONE ) {   // else : it was already an address
			HOSTENT *hostentry  = gethostbyname( name );
			if ( hostentry )
				i_addr =  *(unsigned long *)hostentry->h_addr_list[0];
		}		
		if ( i_addr == INADDR_NONE )
			die( "couldn't find host!" );
		return i_addr;
	}
	
	void Socket::close( SOCKET c ) {
		if ( c != INVALID_SOCKET )
			shutdown( c, 2 );
		c = INVALID_SOCKET;
	}
	
	void Socket::close() {
		close( me );
	}
	
	void Socket::die( char *t ) { 
		MessageBox(0,t,__FUNCTION__,0);
		
		close();
	#ifdef _WIN32
		int  e = WSAGetLastError();               
		if ( e > 10000 )                          
		{
			fprintf( stderr, "wsa-err:%d\n", e ); 
		}
		WSACleanup();
	#endif
		exit(-1); 
	}
	
	int Socket::read( SOCKET s, char *buf, int len ) {
		static int addrlen = sizeof(address);
		memset( buf, 0, len );       
		return ( ( s_type == SOCK_STREAM ) 
			   ? recv( s, buf, len, 0 )
			   : recvfrom ( s, buf, len, 0, (SOCKADDR*) &address, (ADDRPOINTER) &addrlen ) );
	}
	
	int Socket::write( SOCKET s, char *buf, int l ) {
		int len = ( l>0 ? l : strlen(buf) );
		return ( ( s_type == SOCK_STREAM )
			   ? send( s, buf, len, 0 )
			   : sendto ( s, buf, len, 0, (SOCKADDR*) &address, sizeof(SOCKADDR_IN) ) );
	} 
	
	int Socket::read( char *buf ) {
		return read( me, buf );
	}
	
	int Socket::write( char *buf ) {
		return write( me, buf );
	} 
	
	bool Socket::udpDataReady() {
		fd_set rread;
		FD_ZERO( &rread );
		FD_SET( me, &rread );	
	
		struct timeval to;
		memset( &to, 0, sizeof(to) );
		to.tv_usec = 1;
	
		if ( select( 1, &rread, NULL, NULL, &to ) > 0 && FD_ISSET( me, &rread ) )
			return true;
		return false;
	}
	
	SOCKET Socket::connect( char *hostname, PORT port ) {
		setAddress( addressFromString( hostname ), port );
		if ( s_proto == IPPROTO_TCP )
			if ( ::connect( me, (SOCKADDR*) &address, sizeof (SOCKADDR_IN) ) )
				die( "couldn't connect to host!" );
		return me;
	}  
	
	
	SOCKET Socket::listen( PORT port, u_long address ) {
		int optval = 1; 
		if ( setsockopt( me, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof (optval) ) == SOCKET_ERROR )
			die( "couldn't set sockopts!" );
		setAddress( INADDR_ANY, port );
		if ( bind( me, (SOCKADDR*) &address, sizeof(SOCKADDR_IN) ) == SOCKET_ERROR )
		{
			char err[300];
			sprintf( err, "couldn't bind %i %i !", address, port );
			die( err );
		}
		if ( s_proto == IPPROTO_TCP ) 
			if ( ::listen( me, 10 ) == SOCKET_ERROR )
				die( "couldn't listen !" );
		return me;
	}
	
	
	SOCKET Socket::accept() {
		int addrlen = sizeof(SOCKADDR);
		SOCKET con  = ::accept( me, (SOCKADDR*) &address, (ADDRPOINTER) &addrlen );
		if ( con == INVALID_SOCKET )
			die( "couldn't accept !" );
		return con;
	}
	
	void Socket::dump() { 
		printf( "-----------------------------------\n" );
		printf( "sock    : %x\n", me        );
		printf( "proto   : %x\n", s_proto   );
		printf( "type    : %x\n", s_type    );
		printf( "port    : %d\n", getPort() );
		printf( "address : %s\n", inet_ntoa( address.sin_addr ) );
	}
};