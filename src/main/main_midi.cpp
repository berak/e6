#include "e6_sys.h"
#include "e6_impl.h"
#include "e6_enums.h"
#include "../Midi/Midi.h"

#include <conio.h>


using e6::uint;

struct MidiCall
	: Midi::Callback
{
	virtual uint onMidiEvent( uint cmd, uint channel, uint b0, uint b1 )
	{
		printf( "%02x\t%02i\t%02i\t%02i\t%i\n", cmd, channel, b0, b1, e6::sys::getMicroSeconds() );
		return 1;
	}
};

int main(int argc, char **argv)
{
	uint deviceID = 0;;
	if ( argc>1 ) deviceID = atoi(argv[1]);

	e6::Engine * e = e6::CreateEngine();	
	Midi::Device * midi = (Midi::Device *)e->createInterface("Midi","Midi.Device");
	Midi::Callback * cb = new MidiCall;
	
	if ( midi->init( deviceID ) )
	{
		printf( "hello, %i.\n", deviceID );
		midi->setCallback( cb );
		while ( ! kbhit() )
		{
			e6::sys::sleep(100);
		}	
	}

	E_RELEASE(midi);	
	E_DELETE(cb);	
	E_RELEASE(e);	
	return 0;
}
