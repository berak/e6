

def result( db ):
    print( "\r\n----------------------------------------------------------------\r\n" )
    for i in range(0,db.numCols()):
        print( db.colName(i) + "\t" )

    print( "\r\n----------------------------------------------------------------\r\n" );

    for i in range(0,db.numRows()):
        for j in range(0,db.numCols()):
            print( db.getItem(i,j) + "\t" )
        print( "\r\n" )

    print( "----------------------------------------------------------------\r\n" );


def test_sqlite():
    print("\r\n\r\n************************SQLITE*********************************\r\n\r\n")
    db = e6.Connection( "DatabaseSqlite", "DataBase.Connection" )
    ok = db.open("p4p4","","","")
    if ( not ok ):
        print( "ERROR : could not open p4p4.\r\n" )
        return

    ok = db.query("SELECT name FROM sqlite_master WHERE type='table'")
    if ( not ok ):
        print( "ERROR : could not query sqlite_master.\r\n" )
        return
    
    result(db)
    

    ok = db.query("PRAGMA TABLE_INFO(tipis)")
    if ( not ok ):
        print( "ERROR : could not PRAGMA TABLE_INFO tipis.\r\n" )
        return

    result(db)

    ok = db.query("select * from tipis")
    if ( not ok ):
        print( "ERROR : could not query tipis.\r\n" )
        return

    result(db)    


def test_mysql():
    print("\r\n\r\n************************MYSQL**********************************\r\n\r\n")
    db = e6.Connection( "DatabaseMysql", "DataBase.Connection" )
    ok = db.open("p7d4","localhost","root","")
    if ( not ok ):
        print( "ERROR : could not open p7d4.\r\n" )
        return



    ok = db.query("select * from indians")
    if ( not ok ):
        print( "ERROR : could not query indians.\r\n" )
        return

    result(db)



def main():
    test_sqlite()
    test_mysql()


main()
