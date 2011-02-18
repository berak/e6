// particles

function run(t)
{
    if ( m == null ) return;
	m.setPixelShaderConstant(1, t,0,0,0 );

	// clod2:
	//m.setPixelShaderConstant(2, O,Z,R,X );
}

function cleanup()
{
	m = null;
}

function r0_init()
{   

	O <- 0.03;
	Z <- 1.0/64;
	R <- 12;
	X <- 12;
	local world = World("Core", "Core.World");   
    loadWorld( world, "plate.e6" );   

    m <- toMesh(world.getRoot().findRecursive("lissi"));
    if ( m == null ) return;

	//~ local t = loadTexture( world, "decisions2.bmp" );
	//~ m.setTexture( 0, t );

	local v = loadShader( world, "vs_screen.hlsl" );
	//~ local v = loadShader( world, "vs_chocolux.hlsl" );
	m.setVertexShader( v );

	//~ local p = loadShader( world, "ps_sult.hlsl" );
	//~ local p = loadShader( world, "ps_silex.hlsl" );
	//~ local p = loadShader( world, "ps_metatunnel.hlsl" );
	//~ local p = loadShader( world, "ps_clod2.hlsl" );
	//~ local p = loadShader( world, "ps_disco.hlsl" );
	//~ local p = loadShader( world, "ps_chocolux.hlsl" );
	//~ local p = loadShader( world, "ps_kaleidoscope.hlsl" );
	//~ local p = loadShader( world, "ps_tunnel.hlsl" );
	local p = loadShader( world, "ps_ray.hlsl" );
	//~ local p = loadShader( world, "ps_quaternion.hlsl" );
	//~ local p = loadShader( world, "ps_nautilus.hlsl" );
	m.setPixelShader( p );

	// clod2:
	//~ bright <- 0.65;
	//~ steps <- 0.39;
	//~ scale <- 2.13;
	//~ zval <- 0.8;
}


r0_init();

