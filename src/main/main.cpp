#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../DataBase/DataBase.h"



using e6::uint;

struct CC : DataBase::ResultSet
{
	virtual uint handleItem( uint col, const char * name, const char * value ) 
	{
		printf( "%c%s=%-16s", (col?'\t':'\n'), name, value );
		return 1;
	}
};

int main(int argc, char **argv)
{
	char line[200];
	char * host = "p4p4";
	e6::Engine * e = e6::CreateEngine();	
	DataBase::Connection * db = (DataBase::Connection*)e->createInterface( "DataBase", "DataBase.Connection" );
	CC cc;
	uint r = db->open( host );
	while ( r )
	{
		printf( "\n%s>", host );
		if ( ! fgets( line, 199, stdin ) )
			break;
		r = db->exec( line, &cc );
	}

	E_RELEASE(db);	
	E_RELEASE(e);	
	return 0;
}
