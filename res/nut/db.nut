function result( db )
{
    print( "\r\n----------------------------------------------------------------\r\n" );
    for ( local i=0; i<db.numCols(); i++ )
    {
        print( db.colName(i) + "\t" );
    }
    print( "\r\n----------------------------------------------------------------\r\n" );

    for ( local i=0; i<db.numRows(); i++ )
    {
        for ( local j=0; j<db.numCols(); j++ )
        {
            print( db.getItem(i,j) + "\t" );
        }
        print( "\r\n" );
    }
    print( "----------------------------------------------------------------\r\n" );
}

function test_sqlite()
{
    print("\r\n\r\n************************SQLITE*********************************\r\n\r\n")
    local db = Connection( "DataBaseSqlite", "DataBase.Connection" );
    local ok = db.open("p4p4","","","");
    if ( ! ok ) 
    {
        print( "ERROR : could not open p4p4.\r\n" );
        return;
    }

    ok = db.query("SELECT name FROM sqlite_master WHERE type='table'")
    if ( ! ok ) 
    {
        print( "ERROR : could not query sqlite_master.\r\n" );
        return;
    }
    result(db);
    

    ok = db.query("PRAGMA TABLE_INFO(tipis)");
    if ( ! ok ) 
    {
        print( "ERROR : could not PRAGMA TABLE_INFO tipis.\r\n" );
        return;
    }
    result(db);

    ok = db.query("select * from tipis");
    if ( ! ok ) 
    {
        print( "ERROR : could not query tipis.\r\n" );
        return;
    }
    result(db);    
}

function test_mysql()
{
    print("\r\n\r\n************************MYSQL**********************************\r\n\r\n")
    local db = Connection( "DataBaseMysql", "DataBase.Connection" );
    local ok = db.open("p7d4","localhost","root","");
    if ( ! ok ) 
    {
        print( "ERROR : could not open p7d4.\r\n" );
        return;
    }


    ok = db.query("select * from indians");
    if ( ! ok ) 
    {
        print( "ERROR : could not query indians.\r\n" );
        return;
    }
    result(db);
}

function test_odbc()
{
    print("\r\n\r\n************************ODBC**********************************\r\n\r\n")
    local db = Connection( "DataBaseOdbc", "DataBase.Connection" );
    local ok = db.open("my.db.dsn","","","");
    if ( ! ok ) 
    {
        print( "ERROR : could not open my.db.dsn.\r\n" );
        return;
    }


    ok = db.query("select * from my.txt");
    if ( ! ok ) 
    {
        print( "ERROR : could not query my.txt.\r\n" );
        return;
    }
    result(db);
}

function test_postgres()
{
    print("\r\n\r\n************************PG**********************************\r\n\r\n")
    local db = Connection( "DataBasePostgres", "DataBase.Connection" );
    local ok = db.open("p4p4","localhost","postgres","p1p2p3p4");
    if ( ! ok ) 
    {
        print( "ERROR : could not open p4p4.\r\n" );
        return;
    }


    ok = db.query("select * from indians");
    if ( ! ok ) 
    {
        print( "ERROR : could not query my.txt.\r\n" );
        return;
    }
    result(db);
}

function main()
{
//    test_sqlite();
//    test_mysql();
    test_postgres();
}

main();
