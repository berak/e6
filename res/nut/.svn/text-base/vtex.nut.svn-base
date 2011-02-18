// particles

function run(t)
{
}

function cleanup()
{
	//m <- null;
	vt <- null;
}

function r0_init()
{   
	local world = World("Core", "Core.World");   
	local root  = world.getRoot();
//    loadWorld( world, "icosphere.e6" );   
    loadWorld( world, "box.e6" );   
    local root = world.getRoot();
//    local m = toMesh(root.findRecursive("Sphere"));
    local m = toMesh(root.findRecursive("Box"));
    if ( m == null ) return;

    local t = Texture("Core", "Core.Texture");

	vt <- VideoTexture("Video", "Video.VideoTexture");
	if ( vt )
	{
		vt.setTexture( t );
		vt.start("Camera","NullRenderer");
//		vt.start("e:/video/u/cam.avi","NullRenderer");
		m.setTexture( 0, t );
	}
	

    local c = root.findRecursive("DefCam");
    if ( c )
    {
        c.setPos( 0, 0, 60 );
        c.setRot( 0, 0, 0 );
    }
	
}


r0_init();

