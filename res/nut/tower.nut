

function run(t)
{
	if ( m == null ) return;
}

function cleanup()
{
	m <- null;
}

function r0_init()
{   
	local world = World("Core", "Core.World");   
	local root  = world.getRoot();
	loadWorld( world, "cuba.e6" );
	m <- toMesh(root.findRecursive( "Cube" ));
	if ( m == null ) return;
	
	local tex = Texture( "Core", "Core.Texture" );
	tex.setName( "E:/code/e6/res/dds/tower.cube.dds");
	m.setTexture( 0, tex );
	
	local vs = loadShader( world, "vs_refl.hlsl" );
	local ps = loadShader( world, "ps_cube.hlsl" );
	m.setVertexShader( vs );
	m.setPixelShader( ps );

    m.setVertexShaderConstant( 32 ,0,0,15,0 ); // offset
//	m.setRenderState( 15, 1 ); // cull ccw

	local cam = (root.findRecursive( "DefCam" ));
	if ( cam ) cam.setPos( 0, 0, 10 );
}


r0_init();

