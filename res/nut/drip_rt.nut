

function run(t)
{
//	if ( m == null ) return;
}

function cleanup()
{
//	m <- null;
}

function rt_init()
{   
	local world = World("Core", "Core.World");   
	local root  = world.getRoot();
	//~ local vs = loadShader( world, "../hlsl/vs_temp.hlsl" );
	//~ local ps = loadShader( world, "../hlsl/ps_temp.hlsl" );
    //~ print ("VS " + vs + " PS " + ps + "\n");

    local target = createRenderTarget( world, 2, 512, 512, 1 );  
    if ( target )
    {
        target.setName( "_rt0" );
        root.link( target );

        local cam = Camera( "Core","Core.Camera" );
        cam.setPos( 0, 0, 10 );
        cam.setName("lissiCam");
        target.link( cam );
        
        loadWorld( world, "plate.e6" );
        local m = toMesh(root.findRecursive( "lissi" ));
        if ( m )
        {
            local bricks = loadTexture( world, "brick.bmp" );
            m.setTexture( 0, bricks );
            local p = m.getParent();
            p.unlink(m);
            target.link( m );
        }
        loadWorld( world, "cuba.e6" );
        local c = toMesh(root.findRecursive( "Cube" ));
        if ( c )
        {
            local t = target.getRenderTarget();
            c.setTexture( 0, t );
    //~ print ("VS " + vs + " PS " + ps + "\n");
            //~ c.setVertexShader( vs );
            //~ c.setPixelShader( ps );
        }
    }

    local cam = root.findRecursive( "DefCam" );
    if ( cam )
    {
        cam.setPos( 0, 0, 10 );
    }
    //~ m.setVertexShaderConstant( 32 ,0,0,15,0 ); // offset
//	m.setRenderState( 15, 1 ); // cull ccw

}


rt_init();

