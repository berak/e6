function print_table( t, name,sp )
{
	if (t==null)
	{
		print("-"+name+"(nil)- \n");
		return;
	}

	print( sp + "- "+name+" - \n");
	foreach ( x,y in t )
	{

		print( sp + x +"\t\t");
		if ( x.len()<8 )
		{
			print("\t");
		}
		print( y +"\n");
		if (type(y) == "table" || type(y) == "class" )
		{
			print_table( y,x,sp+"\t" );
		}
	}
}

function tl()
{
	local tl = TimeLine("TimeLine", "TimeLine.TimeLine");
	local v1 = Value("TimeLine", "TimeLine.Value");
	local v2 = Value("TimeLine", "TimeLine.Value");
	local s  = Sinus("TimeLine", "TimeLine.Sinus");
	s.setName("sin0");
	v1.setName("val1");
	v1.addInput("i0");
	v2.setName("val2");
	v2.addInput("i0");
	v2.addOutput("o0");
	tl.add( v1 );
	tl.add( v2 );
	tl.add( s );
	tl.connect( "val1", "i0", "val2", "o0" );
	tl.connect( "val2", "i0", "sin0", "sin" );
	tl.print();

	//~ local v2a = tl.get( "val2" );
	//~ print_table( v2a.getclass(), "v2a"+v2a, "" );
	//~ local s2a = tl.get( "sin0" );
	//~ print_table( s2a.getclass(), "s2a"+s2a, "" );
	return tl;
}


function nodeinfo( n )
{
	local z = n.getPos();
	print( "--------- " + n.typeName() + " ---------\n" );
	print( "name\t: "   + n.getName() + "\n" );
	print( "pos\t: [" + z[0] + ", " + z[1] + ", " + z[2] + "]\n");
	print( "sphere\t: " + n.getSphere() + "\n" );
	local p = n.getParent();
	local pn = "nil";
	if ( p ) pn = p.getName();
	print( "parent\t: " + pn + "\n" );
	print( "nchild\t: " + n.numChildren()+ "\n" );
}


function cm()
{
	local m = Mesh("Core", "Core.Mesh");
	local m0 = toMesh(m);
	print_table( m0.getclass(), "m0"+m0, "" );
	nodeinfo(m0);	
}

function rt()
{
	local w = World("Core", "Core.World");
	local r = w.getRoot();
	print_table( r.getclass(), "root"+r, "" );
	nodeinfo(r);	
}

function lm()
{
	local w = World("Core", "Core.World");
	loadWorld( w, "ernie.3ds" );

	local r = w.getRoot();
	local l = Light("Core", "Core.Light");

	r.link(l);

	local e = r.findRecursive( "stand1" );
	if ( e )
	{
		print_table( e.getclass(), e.getName()+e, "" );
		nodeinfo(e);

		local em = toMesh(e);
		print_table( em.getclass(), "EM"+em, "" );
		nodeinfo(em);
	}
	return w;
}

function ct()
{
	local m = Mesh("Core", "Core.Mesh");
	m.setPos(1.2,13.6,-1.1);
	local z = m.getPos();
	print(z +" "+z.len()+" "+z[0]+" "+z[1]+" "+z[2]+"\n");
}

/*
	so schwebe ich also knapp 20 cm überm wasser & krieg
	3 x am tag nasse füsse..
*/


function run( t )
{
    print( t + "\n" );
}
