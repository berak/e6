

function run(t)
{
	if ( m == null ) return;
//    local vs = gui.getValue(  "q0", "offset" );
//	print( "vs " + vs + "\n");
//    local v = vs.tofloat()  * 0.01;
}

function cleanup()
{
	m <- null;
}
function q0_init()
{   
	local world = World("Core", "Core.World");   
	local root  = world.getRoot();
//	loadWorld( world, "cols.3ds" );
//	m <- toMesh(root.findRecursive( "column3_4" ));
//	//loadWorld( world, "torus.3ds" );
//	//m <- toMesh(root.findRecursive( "Torus Knot" ));
//	loadWorld( world, "plane_32.e6" );
//	m <- toMesh(root.findRecursive( "Plane_32" ));

	loadWorld( world, "Sphere.3ds" );
	m <- toMesh(root.findRecursive( "Sphere01" ));

	local cam = root.findRecursive( "DefCam" )
	if ( cam )
	{
		cam.setPos(0,0,0);
		cam.setRot(0,0,0);
	}
	//~ local tex = Texture( "Core", "Core.Texture" );
//~ //	tex.setName( "E:/code/e6/res/dds/pool5121nomip.cube.dds");
	//~ tex.setName( "Park.cube.dds");
	local basetex = loadTexture( world, "pal.bmp" );
	local cubetex = loadTexture( world, "Park.cube.dds" );
	m.setTexture( 0, basetex );
	m.setTexture( 1, cubetex );
	m.recalcTangents();
	
	local vs = loadShader( world, "vs_bubble.hlsl" );
	local ps = loadShader( world, "ps_bubble.hlsl" );
	m.setVertexShader( vs );
	m.setPixelShader( ps );

    m.setVertexShaderConstant( 32 ,0,0,15,0 ); // offset
	m.setRenderState( 15, 1 ); // cull ccw

    //gui   <- Panel("ClientGui","ClientGui.Panel");
    //gui.clear();
    //gui.addRollout( "q0" );
    //gui.addControl( "q0", "Slider", "offset" );
    //gui.setValue( "q0", "offset", "70" );
    //gui.expand( "q0", 1 );

}


q0_init();

