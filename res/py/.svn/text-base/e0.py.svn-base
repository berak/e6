
def dict_info( o ):
    try:
        d = o.__dict__
    except:
        print ("(nodict)\r\n")
        return       
    for (k,v) in d.items():
        sk=str(k)
        try:
            doc = str( eval(sk+'.__doc__') )
        except:
            doc = '(nodoc)'
        print( "  " + sk + "\r\n\t" +  doc + "\r\n\t" +  str(v) + "\r\n" )


def _arr_info( om ):
    for (m) in om:
        try:
            doc = str( eval(m+'.__doc__') )
        except:
            doc = '(nodoc)'
        print( "  " + str(m) + "\t" +  doc + "\r\n" )

def meth_info( o ):
    try:
        _arr_info( o.__methods__ )
    except:
        print ("(nometh)\r\n")
        return       


def member_info( o ):
    try:
        _arr_info( o.__members__ )
    except:
        print ("(nomemb)\r\n")
        return       



#~ def engine_info():
    #~ dict_info( engine )

