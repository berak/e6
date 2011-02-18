function run(t)
{
	if ( m == null ) return;
}

function cleanup()
{
	m <- null;
}

function wave_init()
{   
	local world = World("Core", "Core.World");   

	loadWorld( world, "plate.e6" );

	m <- toMesh(world.findRecursive( "lissi" ));
	if ( m == null ) return;

	m.setPos(0, 0, -6);
	m.setRot(-1.3, 0, 0);
	m.setTexture( 0, loadTexture( world, "ripple.bmp" ) );
	m.setTexture( 1, loadTexture( world, "Lobby.cube.dds" ) );
	m.setVertexShader( loadShader( world, "vs_wave.hlsl" ) );
	m.setPixelShader( loadShader( world, "ps_wave.hlsl" ) );

    m.setVertexShaderConstant( 13 ,0,1,0,1 ); // green
    m.setVertexShaderConstant( 33 ,0.07,0.1,0.1,0 ); // scale
    //~ m.setPixelShaderConstant( 0 ,0.6,0,0,0 );    // particleExp
	//~ m.setRenderState( 15, 1 ); // cull ccw
	//~ m.setRenderState( 20, 1 ); // srcblend
	//~ m.setRenderState( 21, 6 ); // dstblend
	//~ m.setRenderState( 23, 0 ); // zwrite
	//~ m.setRenderState( 25, 1 ); // alpha

	local cam = world.findRecursive( "DefCam" );
	if ( cam ) 
	{
		cam.setPos( 0, 0, 5 );
		//~ cam.setRot( -1.5, 0, 0 );
	}
}


wave_init();

