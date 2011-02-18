
#include "../e6/e6_impl.h"
#include "../e6/e6_str.h"
#include "DataBase.h"


//
// the largest hack in the world i've ever done .. but it work's !!!
//


using e6::uint;
using e6::ClassInfo;
#include <stdio.h>
#include <string.h>

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
	
	
	
	class Socket {
	protected:
		int           s_type;
		int           s_proto;
		SOCKET        me;
		SOCKADDR_IN   address;
	
		void          die( char *t );
		void          setAddress( u_long a, PORT p );
		
	public:
		Socket( int t=SOCK_STREAM, int p=IPPROTO_TCP );
		virtual ~Socket();
	
		SOCKET        connect( char *hostname, PORT port );
		SOCKET        listen( PORT port, u_long address=INADDR_ANY ) ;
		SOCKET        accept();	
		int           read( char *buf, int len );
		int           read( SOCKET s, char *buf, int len=MAXBUF );
		int           write( char *buf, int len );
		int           write( SOCKET s, char *buf, int l=0 );
		void          close();
		void          close( SOCKET c );
		void          dump();
		bool          udpDataReady();
		PORT          getPort();
		u_long        getAddress();
		u_long        addressFromString( char *hostString );
	};
	
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
	
	int Socket::read( char *buf, int len ) {
		return read( me, buf, len );
	}
	
	int Socket::write( char *buf, int len ) {
		return write( me, buf, len );
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
	
	


// mysql_defines.h Aug 18, 2006 UNSTABLE


// Copied from include/mysql_com.h [44-57]
enum enum_server_command
{
  COM_SLEEP, COM_QUIT, COM_INIT_DB, COM_QUERY, COM_FIELD_LIST,
  COM_CREATE_DB, COM_DROP_DB, COM_REFRESH, COM_SHUTDOWN, COM_STATISTICS,
  COM_PROCESS_INFO, COM_CONNECT, COM_PROCESS_KILL, COM_DEBUG, COM_PING,
  COM_TIME, COM_DELAYED_INSERT, COM_CHANGE_USER, COM_BINLOG_DUMP,
  COM_TABLE_DUMP, COM_CONNECT_OUT, COM_REGISTER_SLAVE,
  COM_STMT_PREPARE, COM_STMT_EXECUTE, COM_STMT_SEND_LONG_DATA, COM_STMT_CLOSE,
  COM_STMT_RESET, COM_SET_OPTION, COM_STMT_FETCH,
  /* don't forget to update const char *command_name[] in sql_parse.cc */

  /* Must be last */
  COM_END
};

// Copied from sql/sql_parse.cc [77-85] and capitalized and s/ /_/g by me
static const char *command_name[]={
   "SLEEP", "QUIT", "INIT_DB", "QUERY", "FIELD_LIST", "CREATE_DB",
   "DROP_DB", "REFRESH", "SHUTDOWN", "STATISTICS", "PROCESSLIST",
   "CONNECT","KILL","DEBUG","PING","TIME","DELAYED_INSERT","CHANGE_USER",
   "Binlog Dump","Table Dump",  "Connect Out", "Register Slave",
   "PREPARE", "EXECUTE", "LONG_DATA", "CLOSE_STMT",
   "RESET_STMT", "Set option", "FETCH",
   "Error"                                       // Last command number
};

// Copied from include/mysql_com.h [113-130]
#define CLIENT_LONG_PASSWORD    1       /* new more secure passwords */
#define CLIENT_FOUND_ROWS       2       /* Found instead of affected rows */
#define CLIENT_LONG_FLAG        4       /* Get all column flags */
#define CLIENT_CONNECT_WITH_DB  8       /* One can specify db on connect */
#define CLIENT_NO_SCHEMA        16      /* Don't allow database.table.column */
#define CLIENT_COMPRESS         32      /* Can use compression protocol */
#define CLIENT_ODBC             64      /* Odbc client */
#define CLIENT_LOCAL_FILES      128     /* Can use LOAD DATA LOCAL */
#define CLIENT_IGNORE_SPACE     256     /* Ignore spaces before '(' */
#define CLIENT_PROTOCOL_41      512     /* New 4.1 protocol */
#define CLIENT_INTERACTIVE      1024    /* This is an interactive client */
#define CLIENT_SSL              2048    /* Switch to SSL after handshake */
#define CLIENT_IGNORE_SIGPIPE   4096    /* IGNORE sigpipes */
#define CLIENT_TRANSACTIONS     8192    /* Client knows about transactions */
#define CLIENT_RESERVED         16384   /* Old flag for 4.1 protocol  */
#define CLIENT_SECURE_CONNECTION 32768  /* New 4.1 authentication */
#define CLIENT_MULTI_STATEMENTS 65536   /* Enable/disable multi-stmt support */
#define CLIENT_MULTI_RESULTS    131072  /* Enable/disable multi-results */


static const char *caps_name[]={
	"LONG_PASSWORD",//    1       /* new more secure passwords */
	"FOUND_ROWS",//       2       /* Found instead of affected rows */
	"LONG_FLAG",//        4       /* Get all column flags */
	"CONNECT_WITH_DB",//  8       /* One can specify db on connect */
	"NO_SCHEMA",//        16      /* Don't allow database.table.column */
	"COMPRESS",//         32      /* Can use compression protocol */
	"ODBC",//             64      /* Odbc client */
	"LOCAL_FILES",//      128     /* Can use LOAD DATA LOCAL */
	"IGNORE_SPACE",//     256     /* Ignore spaces before '(' */
	"PROTOCOL_41",//      512     /* New 4.1 protocol */
	"INTERACTIVE",//      1024    /* This is an interactive client */
	"SSL",//              2048    /* Switch to SSL after handshake */
	"IGNORE_SIGPIPE",//   4096    /* IGNORE sigpipes */
	"TRANSACTIONS",//     8192    /* Client knows about transactions */
	"RESERVED",//         16384   /* Old flag for 4.1 protocol  */
	"SECURE_CONNECTION",// 32768  /* New 4.1 authentication */
	"MULTI_STATEMENTS",// 65536   /* Enable/disable multi-stmt support */
	"MULTI_RESULTS" //    131072  /* Enable/disable multi-results */
};

//
//
//// Copied from include/mysql_com.h [133-151]
//#define SERVER_STATUS_IN_TRANS     1    /* Transaction has started */
//#define SERVER_STATUS_AUTOCOMMIT   2    /* Server in auto_commit mode */
//#define SERVER_STATUS_MORE_RESULTS 4    /* More results on server */
//#define SERVER_MORE_RESULTS_EXISTS 8    /* Multi query - next query exists */
//#define SERVER_QUERY_NO_GOOD_INDEX_USED 16
//#define SERVER_QUERY_NO_INDEX_USED      32
///*
//  The server was able to fulfill the clients request and opened a
//  read-only non-scrollable cursor for a query. This flag comes
//  in reply to COM_STMT_EXECUTE and COM_STMT_FETCH commands.
//*/
//#define SERVER_STATUS_CURSOR_EXISTS 64
///*
//  This flag is sent when a read-only cursor is exhausted, in reply to
//  COM_STMT_FETCH command.
//*/
//#define SERVER_STATUS_LAST_ROW_SENT 128
//#define SERVER_STATUS_DB_DROPPED        256 /* A database was dropped */
//#define SERVER_STATUS_NO_BACKSLASH_ESCAPES 512
//
//// Copied from sql/mysql_com.h [71-86] v5.0.15
//#define NOT_NULL_FLAG   1               /* Field can't be NULL */
//#define PRI_KEY_FLAG    2               /* Field is part of a primary key */
//#define UNIQUE_KEY_FLAG 4               /* Field is part of a unique key */
//#define MULTIPLE_KEY_FLAG 8             /* Field is part of a key */
//#define BLOB_FLAG       16              /* Field is a blob */
//#define UNSIGNED_FLAG   32              /* Field is unsigned */
//#define ZEROFILL_FLAG   64              /* Field is zerofill */
//#define BINARY_FLAG     128             /* Field is binary   */
///* The following are only sent to new clients */
//#define ENUM_FLAG       256             /* field is an enum */
//#define AUTO_INCREMENT_FLAG 512         /* field is a autoincrement field */
//#define TIMESTAMP_FLAG  1024            /* Field is a timestamp */
//#define SET_FLAG        2048            /* field is a set */
//#define NO_DEFAULT_VALUE_FLAG 4096      /* Field doesn't have default value */
//#define NUM_FLAG        32768           /* Field is num (for clients) */
//
//// Copied part of include/mysql_com.h [212-232]
//enum enum_field_types {
//                         MYSQL_TYPE_NEWDECIMAL=246,
//                         MYSQL_TYPE_ENUM=247,
//                         MYSQL_TYPE_SET=248,
//                         MYSQL_TYPE_TINY_BLOB=249,
//                         MYSQL_TYPE_MEDIUM_BLOB=250,
//                         MYSQL_TYPE_LONG_BLOB=251,
//                         MYSQL_TYPE_BLOB=252,
//                         MYSQL_TYPE_VAR_STRING=253,
//                         MYSQL_TYPE_STRING=254,
//                         MYSQL_TYPE_GEOMETRY=255
//};
//
//
//// Not from MySQL source code
//static const char *field_type_names[] = {
//   "decimal",
//   "tiny int",
//   "short int",
//   "long int",
//   "float",
//   "double",
//   "NULL",
//   "timestamp",
//   "longlong",
//   "int24",
//   "date",
//   "time",
//   "datetime",
//   "year",
//   "newdate",
//   "varchar",
//   "bit"
//};
//


typedef unsigned int	uint32_t;		//    unsigned 32 bit integer
typedef unsigned char	uint8_t;		//    unsigned 8 bit integer (i.e., unsigned char)
typedef unsigned int	int_least16_t;	//    integer of >= 16 bits

#ifndef _SHA_enum_
#define _SHA_enum_
enum
{
    shaSuccess = 0,
    shaNull,            /* Null pointer parameter */
    shaInputTooLong,    /* input data too long */
    shaStateError       /* called Input after Result */
};
#endif
#define SHA1HashSize 20

/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct SHA1Context
{
    uint32_t Intermediate_Hash[SHA1HashSize/4]; /* Message Digest  */

    uint32_t Length_Low;            /* Message length in bits      */
    uint32_t Length_High;           /* Message length in bits      */

                               /* Index into message block array   */
    int_least16_t Message_Block_Index;
    uint8_t Message_Block[64];      /* 512-bit message blocks      */

    int Computed;               /* Is the digest computed?         */
    int Corrupted;             /* Is the message digest corrupted? */
} SHA1Context;

/*
 *  Function Prototypes
 */

int SHA1Reset(  SHA1Context *);
int SHA1Input(  SHA1Context *,
                const uint8_t *,
                unsigned int);
int SHA1Result( SHA1Context *,
                uint8_t Message_Digest[SHA1HashSize]);



/*
 *  Define the SHA1 circular left shift macro
 */
#define SHA1CircularShift(bits,word) \
                (((word) << (bits)) | ((word) >> (32-(bits))))

/* Local Function Prototyptes */
void SHA1PadMessage(SHA1Context *);
void SHA1ProcessMessageBlock(SHA1Context *);

/*
 *  SHA1Reset
 *
 *  Description:
 *      This function will initialize the SHA1Context in preparation
 *      for computing a new SHA1 message digest.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to reset.
 *
 *  Returns:
 *      sha Error Code.
 *
 */
int SHA1Reset(SHA1Context *context)
{
    if (!context)
    {
        return shaNull;
    }

    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Intermediate_Hash[0]   = 0x67452301;
    context->Intermediate_Hash[1]   = 0xEFCDAB89;
    context->Intermediate_Hash[2]   = 0x98BADCFE;
    context->Intermediate_Hash[3]   = 0x10325476;
    context->Intermediate_Hash[4]   = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;

    return shaSuccess;
}

/*
 *  SHA1Result
 *
 *  Description:
 *      This function will return the 160-bit message digest into the
 *      Message_Digest array  provided by the caller.
 *      NOTE: The first octet of hash is stored in the 0th element,
 *            the last octet of hash in the 19th element.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to use to calculate the SHA-1 hash.
 *      Message_Digest: [out]
 *          Where the digest is returned.
 *
 *  Returns:
 *      sha Error Code.
 *
 */
int SHA1Result( SHA1Context *context,
                uint8_t Message_Digest[SHA1HashSize])
{
    int i;

    if (!context || !Message_Digest)
    {
        return shaNull;
    }

    if (context->Corrupted)
    {
        return context->Corrupted;
    }

    if (!context->Computed)
    {
        SHA1PadMessage(context);
        for(i=0; i<64; ++i)
        {
            /* message may be sensitive, clear it out */
            context->Message_Block[i] = 0;
        }
        context->Length_Low = 0;    /* and clear length */
        context->Length_High = 0;
        context->Computed = 1;

    }

    for(i = 0; i < SHA1HashSize; ++i)
    {
        Message_Digest[i] = context->Intermediate_Hash[i>>2]
                            >> 8 * ( 3 - ( i & 0x03 ) );
    }

    return shaSuccess;
}

/*
 *  SHA1Input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion
 *      of the message.
 *
 *  Parameters:
 *      context: [in/out]
 *          The SHA context to update
 *      message_array: [in]
 *          An array of characters representing the next portion of
 *          the message.
 *      length: [in]
 *          The length of the message in message_array
 *
 *  Returns:
 *      sha Error Code.
 *
 */
int SHA1Input(    SHA1Context    *context,
                  const uint8_t  *message_array,
                  unsigned       length)
{
    if (!length)
    {
        return shaSuccess;
    }

    if (!context || !message_array)
    {
        return shaNull;
    }

    if (context->Computed)
    {
        context->Corrupted = shaStateError;

        return shaStateError;
    }

    if (context->Corrupted)
    {
         return context->Corrupted;
    }
    while(length-- && !context->Corrupted)
    {
    context->Message_Block[context->Message_Block_Index++] =
                    (*message_array & 0xFF);

    context->Length_Low += 8;
    if (context->Length_Low == 0)
    {
        context->Length_High++;
        if (context->Length_High == 0)
        {
            /* Message is too long */
            context->Corrupted = 1;
        }
    }

    if (context->Message_Block_Index == 64)
    {
        SHA1ProcessMessageBlock(context);
    }

    message_array++;
    }

    return shaSuccess;
}

/*
 *  SHA1ProcessMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:

 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the
 *      names used in the publication.
 *
 *
 */
void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const uint32_t K[] =    {       /* Constants defined in SHA-1   */
                            0x5A827999,
                            0x6ED9EBA1,
                            0x8F1BBCDC,
                            0xCA62C1D6
                            };
    int           t;                 /* Loop counter                */
    uint32_t      temp;              /* Temporary word value        */
    uint32_t      W[80];             /* Word sequence               */
    uint32_t      A, B, C, D, E;     /* Word buffers                */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = context->Message_Block[t * 4] << 24;
        W[t] |= context->Message_Block[t * 4 + 1] << 16;
        W[t] |= context->Message_Block[t * 4 + 2] << 8;
        W[t] |= context->Message_Block[t * 4 + 3];
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);

        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;

    context->Message_Block_Index = 0;
}

