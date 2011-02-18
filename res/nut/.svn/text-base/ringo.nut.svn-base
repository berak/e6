// particles

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
	loadWorld( world, "QuadArray.3ds" );
	m <- toMesh(root.findRecursive( "object_0" ));
	if ( m == null ) return;

    m.scaleBy( 40,40,40 );
	m.setTexture( 0, loadTexture( world, "ramp1.bmp" ) );

	local vs = loadShader( world, "vs_ringo0.hlsl" );
	local ps = loadShader( world, "ps_ringo0.hlsl" );
	m.setVertexShader( vs );
	m.setPixelShader( ps );

    m.setVertexShaderConstant( 13 ,0,1,0,1 ); // green
    m.setVertexShaderConstant( 33 ,0.003,0.13,0.101,0 ); // scale
	m.setRenderState( 15, 0 ); // cull ccw
	m.setRenderState( 20, 5 ); // srcblend
	m.setRenderState( 21, 6 ); // dstblend
	m.setRenderState( 23, 0 ); // zwrite
	m.setRenderState( 25, 1 ); // alpha

	local cam = root.findRecursive( "DefCam" );
	if ( cam ) 
	{
		cam.setPos( 0, 10, 0 );
		cam.setRot( -1.5, 0, 0 );
	}
}


r0_init();

