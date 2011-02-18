// ernie_toon

function run(t)
{
	if ( m == null ) return;
}

function cleanup()
{
	m <- null;
}

function ernie_toon_init()
{   
	local world = World("Core", "Core.World");   
	local root  = world.getRoot();
	loadWorld( world, "Dino.3ds" );
	m <- toMesh(root.findRecursive( "dino2" ));
	if ( m == null ) return;

//	m.setTexture( 0, loadTexture( world, "ramp1.bmp" ) );

	local vs = loadShader( world, "vs_toon.hlsl" );
	local ps = loadShader( world, "ps_toon0.hlsl" );
	m.setVertexShader( vs );
	m.setPixelShader( ps );

    m.setVertexShaderConstant( 17 ,0.5,-1,0.5,1 ); // light0
//    m.setVertexShaderConstant( 25 ,3,3,11,0 ); // speed, depth, width, 0
//    m.setPixelShaderConstant( 0 ,0.6,0,0,0 );    // particleExp
//	m.setRenderState( 15, 1 ); // cull ccw
//	m.setRenderState( 20, 1 ); // srcblend
//	m.setRenderState( 21, 6 ); // dstblend
//	m.setRenderState( 23, 0 ); // zwrite
//	m.setRenderState( 25, 1 ); // alpha

	local cam = root.findRecursive( "DefCam" );
	if ( cam ) 
	{
		cam.setPos( 0, 0, 80 );
//		cam.setRot( -1.5, 0, 0 );
	}
}


ernie_toon_init();