/*
 *  SHA1PadMessage
 *

 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call the ProcessMessageBlock function
 *      provided appropriately.  When it returns, it can be assumed that
 *      the message digest has been computed.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to pad
 *      ProcessMessageBlock: [in]
 *          The appropriate SHA*ProcessMessageBlock function
 *  Returns:
 *      Nothing.
 *
 */

void SHA1PadMessage(SHA1Context *context)
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (context->Message_Block_Index > 55)
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56)
        {

            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message length as the last 8 octets
     */
    context->Message_Block[56] = context->Length_High >> 24;
    context->Message_Block[57] = context->Length_High >> 16;
    context->Message_Block[58] = context->Length_High >> 8;
    context->Message_Block[59] = context->Length_High;
    context->Message_Block[60] = context->Length_Low >> 24;
    context->Message_Block[61] = context->Length_Low >> 16;
    context->Message_Block[62] = context->Length_Low >> 8;
    context->Message_Block[63] = context->Length_Low;

    SHA1ProcessMessageBlock(context);
}


namespace DataBaseMysql
{
	struct ResultRow
	{
		uint offsets[32];
		//char line[512];
		e6::CString line;

		void clear()
		{
			memset( offsets, 0, 32*sizeof(int) );
			line.set(0);
		}

		void getItem( uint i, char * buf ) const
		{
			int off = offsets[i];
			int len = offsets[i+1] - off - 1;
			strncpy( buf, line.get() + off, len );
			buf[len]=0;
		}
	};

