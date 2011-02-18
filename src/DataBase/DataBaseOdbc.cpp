#include "../e6/e6_impl.h"
#include "DataBase.h"

#include <stdio.h>
#ifdef WIN32
 #include <windows.h>
#endif

#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>





using e6::uint;
using e6::ClassInfo;


namespace DataBaseOdbc
{

	class CConnection
		: public e6::CName< DataBase::Connection, CConnection >
	{
	private:
		SQLHANDLE odbcHandle, odbcEnv;

		int ncols;
		int nrows;
		char cbuf[64][32][256];

		void _clearResult()
		{
			this->ncols = this->nrows = 0;
		}
		uint _error( const char * fn, const char * txt="" )
		{
			const char * err = "";
			printf( "%s(%s) : error( %s )\n", fn, txt, err );
			return 0; 
		}

	public:
		CConnection() 
	        : odbcHandle(0)
			, odbcEnv(0)
			, nrows(0)
			, ncols(0)
		{
		}

		~CConnection() 
		{
			close();
		}




		virtual uint open( const char * db, const char * host, const char * user, const char * pass ) 
		{
			SQLRETURN r;
			if ( this->odbcHandle )
				return _error(__FUNCTION__, "close running connection first");
			
			r = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &odbcEnv);
			if ( r != SQL_SUCCESS )
				return _error(__FUNCTION__, "SQLAllocHandle(ENV)" );
			
			r = SQLSetEnvAttr(odbcEnv,SQL_ATTR_ODBC_VERSION,(void*)SQL_OV_ODBC3,0);
			if ( r != SQL_SUCCESS )
				return _error(__FUNCTION__, "SQLSetEnvAttr" );
			
			r = SQLAllocHandle(SQL_HANDLE_DBC, odbcEnv, &odbcHandle);
			if ( r != SQL_SUCCESS )
				return _error(__FUNCTION__, "SQLAllocHandle(ODBC)" );

			if ( 1 )
			{
				r = SQLConnect(
					odbcHandle,
					(SQLCHAR*) db, strlen(db),
                    (SQLCHAR*) user, strlen(user),
                    (SQLCHAR*) pass,strlen(pass));
			}
			else
			{
				char xd[300];
				sprintf(xd, "DSN=%s;", db );
				r = SQLDriverConnect(odbcHandle, NULL, (SQLCHAR*)xd, SQL_NTS,
					 NULL, 0, NULL, SQL_DRIVER_COMPLETE);
			}
			if ( r != SQL_SUCCESS )
				return _error(__FUNCTION__, "Connect" );

			return 1; 
		}

		virtual uint query( const char * statement ) 
		{
			_clearResult();

			if ( ! this->odbcHandle ) 
				return _error(__FUNCTION__,"DB_CLOSED");

			SQLRETURN r;
			SQLHANDLE odbcStmt;

			r = SQLAllocHandle(SQL_HANDLE_STMT, odbcHandle, &odbcStmt);
			if ( r != SQL_SUCCESS )
				return _error(__FUNCTION__, "SQLAllocHandle(stmt)" );

			r = SQLExecDirect(odbcStmt, (SQLCHAR*)statement, SQL_NTS);
			if ( r != SQL_SUCCESS )
				return _error(__FUNCTION__, "SQLExecDirect" );

			// get rows/cols:
			SQLSMALLINT nc;
			r = SQLNumResultCols(odbcStmt, &nc);
			if ( r != SQL_SUCCESS )
				return _error(__FUNCTION__, "SQLNumResultCols" );
			this->ncols = nc-1; /// HARRRRRR!!

			//SQLINTEGER nr;
			//r = SQLRowCount(odbcStmt, &nr);
			//if ( r != SQL_SUCCESS )
			//	return _error(__FUNCTION__, "SQLRowCount" );
			//this->nrows = nr;
			
			// retrieve names:
			for ( int i=0; i<ncols; i++ )
			{
				r = SQLDescribeCol( odbcStmt, (i+1), (SQLCHAR*)cbuf[0][i], 256, 0, 0,0,0,0);
				if ( r != SQL_SUCCESS )
				{
					_error(__FUNCTION__, "SQLDescribeCol" );
					break;
				}
			}

			// retrieve resultset:
			if ( this->ncols>0  )
			{
				SQLINTEGER resLen;
				int row = 1;
				while(SQLFetch(odbcStmt) == SQL_SUCCESS)
				{
					for ( int i=0; i<ncols; i++ )
					{
						r = SQLGetData(odbcStmt,(i+1),SQL_C_CHAR,(SQLPOINTER)cbuf[row][i],256,&resLen);
						if ( r != SQL_SUCCESS )
						{
							_error(__FUNCTION__, "SQLGetData" );
							break;
						}
					}
					row ++;
					this->nrows ++;
				}
			}

			SQLFreeHandle(SQL_HANDLE_STMT, odbcStmt);
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
			if ( (int)col >= this->ncols ) return 0;
			return this->cbuf[0][col];
		}
		virtual const char *getItem( uint row, uint col ) const 
		{
			if ( (int)row >= this->nrows ) return 0;
			if ( (int)col >= this->ncols ) return 0;
			return this->cbuf[(row+1)][col];
		}

		virtual uint close() 
		{
			_clearResult();

			if ( ! this->odbcHandle )
				return 0;

			SQLDisconnect(this->odbcHandle);
			#if ODBCVER < 0x0300
			  SQLFreeConnect(this->odbcHandle);
			#else
			  SQLFreeHandle(SQL_HANDLE_DBC,this->odbcHandle);
			  SQLFreeHandle(SQL_HANDLE_ENV,this->odbcEnv);
			#endif
			this->odbcHandle = 0;
			this->odbcEnv = 0;

			return 1;
		}

	}; //CConnection


}; //DataBaseOdbc





extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"DataBase.Connection",	 "DataBaseOdbc",	DataBaseOdbc::CConnection::createInterface, DataBaseOdbc::CConnection::classRef	},
		{	0, 0, 0	}
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("DataBaseOdbc 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
