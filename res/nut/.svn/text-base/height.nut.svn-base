

function run(t)
{
    mesh.setVisibility(0);
    height.update( mesh );
    mesh.setVisibility(1);
}


function height_init()
{
    local world = World("Core", "Core.World");   
    local root  = world.getRoot();
    local tex = loadTexture( world, "i1.bmp" );
    
    height  <- HeightField("Surf", "Surf.HeightField");
    height.setTiling(0,200,200,16,16);
    height.setTexture( tex, 0.1 );

    mesh  <- Mesh("Core", "Core.Mesh");
    mesh.setName( "HeightField" );
    mesh.setTexture( 0, tex );
    mesh.setVisibility(0);
    
    cam   <- Camera("Core", "Core.Camera");
    cam.setPos( 50, 20, 50 );
    cam.setName( "MyCam" );
    
    light <- Light("Core", "Core.Light");
    light.setName("MyLight");
    light.setPos( 15,150,10 );
    light.setColor( 0.8, 1, 0.8 );

    root.link( mesh );
    root.link( cam );
    root.link( light );
   
}


height_init();