	class CConnection
		: public e6::CName< DataBase::Connection, CConnection >
	{

	private:

		enum { BUFFER_SIZE = 16 * 1024 };
		char buf[BUFFER_SIZE];
	
		char fields[32][128];
		ResultRow rows[32];

		Socket sock;
		uint nrows;		
		uint ncols;		

		int verbose;



		bool connect( const char * server, const char * user, const char * password, const char *db ) 
		{
			PORT port = 3306;
			if ( ! sock.connect( (char*)server, port ) )
			{
				return false;
			}
			
			if( verbose )
			{
				fprintf(stderr," .  connected to %s %i.\n",server, port );
			}

			char scramble[21] = {0};
			unsigned caps = 0;
			if ( ! parseGreeting( caps, scramble ) )
			{
				fprintf(stderr, "\nerrrr server init failed <%s/>\n", server);
				return false;
			}

			char * _db = 0;
			if ( caps & 8 ) // CLIENT_CONNECT_WITH_DB
			{
				_db = (char*)db;
			} 
			// else we do it after auth.
			
			if ( ! auth( user, password, _db, scramble ) )
			{
				fprintf(stderr, "\nerrrr client auth failed <%s/%s>\n",user, password );
				return false;
			}

			// if the caps didn't support connecting with a db, do it now:
			if ( ! (caps & 8) ) 
			{
				if ( ! processCmd( COM_INIT_DB, db, "init" ) )
				{
					fprintf(stderr, "\nerrrr set_db failed <%s>\n",db );
					return false;
				}
			}
			return true;		
		}

		void printCaps( int caps, char *txt )
		{
			fprintf( stderr, "\t%s\t%p", txt, caps );
			for ( int i=0; i<16; i++ ) 
			{
				if ( caps & (1<<i) )
					fprintf( stderr, " %s ", caps_name[i] );
			}
			fprintf( stderr, "\n");
		}



		bool queryResponse()
		{
			Sleep(50);
			size_t nbytes = 512;
			char _p[512] = { 0 };
			int ok = readPacket( _p, nbytes );
			if ( ! ok )
			{
				fprintf( stderr, "Packet error.\n" );
				return 0;
			}
			//fprintf( stderr, "Packet type %x.\n", _p[0] );
			switch ( (unsigned char)_p[0] )
			{
				case 0: // NO_ERR: 
					if( verbose )
						fprintf(stderr,"Ok.\n");
					break;
				case 0xfe : // EOF
					if( verbose )
						fprintf( stderr, "Eof.\n" );
					break;
				case 0xff : // ERR
					{
						uint e = _p[1] + (_p[2]>>0x8);
						fprintf( stderr, "Error %p: %s\n", e, &_p[3] );
					}
					return 0;;
				default:		
					//fprintf( stderr, "ResultSet.\n" );
					return queryResultSet( _p );
			}
			return 1;
		}

		void clearResultSet()
		{
			for ( uint i=0; i<this->ncols; i++ )
			{
				this->fields[i][0] = 0;
			}
			for ( uint i=0; i<this->nrows; i++ )
			{
				//this->rows[i].line.set(0);
				this->rows[i].clear();
			}
			this->nrows = this->ncols = 0;
		}


		void procField( char* packet, uint nb, int col)
		{		
			int off = 0;
			off += 1 + packet[off]; // def
			off += 1 + packet[off]; // dbname
			off += 1 + packet[off]; // tablename
			off += 1 + packet[off]; // tablename2
			int len = packet[off];

			strncpy( this->fields[col], packet + off + 1, len );
			this->fields[col][len] = 0; // terminate

			//printf( "field %i %s.\n", col, this->fields[col] );
		}
		

		void procRow( char* packet, uint nb, int row)
		{		
			//memcpy( this->rows[ row ].line , packet, nb );
			this->rows[ row ].line.set( packet, nb );
			uint off = 1;
			this->rows[ row ].offsets[ 0 ] = off;
			for ( uint i=0; i<this->ncols; i++ )
			{
				uint len = packet[off-1];
				off += len + 1;
				this->rows[ row ].offsets[ i+1 ] = off ;
			}
		}
		

		bool queryResultSet( char * _p )
		{
			this->nrows = 0;
			this->ncols = _p[0]; 

			// get field  packets:
			int col = 0;
			while ( 1 )
			{
				size_t nb=400;
				unsigned char p[400];
				bool ok = readPacket( (char*)p, nb );
				if ( ! ok || p[0] == 0xfe )
					break;
				procField((char*)p,nb, col++);
			}
			// get row  packets:
			int row = 0;
			while ( 1 )
			{
				size_t nb=400;
				unsigned char p[400];
				bool ok = readPacket( (char*)p, nb );
				if ( ! ok || p[0] == 0xfe )
					break;
				procRow((char*)p, nb, row++);
			}
			this->nrows = row;
			return 1;
		}
		
		bool parseGreeting(unsigned & caps, char * scramble )
		{
			size_t nb=200;
			char bytes[200], *b=bytes;

			if ( ! readPacket( bytes, nb ) )
			{
				return false;
			}

			if ( b[0] != 0xa ) // Version
			{
				fprintf(stderr, "\nerrrr invalid server init <%i  != 0x0a>\n",b[0] );
				return false;
			}
			b ++;         // cmd

			char sv[200], *s=sv;
			while( *b )
			{
				*s++ = *b++;
			}
			*s = 0;
			b++;          // term. zero
			
			unsigned thread_id = g4(b);
			b += 4;		// thread_id
			for ( int s=0; s<8; s++ )
			{
				*scramble++ = *b++;
			}
			//b += 8;		// scramble
			b += 1;		// filler

			caps = g2(b);
			b += 2;		// caps

			unsigned char lang = *(b);
			b += 1;		// lang

			unsigned status  = g2(b);
			b += 2;       // status

			b += 13;    // filler
			for ( int s=0; s<12; s++ )
			{
				*scramble++ = *b++;
			}
			//b += 8;		// scramble

			if( verbose )
			{
				fprintf( stderr, "%s\n", sv );
				fprintf( stderr, "\tthread\t%p\n", thread_id );
				fprintf( stderr, "\tlang\t%p\n", lang );
				fprintf( stderr, "\tstatus\t%p\n", status );
				printCaps( caps, "server" );
			}

			return true;
		}

		
		bool auth( const char * user, const char * pass, char * db , char * scramble )
		{	
			char _p[512] = {
				//client_flags
				0,0,0,0, //0x85, 0xa6, 0x03, 0x00,
				//max_packet_size
				0x00, 0x00, 0x00, 0x01,
				// charset_number
				0x08,                         
				// (filler)
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			};

			long flag = 0
					  | 1	 // LONG_PASSWORD
					  | 2	 // Found instead of affected rows  
					  | 4	 // LONG_FLAG
					  // | 8    // One can specify db on connect 
					  | 512  // PROTOCOL_41
					  | 1024 // INTERACTIVE
					  | 8192    // transactions
					  | 65536 // Multi-statement support
					  | 131072 //Multi-results 
						;
			if ( db )
			{
				flag |= 8;     // We specify db on connect 
			}

			if( verbose )
			{
				printCaps( flag, "client" );
			}

			s4( _p, flag );
			
			int nbytes = 32;
			strcpy( _p+nbytes, user );
			nbytes += strlen(user);

			_p[nbytes] = 0;
			nbytes += 1;

			if ( pass && pass[0] )
			{
				// hmmm, error in the spec ?
				//
				//			_p[nbytes] = 20;
				//			nbytes += 1;

				cryptPassword( pass, scramble, _p+nbytes );
				nbytes += 20;
			}
			// else ( no password )
			{
				_p[nbytes] = 0;
				nbytes += 1;
			}



			
			if ( db )
			{
				strcpy( _p+nbytes, db );
				nbytes += strlen(db);
			}
			_p[nbytes] = 0;
			nbytes += 1;

			int ok = sendPacket( _p, nbytes );

			Sleep(50);
			if ( ok && ! queryResponse() )
			{
				fprintf(stderr, "\nerrrr client auth response failed <%s/%s>\n",user, pass );
				return false;
			}

			return ok;
		}

		void printHex( const uint8_t * b, const char * pre )
		{
			char _dig_vec_upper[] =  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			printf( "%s ", pre  );
			for ( int i=0; i<20; i++ )
			{
				uint8_t lo = _dig_vec_upper[((uint8_t) b[i]) >> 4];
				uint8_t hi = _dig_vec_upper[((uint8_t) b[i]) & 0x0F];
				printf( "%c%c", lo,hi  );
			}
			printf( "\n");
		}
		bool cryptPassword(const char * pass, const char * scramble, char * output )
		{
			uint8_t hash_stage1[SHA1HashSize];
			uint8_t hash_stage2[SHA1HashSize];

			SHA1Context sha;

			SHA1Reset(&sha);
			SHA1Input(&sha, (uint8_t*)pass, strlen(pass) );
			SHA1Result(&sha, hash_stage1 );
			//printHex( hash_stage1, "STAGE1 " );
			//if ( verbosity > 3 ) printHex( (uint8_t*)scramble, "SCRAMBLE" );

			SHA1Reset(&sha);
			SHA1Input(&sha, hash_stage1, SHA1HashSize );
			SHA1Result(&sha, hash_stage2 );
			//if ( verbosity > 3 ) printHex( hash_stage2, "STAGE2  " );

			SHA1Reset(&sha);
			SHA1Input(&sha, (uint8_t*)scramble, SHA1HashSize );
			SHA1Input(&sha, hash_stage2, SHA1HashSize );
			SHA1Result(&sha, (uint8_t*)output );
			//if ( verbosity > 3 ) printHex( (uint8_t*)output, "STAGE3  " );
			
			for ( int i=0; i<20; i++ )
				output[i] ^= hash_stage1[i];

			//if ( verbosity > 3 ) printHex( (uint8_t*)output, "STAGE4  " );
			return 1;
		}
		
		bool processCmd( unsigned char cmd, const char * str, const char * txt )
		{
			char _p[512] = { cmd, 0 };

			int nbytes = 1;
			if ( str )
			{
				nbytes += strlen(str);
				strcpy( _p+1, str );
			}

			int ok = sendPacket( _p, nbytes, 0 );

			if ( ok && ! queryResponse() )
			{
				fprintf(stderr, "\nerrrr cmd[%02x] %s(%s) failed.", cmd, txt,str );
				return false;
			}

			if( verbose )
				fprintf(stderr, "\nok.\tcmd[%02x] %s(%s).", cmd, txt,str );
			return ok;
		}



		bool sendPacket( const char * bytes, size_t len, int packetID=-1 )
		{		
			assert( len < BUFFER_SIZE-4 );
			memset( buf+4, 0, len );
			
			s3( buf, len );
			
			if (packetID==-1)
			{
				buf[3] ++;  // increase packet id
			}
			else
			if (packetID >= 0) 
			{
				buf[3] = packetID;
			}
				
			memcpy( buf + 4, bytes, len );

			if( verbose )
			{
				dumpPacket(">");
			}

			int res = sock.write( buf, len + 4 );
			if ( res == SOCKET_ERROR ) 
			{
				fprintf(stderr, "\nerrrr write failed <%i>\n",len );
				return false;
			}
			return true;		
		}

		bool readPacket( char * bytes, size_t & len )
		{		
			memset( buf, 0, BUFFER_SIZE );
			int res = 0;
			res = sock.read(buf,4);	
			if ( res == SOCKET_ERROR ) 
			{
				fprintf(stderr, "\nerrrr read failed on header <%i>\n", len );
				return 0;
			}
			len = g3(buf);

			if ( ! len )
			{
				fprintf(stderr, "\nerrrr read failed : no user data <%i>\n",len );
				return 0;
			}
			res = sock.read(buf+4,len);	
			if ( res == SOCKET_ERROR ) 
			{
				fprintf(stderr, "\nerrrr read failed on data <%i>\n", len );
				return 0;
			}

			if( verbose )
			{
				dumpPacket("<");
			}

			memcpy( bytes, buf + 4, len );
			
			return true;		
		}

		bool dumpPacket( char * pre )
		{		
			int len = g3(buf);
			fprintf(stderr, "\n%s [%i/%i]\n",pre, len,buf[3] );
			int r=0;
			for ( r=0; r<4; r++ )
			{
				fprintf(stderr,"%02x ",(unsigned char)(buf[r]) );
			}
			if ( (unsigned char)(buf[4]) < 16 && *pre=='>')
			{
				fprintf(stderr,"\t%s",command_name[ (unsigned char)buf[4] ]  );
			}
			char b1[600],b2[600], b[10];;
			b1[0]=0; b2[0]=0; b[0]=0;
			for ( r=0; r<len; r++ )
			{
				if ( r%8 == 0 ) 
				{
					while (strlen(b1)<26) strcat(b1," ");
					fprintf(stderr,"%s\t%s\n", b1, b2);
					b1[0]=0; b2[0]=0; b[0]=0;
				}
				int c = (unsigned char)(buf[r+4]);
				sprintf(b, "%02x ",c );
				strcat( b1, b );
				if ( (c<' ') || (c>'z') )
				{
					c='.';
				}
				sprintf(b, "%c ", c);
				strcat( b2, b );
			}
			{
				while (strlen(b1)<26) strcat(b1," ");
				fprintf(stderr,"%s\t%s\n", b1, b2);
				b1[0]=0; b2[0]=0; b[0]=0;
			}
			
			return true;		
		}

		//
		// helpers to pack uints from/to bytestream:
		// warning : they're all litte endian!
		//
		unsigned g2( char *b )
		{
			 return  ((unsigned char)(*(b)))
				   + ((unsigned char)(*(b+1)) << 8 );
		}
		unsigned g3( char *b )
		{
			 return  ((unsigned char)(*(b)))
				   + ((unsigned char)(*(b+1)) << 8 )
				   + ((unsigned char)(*(b+2)) << 16 );
		}
		unsigned g4( char *b )
		{
			 return  ((unsigned char)(*(b)))
				   + ((unsigned char)(*(b+1)) << 8 )
				   + ((unsigned char)(*(b+2)) << 16 )
				   + ((unsigned char)(*(b+3)) << 24 );
		}

		void s2( char * _p, unsigned i )
		{
			_p[0] =   i & 0xff;
			_p[1] = ( i & 0xff00    ) >> 8;
		}
		void s3( char * _p, unsigned i )
		{
			_p[0] =   i & 0xff;
			_p[1] = ( i & 0xff00    ) >> 8;
			_p[2] = ( i & 0xff0000  ) >> 16;
		}
		void s4( char * _p, unsigned i )
		{
			_p[0] =   i & 0xff;
			_p[1] = ( i & 0xff00    ) >> 8;
			_p[2] = ( i & 0xff0000  ) >> 16;
			_p[3] = ( i & 0xff000000) >> 24;
		}

		char * lestr( const char * _s )
		{
			static char s[300];
			s[0] = strlen( _s );
			strncpy( s+1, _s, 298 ); 
			return s;
		}

	public:

		CConnection()
			: nrows(0)
			, ncols(0)
			, verbose(0)
		{}

		~CConnection()
		{
			close();
		}

		//
		// DataBase::Connection impl:
		//
		virtual uint open( const char * db, const char * host, const char * user, const char * pw ) 
		{
			return this->connect( host, user, pw, db );
		}
		virtual uint close() 
		{
			return this->sendPacket( "\1", 1 ); // no response.
		}
		virtual uint query( const char * statement ) 
		{
			return this->processCmd( COM_QUERY, statement, "query" );
		}

		// result:
		virtual uint numCols() const 
		{
			return this->ncols;
		}
		virtual uint numRows() const 
		{
			return this->nrows;
		}
		virtual const char *colName( uint col ) const 
		{
			return this->fields[ col ];
		}
		virtual const char *getItem( uint row, uint col ) const 
		{
			static char res[200];
			res[0]=0;
			this->rows[ row ].getItem( col, res );
			return res;
		}

	}; // CConnection

}; // DataBaseMysql


extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"DataBase.Connection",	 "DataBaseMysql",	DataBaseMysql::CConnection::createInterface, DataBaseMysql::CConnection::classRef	},
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
	mv->modVersion = ("DataBaseMysql 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
