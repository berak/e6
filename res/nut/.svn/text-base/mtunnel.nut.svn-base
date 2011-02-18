// particles

function run(t)
{
	t += 0.02;
    if ( m == null ) return;
	m.setPixelShaderConstant(1, t,0,0,0 );
}

function cleanup()
{
	m = null;
}

function r0_init()
{   
	t <- 0.0;

	local world = World("Core", "Core.World");   
	local root  = world.getRoot();
    loadWorld( world, "plate.e6" );   
    local root = world.getRoot();
    m <- toMesh(root.findRecursive("lissi"));
    if ( m == null ) return;
	local v = loadShader( world, "vs_screen.hlsl" );
	m.setVertexShader( v );

	local p = loadShader( world, "ps_metatunnel.hlsl" );
	m.setPixelShader( p );


    local c = root.findRecursive("DefCam");
    if ( c )
    {
        c.setPos( 0, 0, 60 );
        c.setRot( 0, 0, 0 );
    }
	
}


r0_init();

