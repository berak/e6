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
    }
};



class Eyes
{
    quad = null;
    damp_x = null;
    damp_y = null;
    eye_l = null;
    eye_r = null;
    rx=-1.5;
    ry=1.60;
    rz=0;
    rylr=0.2;
    
    constructor()
    {
        damp_x = Damping();
        damp_y = Damping();

        local tl = TimeLine("TimeLine", "TimeLine.TimeLine");
        quad = Filter("Video", "Video.QuadrantFilter");
        quad.setName( "Quadrupe" );
        tl.add( quad );

        local world = World("Core", "Core.World");

        loadWorld( world, "Sphere.3ds" );   
        local root = world.getRoot();
        local b = root.findRecursive("Sphere01");
        if ( b )
        {
            eye_l = toMesh(b);
            if ( eye_l != null )
            {
                local t = loadTexture( world, "eye.bmp" ); //i6
                eye_l.setTexture( 0, t );
                eye_l.setRot( rx, ry, rz );
                eye_l.setFriction( 0.1 );

                eye_r = eye_l.copy();

                eye_l.setName( "eye_l" );
                eye_l.addPos( -20, 0, 0 );

                eye_r.setName( "eye_r" );
                eye_r.addPos( 20, 0, 0 );
            }

        }
        local d = root.findRecursive("DefCam");
        if ( d )
        {
            d.setPos( 50, 0, 185 );
            d.setRot( 0, 0.03, 0 );
        }
    }

    function run(t)
    {
        if ( quad == null ) return;
        local v = 0.0003 * quad.getOutput(0);
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
            x =         quad.getOutput(1);
            y = 0.02 +  quad.getOutput(2);
            damp_x.set(x);    x = damp_x.avg();
            damp_y.set(y);    y = damp_y.avg();
        }
        // print( "# " + x + " " + y + " " + v + "\n" );
        
		local ax = rx - x;
		local ay = ry;
		local az = rz - y;
        print( "# " + ax + "\t" + ay + "\t" + az + "\n" );
        eye_l.setRot( ax, ay, az );
        eye_r.setRot( ax, ay, az );
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
