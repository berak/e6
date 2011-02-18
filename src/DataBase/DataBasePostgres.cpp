
#include "DataBase.h"
#include "../e6/e6_impl.h"
#include "libpq-fe.h"



using e6::uint;
using e6::ClassInfo;


namespace DataBasePostgres
{
	class CConnection
		: public e6::CName< DataBase::Connection, CConnection >
	{	
	private:
	  
		PGconn     *conn;
		PGresult   *res;

		uint ncols;
		uint nrows;

	
		void _clearResult()
		{
		    PQclear(res);
			res = 0;

			this->ncols = this->nrows = 0;
		}


		uint _error( const char * fn, const char * txt="" )
		{
			fprintf(stderr, "error: %s", PQerrorMessage(conn));
//			close();
			return 0; 
		}


	public:
		CConnection() 
	        : conn(0)
			, res(0)
			, nrows(0)
			, ncols(0)
		{
	        // printf( "<sqlite %s>\n", sqlite3_libversion() );
		}

		~CConnection() 
		{
			close();
		}


		virtual uint open( const char * db, const char * host, const char * user, const char * pw ) 
		{
			char conninfo[666] = {0};
			if ( host )
			{
				strcat( conninfo, "host=" );
				strcat( conninfo, host );
				strcat( conninfo, " " );
			}
			if ( user )
			{
				strcat( conninfo, "user=" );
				strcat( conninfo, user );
				strcat( conninfo, " " );
			}
			if ( pw )
			{
				strcat( conninfo, "password=" );
				strcat( conninfo, pw );
				strcat( conninfo, " " );
			}
			if ( db )
			{
				strcat( conninfo, "dbname=" );
				strcat( conninfo, db );
			}

			this->conn = PQconnectdb(conninfo);
			if (this->conn == 0)
				return _error(0,0);
			return 1;
		}


		virtual uint query( const char * statement ) 
		{
			_clearResult();

			if (this->conn == 0)
				return _error(0,0);

			res = PQexec(this->conn, statement);
			if (this->res == 0)
				return _error(0,0);
			
		    this->ncols = PQnfields(this->res);
			this->nrows = PQntuples(this->res);
			return 1;
		}


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
			if ( ! res ) return 0;		   
			if ( col > this->ncols ) return 0;		   
			return PQfname(res, col);
		}


		virtual const char *getItem( uint row, uint col ) const 
		{
			if ( ! res ) return 0;		   
			if ( row > this->nrows ) return 0;
			if ( col > this->ncols ) return 0;
			return PQgetvalue( res, row, col );
		}


		virtual uint close() 
		{
			_clearResult();

			if ( conn )
				PQfinish(conn);
			conn = 0;
			return 1;
		}

	}; //CConnection


};


extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"DataBase.Connection",	 "DataBasePostgres",	DataBasePostgres::CConnection::createInterface, DataBasePostgres::CConnection::classRef	},
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
	mv->modVersion = ("DataBasePostgres 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
