

function run(t)
{
    local top = 0.85;
    local bot = 0.15;
    local scl = 1.0;
    for ( local i=0; i<6; i++ )
    {
        pos[i] += dx[i]; 
        if ( ( pos[i] > top ) || ( pos[i] < bot ) ) dx[i] = dx[i] * -1;
    }

    cube.setPotential(0,  scl*pos[0],  scl*0.6,  scl*0.5, scl*0.7 );
    cube.setPotential(1,  scl*0.5,  scl*pos[1],  scl*0.4, scl*0.5 );
    cube.setPotential(2,  scl*0.2,  scl*0.7,  scl*pos[2], scl*0.6 );
    cube.setPotential(3,  scl*0.6,  scl*pos[3],  scl*0.6, scl*0.8 );
    cube.setPotential(4,  scl*pos[4], scl*0.6,   scl*0.5, scl*0.4 );
    cube.setPotential(5,  scl*0.2,  scl*0.5,  scl*pos[5], scl*0.6 );
    
    mesh.setVisibility(0);
    cube.update( mesh );
    //~ mesh.recalcSphere();
    mesh.setVisibility(1);
}


function surf_init()
{
    cube  <- Marching("Surf", "Surf.Marching");
    cube.setNumPotentials(6);
    cube.setScale(45);
    cube.setSize(40);

    mesh  <- Mesh("Core", "Core.Mesh");
    mesh.setName( "marching" );
    mesh.setVisibility(0);
    
    cam   <- Camera("Core", "Core.Camera");
    cam.setPos( 15,15,40 );
    cam.setName( "MyCam" );
    
    light <- Light("Core", "Core.Light");
    light.setName("MyLight");
    light.setPos( 15,150,10 );
    light.setColor( 0.8, 1, 0.8 );

    local world = World("Core", "Core.World");   
    local root  = world.getRoot();
    root.link( mesh );
    root.link( cam );
    root.link( light );
    
    pos <- [0.5,0.5,0.6,0.7,0.3,0.5];
    dx  <- [-0.017,0.012,0.051,0.017,-0.0182,0.025];
}


surf_init();
