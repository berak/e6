
function buildXml( xml, path )
{
    xml.push("foo");
        xml.pushAttribute("climate", "luke");

        xml.push("bar");
            xml.pushAttribute("name", "mirola");
            xml.pushAttribute("handy", "7236286384628");
        xml.pop();

        xml.push("bar");
            xml.push("location");
                xml.pushAttribute("pz", "6465 5 4");
                xml.pushAttribute("cat", "brandy");
            xml.pop();
            xml.pushAttribute("handy", " 6465 5 4");
            xml.pushAttribute("name", "brandy");
        xml.pop();

        xml.push("bar");
            xml.pushAttribute("handy", "-");
            xml.pushAttribute("name", "manola");
        xml.pop();

        xml.push("bar");
            xml.pushAttribute("handy", "13");
            xml.pushAttribute("name", "carla");
        xml.pop();

    xml.pop();
    xml.save(path);
    xml.clear();
}

function printNode(xml, indent)
{
    print( indent + xml.tagName() );
    for ( local i=0; i<100; i++ )
    {
        local attr = xml.tagAttribute(i);
        if ( ! attr ) break;       
        print( "[" + xml.tagAttributeName(i) + "=" + attr + "]" );
    }
    
    local cdata = xml.tagData();
    if ( cdata )
        print( "[cdata='" + cdata + "']" );
    
    print( "\n" );
}


function traverseNode(xml, indent)
{
    printNode( xml, indent );

    for ( local i=0; xml.child(i); i++ )
    {
        traverseNode( xml, indent + "   " );
    }
    xml.pop();
}


function loadXml( xml, path )
{
    if ( xml.load(path) )
    {
        traverseNode( xml, "*" );
        //~ if ( xml.find("foo") )
        //~ {
            //~ printNode( " " );
            //~ while ( xml.find("bar",k++) )
            //~ {
                //~ printNode( "   " );
                //~ xml.pop();
            //~ }
            //~ xml.pop();
        //~ }
    }
    else
    {
        print( "Error reading" + xml.getName() + " " + path + "\n");
    }
}        


function main(xml)
{
    local path="my.xml";

    buildXml(xml,path);

    loadXml(xml,path);
}

main(Document("xml","xml.Document"));
