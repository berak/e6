
function run(t)
{
    print(".\n");
    if ( ! m ) return;
    //m.setPixelShaderConstant( 10, audio.getOutput(0), audio.getOutput(1), audio.getOutput(2), audio.getOutput(3) );        
    //m.setPixelShaderConstant( 11, audio.getOutput(4), audio.getOutput(5), audio.getOutput(6), audio.getOutput(7) );        
    //local s = "";
    for ( local i=0; i<8; i++ )
    {
        local a = audio.getOutput(i);
        //s += a + " ";
	    m.setPixelShaderConstant( 10 + i, aScale * a, 0,0,0 );        		
    }
    //print( s + "\r\n" );
}

function cleanup()
{
	m     <- null;
	audio <- null;
}


function audio2_init()
{
    local tl = TimeLine("TimeLine", "TimeLine.TimeLine");
    audio <- Filter("Audio", "Audio.BandFilter");
    tl.add( audio );

    aScale <- 0.05;

    world <- World("Core", "Core.World");

    loadWorld( world, "plate.e6" );   

    m <- toMesh(world.findRecursive("lissi"));
    if ( ! m ) return;
    
    m.setVertexShader( loadShader( world, "vs_audio.hlsl" ) );
    m.setPixelShader ( loadShader( world, "ps_audio.hlsl" ) );
}

audio2_init();
