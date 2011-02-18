// xeyes

class Damping
{
    _items = null;
    _cur = 0;
    _sum = 0.0;

    constructor()
    {
        _items = [0.0,0.0,0.0,0.0, 0.0,0.0,0.0,0.0, 0.0,0.0,0.0,0.0, 0.0,0.0,0.0,0.0]; // 16
        _cur = 0;
        _sum = 0.0;
    }
        
    function avg()
    {
        return _sum / 16.0; //( sum() / count() );
    }
    
    function set( v )
    {
        _cur ++;
        if ( _cur >= 16 ) _cur = 0;
        _sum -= _items[_cur];
        _sum += v;
        _items[_cur] = v;
        return _sum / 16.0; 
    }
};



class Eyes
{
    quad = null;
    damp_x = null;
    damp_y = null;
    damp_dx = null;
    damp_dy = null;
    balls = [];
    audio = null;
    
    constructor()
    {
        damp_x = Damping();
        damp_y = Damping();
        damp_dx = Damping();
        damp_dy = Damping();

        quad = MotionDetector("Video", "Video.MotionDetector");

        local world = World("Core", "Core.World");

        loadWorld( world, "Sphere.3ds" );   
        local root = world.getRoot();
        local b = root.findRecursive("Sphere01");
        if ( b )
        {
            local m = toMesh(b);
            if ( m != null )
            {
                local t = loadTexture( world, "eye.bmp" ); //i6
                m.setTexture( 0, t );
                
                local v = loadShader( world, "vs_eyes.hlsl" );
                m.setVertexShader( v );
                m.setVertexShaderConstant( 13, 0.8, 0.8, 0.8, 1 );

                local p = loadShader( world, "ps_temp.hlsl" );
                m.setPixelShader( p );

                m.setRot( -1.5, 1.6, 0 );
                m.setFriction( 0.1 );

                local m2 = m.copy();

                m.setName( "eye_l" );
                m.addPos( -20, 0, 0 );
                balls.append( m );

                m2.setName( "eye_r" );
                m2.addPos( 20, 0, 0 );
                balls.append( m2 );
            }

        }
        local d = root.findRecursive("DefCam");
        if ( d )
        {
            d.setPos( 50, 0, 225 );
            d.setRot( 0, 0.03, 0 );
        }

        //~ audio = Player("Audio", "Audio.Player");
        //~ if ( audio )
        //~ {
            //~ audio.loadVoice( "E:/wav/ambient/Music_from_hell_1.wav" );
            //~ audio.setState( 0, 3 );
        //~ }

        //~ local app = Main("Application", "Application.Main");
        //~ if ( app )
        //~ {
            //~ app.bindScriptEvent( 1, "run(t)" );
        //~ }
    }

    function run(t)
    {
        if ( quad == null ) return;
        local v = 0.0003 * quad.peakV();
        local x = 0;
        local y = 0;
        if ( v < 0.02 ) // turn back to center pos
        {
            x = damp_x.avg();       y = damp_y.avg();
            x *= 0.9;               y *= 0.9;
            damp_x.set(x);          damp_y.set(y);  
        }
        else
        {
            x =        0.15  * quad.peakX();
            y = 0.02 + 0.12  * quad.peakY();
            x = damp_x.set(x); 
            y = damp_y.set(y); 
        }
        // print( "# " + x + " " + y + " " + v + "\n" );
        

        local x2 = x * v ;
        local v2 = v * v;

        toMesh(balls[0]).setVertexShaderConstant( 25, y, (x + x2), v2, 0 );
        toMesh(balls[1]).setVertexShaderConstant( 25, y, (x - x2), v2, 0 );

        if ( audio )
        {
            audio.setPan( 0, -1000 * quad.peakX() );
        }
    }


};


function run(t)
{
    if ( eyes ) eyes.run(t);
}

function cleanup()
{
    eyes <- null;
}



eyes <- Eyes();
