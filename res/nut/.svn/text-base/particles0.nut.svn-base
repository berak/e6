// particles

function run(t)
{
}

function cleanup()
{
}

function r0_init()
{   
	local world = World("Core", "Core.World");   
	local root  = world.getRoot();
	loadWorld( world, "QuadArray.3ds" );
	local m = toMesh(root.findRecursive( "object_0" ));
	if ( m == null ) return;

	m.setTexture( 0, loadTexture( world, "ramp1.bmp" ) );

	local vs = loadShader( world, "vs_part0.hlsl" );
	local ps = loadShader( world, "ps_part0.hlsl" );
	m.setVertexShader( vs );
	m.setPixelShader( ps );

    m.setVertexShaderConstant( 13 ,0,1,0,1 ); // green
    m.setVertexShaderConstant( 25 ,3,3,11,0 ); // speed, depth, width, 0
    m.setPixelShaderConstant( 0 ,0.6,0,0,0 );    // particleExp
	m.setRenderState( 15, 1 ); // cull ccw
	m.setRenderState( 20, 1 ); // srcblend
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

