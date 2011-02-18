
function run(t)
{
    for ( local i=0; i<5; i++  )       
    {
        local v = midi.getOutput(i);
        print( "*" + i + " " + midi.getOutputName(i) + " " + v + "\n" );
        //~ local p = balls[i].getPos();
        //~ balls[i].setPos( p[0], v*.01, p[2] );
        //~ local m = toMesh( balls[i] );
        //~ if (m) m.setVertexShaderConstant( 13, 0.5, v*2, 0.5, 0 );        
    }
    print( "*" + "\n" );
}

function midi_init()
{
    balls <- [];

    local tl = TimeLine("TimeLine", "TimeLine.TimeLine");
    midi <- MidiInput("Midi", "Midi.Input");
    tl.add( midi );
    midi.addEvent( 0x90,0,32 );
    midi.addEvent( 0x90,0,33 );
    midi.addEvent( 0x90,0,34 );
    midi.addEvent( 0xb0,0,7 );
    midi.addEvent( 0xb0,0,91 );
    //~ world <- World("Core", "Core.World");

    //~ loadWorld( world, "ball.3ds" );   
    //~ local root = world.getRoot();
    //~ local b = root.findRecursive("Sphere");
    //~ if ( b )
    //~ {
        //~ b.setFriction( 0.1 );
        //~ balls.append( b );
        
        //~ for ( local i=0; i<7; i++  )       
        //~ {
            //~ local b2 = b.copy();
            //~ b2.setName( "Band_" + i );
            //~ b2.addPos( (i+1)*3, 0, 0 );
            //~ b2.setFriction( 0.1 );
            //~ balls.append( b2 );
        //~ }
    //~ }
}

midi_init();
