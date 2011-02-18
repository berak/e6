// particles

function run(t)
{
    if ( m == null ) return;
	m.setPixelShaderConstant(1, t,0,0,0 );
}

function cleanup()
{
	m = null;
}

function r0_init()
{   

	local world = World("Core", "Core.World");   
    loadWorld( world, "plate.e6" );   

    m <- toMesh(world.getRoot().findRecursive("lissi"));
    if ( m == null ) return;

	local v = loadShader( world, "vs_screen.hlsl" );
	m.setVertexShader( v );
	local p = loadShader( world, "ps_sult.hlsl" );
	m.setPixelShader( p );
}


r0_init();

