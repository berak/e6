function cleanup()
{
    balls <- null;
}


function run(t)
{
    local s = 0;
    for ( local i=0; i<8; i++  )       
    {
        local v = audio.getOutput(i);
        local p = balls[i].getPos();
        balls[i].setPos( p[0], v*0.01, p[2] );
        s += v * 0.3;
    }
    local m = toMesh( balls[i] );
    if (m) m.setVertexShaderConstant( 13, 0.5, s, 0.5, 0 );        
    print( s + "\r\n" );
}

function audio_init()
{
    balls <- [];

    local tl = TimeLine("TimeLine", "TimeLine.TimeLine");
    audio <- Filter("Audio", "Audio.BandFilter");
    tl.add( audio );

    world <- World("Core", "Core.World");

    loadWorld( world, "ball.3ds" );   
    local root = world.getRoot();
    local b = root.findRecursive("Sphere");
    if ( b )
    {
        b.setFriction( 0.1 );
        balls.append( b );
        
        for ( local i=0; i<7; i++  )       
        {
            local b2 = b.copy();
            b2.setName( "Band_" + i );
            b2.addPos( (i+1)*3, 0, 0 );
            b2.setFriction( 0.1 );
            balls.append( b2 );
        }
    }
}

audio_init();
