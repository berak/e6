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
	loadWorld( world, "ball.3ds" );
	m <- toMesh(root.findRecursive( "Sphere" ));
	if ( m == null ) return;

	local tex = loadTexture( world, "Random.3d.dds" );
	m.setTexture( 0, tex );

	local vs = loadShader( world, "vs_noise0.hlsl" );
	local ps = loadShader( world, "ps_noise0.hlsl" );
	m.setVertexShader( vs );
	m.setPixelShader( ps );

    m.setVertexShaderConstant( 13 ,0,1,0,1 ); // green
    m.setVertexShaderConstant( 25 ,0.07,0.1,0.1,0 ); // scale
    //~ m.setPixelShaderConstant( 0 ,0.6,0,0,0 );    // particleExp
	//~ m.setRenderState( 15, 1 ); // cull ccw
	//~ m.setRenderState( 20, 1 ); // srcblend
	//~ m.setRenderState( 21, 6 ); // dstblend
	//~ m.setRenderState( 23, 0 ); // zwrite
	//~ m.setRenderState( 25, 1 ); // alpha

	local cam = root.findRecursive( "DefCam" );
	if ( cam ) 
	{
		cam.setPos( 0, 0, 5 );
		//~ cam.setRot( -1.5, 0, 0 );
	}
}


r0_init();

