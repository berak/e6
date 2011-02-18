# particles

def run(t):
	pass

def cleanup():
	pass


def r0_init():
	world = e6.World("Core", "Core.World");   
	e6.loadWorld( world, "QuadArray.3ds" );

	m = e6.toMesh( world.findRecursive( "object_0" ));
	if ( not m  ): 
		return;

	m.setTexture( 0, e6.loadTexture( world, "ramp1.bmp" ) );
	m.setVertexShader( e6.loadShader( world, "vs_part0.hlsl" ) );
	m.setPixelShader(  e6.loadShader( world, "ps_part0.hlsl" ) );
	m.setVertexShaderConstant( 13 ,0,1,0,1 ); # green
	m.setVertexShaderConstant( 25 ,3,3,11,0 ); # speed, depth, width, 0
	m.setPixelShaderConstant( 0 ,0.6,0,0,0 );    # particleExp
	m.setRenderState( 15, 1 ); # cull ccw
	m.setRenderState( 20, 1 ); # srcblend
	m.setRenderState( 21, 6 ); # dstblend
	m.setRenderState( 23, 0 ); # zwrite
	m.setRenderState( 25, 1 ); # alpha

	cam = world.findRecursive( "DefCam" );
	if ( cam ) :
		cam.setPos( 0, 10, 0 );
		cam.setRot( -1.5, 0, 0 );



r0_init();

