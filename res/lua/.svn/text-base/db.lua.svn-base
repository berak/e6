
function result( db )

    local nc = db:numCols()
    local nr = db:numRows()
    print( "\n----------------------------------------------------------------\n" );
    i=0
    while( i < nc ) do
        print( db:colName(i) , "\t" );
        i = i + 1
    end
    print( "\n----------------------------------------------------------------\n" );

    i=0
    while( i < nr ) do
        j=0
        while( j < nc ) do
            print( db:getItem(i,j) , "\t" );
            j = j + 1
        end
        print( "\n" );
        i = i + 1
    end
    print( "----------------------------------------------------------------\n" );
end

function test_sqlite()
    print("\n\n************************SQLITE*********************************\n\n")
    local db = Connection( "DatabaseSqlite", "DataBase.Connection" );
    local ok = db:open("p4p4","","","");
    if ( not ok ) then
        print( "ERROR : could not open p4p4.\n" );
        return;
    end

    ok = db:query("SELECT name FROM sqlite_master WHERE type='table'")
    if ( not ok ) then
        print( "ERROR : could not query sqlite_master.\n" );
        return;
    end
    result(db);
    

    ok = db:query("PRAGMA TABLE_INFO(tipis)");
    if ( not ok ) then
        print( "ERROR : could not PRAGMA TABLE_INFO tipis.\n" );
        return;
    end
    result(db);

    ok = db:query("select * from tipis");
    if ( not ok ) then
        print( "ERROR : could not query tipis.\n" );
        return;
    end
    result(db);

    -- db:close();   
end


function test_mysql()
    print("\n\n************************MYSQL**********************************\n\n")
    local db = Connection( "DatabaseMysql", "DataBase.Connection" );
    local ok = db:open("p7d4","localhost","root","");
    if ( not ok ) then
        print( "ERROR : could not open p7d4.\n" );
        return;
    end


    ok = db:query("select * from indians");
    if ( not ok ) then
        print( "ERROR : could not query indians.\n" );
        return;
    end
    result(db);

    
    -- db:close();
end


function main()
    test_sqlite();
    test_mysql();
end

main();
