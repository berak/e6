// particles

function run(t)
{
	if ( m != null )
		m.setPixelShaderConstant( 0, 0.7, 0.0, 2.2,70.0 );  
}

function cleanup()
{
	m <- null;
}

function r0_init()
{   
	local world = World("Core", "Core.World");   
	local root  = world.getRoot();
    loadWorld( world, "plate.e6" );   
    local root = world.getRoot();
    m <- toMesh(root.findRecursive("lissi"));
    if ( m == null ) return;
    
    m.setTexture( 0,  loadTexture( world, "pal.bmp" ) );
    m.setPixelShader ( loadShader( world, "ps_mandel.hlsl" ) );

    local c = root.findRecursive("DefCam");
    if ( c )
    {
        c.setPos( 0, 0, 70 );
        c.setRot( 0, 0, 0 );
    }
}


r0_init();

