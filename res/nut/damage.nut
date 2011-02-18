
function run(t)
{
    if ( m == null ) return;
    
    local speed = 0.067;
    local distort = 0.033*(0.5+0.5*sin(t*1.7412*speed));
    local t1 = 0.5 + 0.5 * cos(t * speed);
    local t2 = 0.5 + 0.5 * cos((t+(1-distort))*speed);
    local mix = 0.5 + 0.5 * sin(t*speed);
       
    //~ print(  "> " + t1 + " " + t2 + "\n" );
    m.setVertexShaderConstant( 32, t1,t2, 0, 0 );           // Anim
    m.setPixelShaderConstant( 0, mix, distort, 1.41, 0 );        // Mix, distort,scale
}

function cleanup()
{
	m     <- null;
}


function damage_init()
{
    local world = World("Core", "Core.World");

    loadWorld( world, "plate.e6" );   
    local root = world.getRoot();
    m <- toMesh(root.findRecursive("lissi"));
    if ( m == null ) return;
    
	local tex = loadTexture( world, "Noise.3d.dds" );
	m.setTexture( 1, tex );
    m.setTexture( 0,  loadTexture( world, "decisions2.bmp" ) );
    m.setVertexShader( loadShader( world, "vs_damage.hlsl" ) );
    m.setPixelShader ( loadShader( world, "ps_damage.hlsl" ) );
    
    local c = root.findRecursive("DefCam");
    if ( c )
    {
        c.setPos( 0, 0, 70 );
        c.setRot( 0, 0, 0 );
    }
    run(0.001);
}

damage_init();
