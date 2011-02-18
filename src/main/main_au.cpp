#include "e6_sys.h"
#include "e6_impl.h"
#include "e6_enums.h"
#include "e6_math.h"
#include "../Audio/Audio.h"
#include "../Midi/Midi.h"

void midiTest( e6::Engine * e )
{
	Midi::Device * device = (Midi::Device *)e->createInterface( "Midi", "Midi.Device" );	
	bool ok = device->init(0);
	while( ok )
	{
	}		

	E_RELEASE(device);	
}



char audio_v( float f )
{
	if ( f<10  ) return '.';
	if ( f<50  ) return '_';
	if ( f<100 ) return '-';
	if ( f<150 ) return '+';
	if ( f<200 ) return '*';
	if ( f<250 ) return 'm';
	if ( f<300 ) return '%';
	return '#';
}

void audioTest( e6::Engine * e )
{
	Audio::DeviceRT * device = (Audio::DeviceRT *)e->createInterface( "Audio", "Audio.DeviceRT" );	
	Audio::Bands   * bands = (Audio::Bands *)e->createInterface( "Audio", "Audio.Bands" );	
	uint n  = bands->numBands();
	bool ok = device->init(0);
	int ntests=500;
	while( ok )
	{
		float *p=0;
		uint nsamp = device->getBuffer( &p );
		if ( ! nsamp )
			break;
		if ( ! bands->update( p, nsamp ) )
			break;
		for ( uint i=0; i<n; i++ )
		{
			//printf( "%8.2f", bands->getBand(i) );
			printf( "%c ", audio_v(bands->getBand(i)) );
		}
		printf( "\n" );
		// cheap sleep:
		for ( uint j=0,k=0; j<0xfffff; j++ )
			k+=7*i;
		// breakout:
		ok = --ntests > 0;
	}		
	E_RELEASE(device);	
	E_RELEASE(bands);	
}




int main(int argc, char **argv)
{

	e6::Engine * e = e6::CreateEngine();	

	audioTest( e );

	E_RELEASE(e);	
	printf("!\n");

	return 0;
}
