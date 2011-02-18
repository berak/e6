//~ function sign(i)
//~ {
	//~ return (i>0?1:-1);
//~ }

function run(t)
{
	if ( m == null ) return;
	if ( art == null ) return;
	
	if ( art.getPatternLost(0) < 5 )
	{
		print( "* " + t + " : " + art.getPatternOrient(2) + ".\r\n" );

		local v = null;
		v = art.getPatternPos(0);
		v[0] *= 0.2;
		v[1] *= 0.2;
		v[2] *= -0.07;
		m.setPos( v[0] , v[1] , v[2] );
		print( "  " + v[0] + ", " + v[1] +", "+ v[2] + "\r\n" );

		v = art.getPatternRot(0);
		local invPi = 1.0 / 3.14; 
		v[0] *= invPi;
		v[1] *= invPi;
		v[2] *= invPi;
		print( "  " + v[0] + ", " + v[1] +", "+ v[2] + "\r\n" );
		//m.setRot( v[0] , v[1] , v[2] );

		v = art.getPatternQuat(0);
		print( "  " + v[0] + ", " + v[1] +", "+ v[2] + ", "+ v[3] + "\r\n" );
		//m.setRot( v[0] , v[1] , v[2] );
	}
}


function cleanup()
{
	print( "cleanup  " + m + "," + art + "\r\n" );
	m <- null;
	art <- null;
}


function art_init()
{   
	art <- ArtDetector("Video","Video.ArtDetector");
	if ( art == null ) return;

	art.addPattern("res/pat/patt.kanji");
	art.addPattern("res/pat/patt.hiro");
	art.addPattern("res/pat/patt.my");
	art.addPattern("res/pat/patt.p0");
	art.addPattern("res/pat/patt.p1");
	
	art.setThreshold( 60 );
	art.start("Camera", "Video Renderer");

	local world = World("Core", "Core.World");   
	loadWorld( world, "cuba.e6" );
	m <- toMesh(world.findRecursive( "Cube" ));
	if ( m == null ) return;
	m.setPos(0, 0, -6);
	//m.setRot(-1.3, 0, 0);
	//m.setTexture( 0, loadTexture( world, "ripple.bmp" ) );

	local cam = world.findRecursive( "DefCam" );
	if ( cam ) 
	{
		cam.setPos( 0, 0, 15 );
	}
}


art_init();

