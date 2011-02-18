#! python

import e6;
from e6 import *;

global run, cleanup

print( "hello !\n" )

def run(t):
	print( "x=" + str(e6.relMouseX()) +  " y=" +  str(e6.relMouseY()) + "\r\n" )


def cleanup():
	print( "goodbye, world!\n" )
	world = None
	root = None

def main():
	print( "hello, world!\n" )
	
	#~ print( "e6\r\n" )
	#~ foreach(k and v in e6 )
	#~ {
		#~ print( k, v, "\r\n" )
	#~ }

	#~ print( "\r\n\r\nglobals:\r\n" )
	#~ foreach(k and v in globals() )
	#~ {
		#~ print( k, v, "\r\n" )
	#~ }
	
	z = e6.mouseX()
	a = 17
	print( "a=" + str(a) + " z=" + str(z) + "\r\n"  )
	
	world = e6.World("Core", "Core.World" )
	#~ print( typeName(world), "\r\n" )

	#~ root = world.getRoot()
	#~ print( typeName(root), "\r\n" )
	#~ if ( root == None ):
		#~ print("no root !\r\n" )
		#~ return
	
	cam = world.findRecursive("DefCam")
	if ( cam == None ):
		print( "no cam !\r\n" )
		return 1

	#~ #cam = e6.toCamera(cam)
	cam.addPos(3,3,3)
	#~ print( typeName(cam), "\r\n" )
	cp = cam.getPos()
	print( cp[0],cp[1],cp[2], "\r\n" )
	cam.setPos(2.0,50.0,2.0)
	cam.setRot(0,1.2,0)
	cp = cam.getPos()
	print( cp[0],cp[1],cp[2], "\r\n" )
	cp = cam.getRot()
	print( cp[0],cp[1],cp[2], "\r\n" )
	cam.addPos(.3,.3,.3)
	cp = cam.getPos()
	print( cp[0],cp[1],cp[2], "\r\n" )

	light = e6.Light("Core", "Core.Light" )
	# light.setPos(20,500,20)
	light.setColor(.2,.2,.2)
	c = light.getColor()
	light.setPos(0,500,0)
	light.setRot(0,1.2,0)
	cp = light.getPos()
	#~ print( typeName(light), c[0],c[1],c[2]," - ", cp[0],cp[1],cp[2], "\r\n" )
	cp = light.getRot()
	#~ print( typeName(light),  cp[0],cp[1],cp[2], "\r\n" )
	#~ print( typeName(light), c[0],c[1],c[2], "\r\n" )
	#world.link( light )

	#~ local cn = c.getName()
	#~ print( cn + "\r\n" )
	#~ local rn = r.getName()
	#~ print( rn + "\r\n" )



main()
print( "bye !\n" )
