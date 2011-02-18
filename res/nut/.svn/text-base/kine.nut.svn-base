
class Chain extends Spring
{
    nodes = [];
    
    function addNode( n )
    {
        nodes.append( n );
    }


    function update( dt )
    {
        for ( local i=1; i<nodes.len(); i++  )
        {
            move( nodes[i-1], nodes[i], dt );
        }
    }
};


t_last <- 0;
function run(t)
{
    local dt = t - t_last;
    chain.update(dt);
    t_last = t;

    local vs = gui.getValue(  "Kine", "damping" );
//print( "vs " + vs + "\n");
    local v = vs.tofloat()  * 0.0001;
    chain.setDamping( v );

    local rs = "0" + gui.getValue(  "Kine", "reset" );
//print( "rs " + rs + "\n");
    local r = rs.tointeger() > 0;
    if ( r )
    {
        gui.setValue(  "Kine", "reset", "0" );
    }
//print( "v " + v + "\tr" + r + "\n");
}

function kine_init()
{
    chain <- Chain("Physics", "Physics.Spring");
    world <- World("Core", "Core.World");
    gui   <- Panel("ClientGui","ClientGui.Panel");
 
    gui.clear();
    gui.addRollout( "Kine" );
    gui.addControl( "Kine", "Slider", "damping" );
    gui.addControl( "Kine", "Edit", "reset" );
    gui.setValue( "Kine", "damping", "0.2" );
    gui.setValue( "Kine", "reset", "0" );
    gui.expand( "Kine", 1 );
    
    //~ world.clear();
    loadWorld( world, "ball.3ds" );
    
    local root = world.getRoot();
    local b = root.findRecursive("Sphere");
    if ( b )
    {
        chain.setDamping( 0.2 );

        chain.addNode( b );
        b.setFriction( 0.1 );
        
        for ( local i=1; i<8; i++  )       
        {
            local b2 = b.copy();
            b2.setName( "Spring_" + i );
            b2.addPos( i*3, 0, 0 );
            chain.addNode( b2 );
        }
    }
}

kine_init();
