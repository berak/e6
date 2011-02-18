
function cubase_param( index, value )
{
 // print( "cb_param " + index + ", " +  value + "\r\n" );
  if ( index == 5 )
  {
    x <- ( 5.0 - value * 10 );
  }
  else
  if ( index == 0 )
  {
    y <- ( 5.0 - value * 10 );
  }
  else
  if ( index == 3 )
  {
    z <- ( 5.0 - value * 10 );
  }
  else
  {
    return;
  }

  if ( m )
  {
    m.setPos(x,y,z);
  }
}

function cubase_midi( b0,b1,b2 )
{
 // print( "cb_midi  " + b0 + ", "  + b1 + ", "  + b2 + ", " + "\r\n" );
}

function cleanup()
{
  if ( m )
  {
    m <- null;
  }
}

function run(t)
{
}

function cu_init()
{
  local w = World("Core","Core.World");
  loadWorld(w, "cuba.e6");
  m <- w.findRecursive("Cube");
  x <- 0;
  y <- 0;
  z <- 0;
}

cu_init();
