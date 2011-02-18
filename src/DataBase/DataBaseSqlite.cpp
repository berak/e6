#include <stdio.h>

#include "../e6/e6_impl.h"
#include "DataBase.h"
#include "sqlite3.h"




using e6::uint;
using e6::ClassInfo;


namespace DataBaseSqlite
{

	class CConnection
		: public e6::CName< DataBase::Connection, CConnection >
	{
	private:
	    sqlite3 * sqlite;

		int ncols;
		int nrows;
		char ** cbuf;

		void _clearResult()
		{
			this->ncols = this->nrows = 0;
			if ( this->cbuf )
				sqlite3_free_table( this->cbuf );
			this->cbuf = 0;
		}
		uint _error( const char * fn, const char * txt="" )
		{
			const char * err = sqlite3_errmsg( this->sqlite );
			printf( "%s(%s) : error( %s )\n", fn, txt, err );
			return 0; 
		}

	public:
		CConnection() 
	        : sqlite(0)
			, cbuf(0)
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
			if ( this->sqlite )
			{
				return _error(__FUNCTION__, "close running connection first");
			}
			int r = sqlite3_open( db, &this->sqlite );
			if ( r != SQLITE_OK )
			{
				return _error(__FUNCTION__);
			}
			return 1; 
		}

		virtual uint query( const char * statement ) 
		{
	        if ( ! this->sqlite ) 
			{
				return _error(__FUNCTION__,"DB_CLOSED");
			}

			_clearResult();

			char * msg = 0;
			int r = sqlite3_get_table( this->sqlite, statement, &this->cbuf, &this->nrows, &this->ncols, &msg );
			if ( r != SQLITE_OK )
			{
				return _error(__FUNCTION__);
			}
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
			if ( ! this->cbuf ) return 0;
			if ( (int)col >= this->ncols ) return 0;
			return this->cbuf[col];
		}
		virtual const char *getItem( uint row, uint col ) const 
		{
			if ( ! this->cbuf ) return 0;
			if ( (int)row >= this->nrows ) return 0;
			if ( (int)col >= this->ncols ) return 0;
			return this->cbuf[(row+1)*this->ncols + col];
		}

		virtual uint close() 
		{
			_clearResult();

			if ( ! this->sqlite ) return 0;

			int r = sqlite3_close(this->sqlite);
			this->sqlite = 0;
			if ( r != SQLITE_OK )
			{
				return _error(__FUNCTION__);
			}
			return 1;
		}

	}; //CConnection


}; //DataBaseSqlite





extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"DataBase.Connection",	 "DataBaseSqlite",	DataBaseSqlite::CConnection::createInterface, DataBaseSqlite::CConnection::classRef	},
		{	0, 0, 0	}
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("DataBaseSqlite 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
