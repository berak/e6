
#include "../e6/e6_impl.h"
#include "Dmx.h"
#include "odmxusb/odmxusb.h"



using e6::uint;
using e6::ClassInfo;


namespace Dmx
{
	struct CDevice
		: public e6::CName< Dmx::Device, CDevice >
	{
        Open_USB_DMX dmx;

        bool deviceOK;
        unsigned char packet[512];

		CDevice::CDevice()
			: deviceOK(0)
		{
			for ( uint i=0; i<512; i++ )
			{
				packet[i] = 0;
			}
		}
	    
	    
		CDevice::~CDevice()
		{
			close();
		}
	    
	    

		bool open()
		{
			int res = dmx.open_dmx_devices();
			if ( res < 0 )
			{
				printf("Error:could not open dmx-device(errcode:%d)",res );
				return 0;
			}

			int ndevs = dmx.get_dmx_device_count() ;
			if ( ! ndevs )
			{
				printf("Error:could not enumerate dmx-device" );
				return 0;
			}

			deviceOK = 1;
			return deviceOK;
		}



		bool close()
		{
			dmx.close_dmx_devices();
			deviceOK = 0;

			return 1;
		}



		bool setValue( int id, float value )
		{
			if ( ! deviceOK ) return 0;

			assert( id < 512 );
			if ( value < 0 ) value = 0;
			if ( value > 1.0 ) value = 1.0;
			packet[id] = (unsigned char)( value * 0xff );

			/// fixme!
			dmx.send_dmx_packet( packet );
			return 1;
		}



		float getValue( int id )
		{
			assert( id < 512 );
			return ( (float)packet[id] ) / 0xff;
		}




	};

};

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Dmx.Device",	 "Dmx",	Dmx::CDevice::createInterface, Dmx::CDevice::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Dmx 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
