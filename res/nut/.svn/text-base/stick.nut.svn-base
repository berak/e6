
class Stickman // extends Spring
{
    bones = [];
    root = null;
    
    function addNode( n )
    {
        bones.append( n );
    }

    function update( dt )
    {
        for ( local i=1; i<bones.len(); i++  )
        {
            //~ move( nodes[i-1], nodes[i], dt );
        }
    }
};


function run(t)
{
    //~ local dt = t - t_last;
    //~ chain.update(dt);
    //~ t_last = t;

    for ( local i=0; i<8; i++  )       
    {
        local v = audio.getOutput(i);
        //~ local p = balls[i].getPos();
        //~ balls[i].setPos( p[0], v*0.01, p[2] );
        //~ local m = toMesh( balls[i] );
        //~ if (m) m.setVertexShaderConstant( 13, 0.5, v*2, 0.5, 0 );        
    }

    //~ local vs = gui.getValue(  "Kine", "damping" );
//~ //print( "vs " + vs + "\n");
    //~ local v = vs.tofloat()  * 0.0001;
    //~ chain.setDamping( v );

    //~ local rs = "0" + gui.getValue(  "Kine", "reset" );
//~ //print( "rs " + rs + "\n");
    //~ local r = rs.tointeger() > 0;
    //~ if ( r )
    //~ {
        //~ gui.setValue(  "Kine", "reset", "0" );
    //~ }
//~ //print( "v " + v + "\tr" + r + "\n");
}

function stick_init()
{
    //~ chain <- Chain("Physics", "Physics.Spring");
    t_last <- 0;
    stickman <- Stickman();
    world <- World("Core", "Core.World");
    //~ gui   <- Panel("ClientGui","ClientGui.Panel");
 
    //~ gui.clear();
    //~ gui.addRollout( "Kine" );
    //~ gui.addControl( "Kine", "Slider", "damping" );
    //~ gui.addControl( "Kine", "Edit", "reset" );
    //~ gui.setValue( "Kine", "damping", "0.2" );
    //~ gui.setValue( "Kine", "reset", "0" );
    //~ gui.expand( "Kine", 1 );
    
    //~ world.clear();
    loadWorld( world, "stick.e6" );
    
    local root = world.getRoot();
    local b = root.findRecursive("Stick");
    if ( b )
    {
        stickman.root = b.getParent();
        
        local ll = b.copy();
        ll.setName( "Leg_l" );
        ll.addPos( -3, -5, 0 );
        stickman.addNode( ll );
        
        local lr = b.copy();
        lr.setName( "Leg_r" );
        lr.setPos( 3, -5, 0 );
        stickman.addNode( lr );

        local al = b.copy();
        al.setName( "Arm_l" );
        al.setPos( -3, 5, 0 );
        al.setRot( 0, 0, -1.5052 );
        stickman.addNode( al );
        
        local ar = b.copy();
        ar.setName( "Arm_r" );
        ar.setPos( 3, 5, 0 );
        ar.setRot( 0, 0, 1.5052 );
        stickman.addNode( ar );

        local hd = b.copy();
        hd.setName( "Head" );
        hd.addPos( 0, 5, 0 );
        hd.setRot( 0, 0, 0.1 );
        stickman.addNode( hd );
    }

    local tl = TimeLine("TimeLine", "TimeLine.TimeLine");
    audio <- Filter("Audio", "Audio.BandFilter");
    tl.add( audio );
}

stick_init();
