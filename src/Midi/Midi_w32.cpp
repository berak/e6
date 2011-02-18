//#include "e6.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_math.h"

#include "../Midi/Midi.h"
#include "../TimeLine/TimeLine.h"

#include <windows.h>


using e6::uint;
using e6::ClassInfo;




namespace Midi
{
	unsigned int  _pitch[16];
	unsigned char _ctl[16][128];
	unsigned char _note[16][128];
	Callback 	* _call  = 0;
	uint		  _e_cmd = 0;	//! cached event for running status
	uint		  _e_chn = 0;	//! cached event for running status
	uint		  _e_b0  = 0;	//! cached event for running status
	uint		  _e_b1  = 0;	//! cached event for running status


	// event callback:
	void CALLBACK _midiInProc(
		HMIDIIN hMidiIn,  
		UINT wMsg,        
		DWORD dwInstance, 
		DWORD dwParam1,   
		DWORD dwParam2    
	)
	{
		// get the bytes from lparam into array ;-) 
		BYTE  in_buf[4];
		memcpy( in_buf, &dwParam1, 4 );

		if ( in_buf[0] < 128 ) 
		{
			// data<127, runningstatus
			// 2-byte message ( running status on):
			_e_b0 = in_buf[0];        // 1st & 2nd byte
			_e_b1 = in_buf[1];
		} 
		else 
		{
			// 3-byte message ( running status off ):
			_e_chn  = (in_buf[0] & 0x0f); //channels[0-15] !!!
			_e_cmd  = (in_buf[0] & 0xf0);
			_e_b0   = in_buf[1];     //  2nd & 3rd byte
			_e_b1   = in_buf[2];
		}

		switch( _e_cmd )
		{
		case 0x90://note_on
			_note[_e_chn][_e_b0] = _e_b1;
			break;
		case 0xb0://ctl
			_ctl[_e_chn][_e_b0] = _e_b1;
			break;
		case 0xe0://pitch ///PPP fixme!
			_pitch[_e_chn] = (_e_b0<<7) | _e_b1; //float!!!!vl;
			break;
		}
		
		if ( _call )
		{
			_call->onMidiEvent( _e_cmd, _e_chn, _e_b0, _e_b1 );
		}
	}

	struct CDevice : e6::Class< Device, CDevice >
	{
		HMIDIIN       _midiIn;
	    char          _err[1024];

		CDevice()
		{
			_call = 0;
			_clear();
		}
		
		~CDevice()
		{
			_close();
		}


		bool _error( MMRESULT res ) 
		{
			if ( ! res )
				return 1;
			midiInGetErrorText( res, _err, 1024 );
			printf( "Midi Error: %s\n", _err );
			return 0; //MessageBox( NULL, _err, "midi in", MB_OK );     
		}

		bool _clear()
		{
			_midiIn = 0;
			_e_chn = 0;
			_e_cmd = 0;
			_e_b0 = 0;
			_e_b1 = 0;

			_err[0] = 0;

			memset( _ctl,   0, 128*16 );
			memset( _note,  0, 128*16 );
			memset( _pitch, 0, 16*sizeof(uint) );

			return 1;
		}


		bool  _open( uint _deviceID )
		{
			if ( _midiIn ) 
			{
				return true; // shared
			}

			MMRESULT res = 0;
			DWORD_PTR hinst = (DWORD_PTR)::GetModuleHandle(NULL);

			res = midiInOpen( &_midiIn, _deviceID, (DWORD_PTR)_midiInProc, hinst, CALLBACK_FUNCTION );
			if ( res ) return _error( res );   

			res = midiInStart( _midiIn );
			return _error( res ) ;   
		}


		bool _close() 
		{
			MMRESULT res;
			if ( ! _midiIn ) return 0;
			res = midiInStop( _midiIn );
			if ( res ) return _error( res );   
			res = midiInClose( _midiIn );
			if ( res ) return _error( res );   
			_midiIn = 0;
			_call = 0;
			return 1;
		}



		bool _reset(uint _deviceID) 
		{
			MMRESULT res;
			if ( ! _midiIn ) return 0;
			res = midiInStop( _midiIn );
			if ( res ) return _error( res );   
			res = midiInReset( _midiIn );
			if ( res ) return _error( res );   
			res = midiInStart( _midiIn );
			return  _error( res );   
		}


		bool _getLastError(char * buf)
		{
			if ( _err[0] == 0 ) return false;
			strcpy( buf, _err );
			_err[0] = 0;
			return 1;
		}

		
		virtual uint init( uint n )
		{
			_clear();
			uint r = _open(n);
			if ( ! r )
			{
				printf(__FUNCTION__ " : no device for id %i.\n",n);
				for ( uint i=0; i<8; i++ )
				{
					if ( i==n ) continue;
					r = _open( i );
					if ( r )
					{
						printf(__FUNCTION__ " : created device %i instead.\n",i);
						break;
					}
				}
			}
			return r;
		}

		
		virtual uint setCallback(Callback * cb) 	
		{
			_call = cb;
			return 1;
		}

		
		virtual uint getState( uint cmd, uint channel, uint b0 )	
		{
			switch( cmd )
			{
				case 0x90:
					return _note[channel][b0];
				case 0xb0:
					return _ctl[channel][b0];
				case 0xe0:
					return _pitch[channel];
			}
			return 0;
		}

	}; // CDevice

	
	struct CMidiInput
		: e6::CName< MidiInput , CMidiInput >
	{
		CDevice dev;		
		struct Item
		{
			uint channel;
			uint event;
			uint key;
			uint val;

			Item(uint c,uint e,uint k=0,uint v=0)
				: channel(c)
				, event(e)
				, key(k)
				, val(v)
			{
			}
		};
		std::vector<Item> items;

		CMidiInput() 
		{
			unique("MidiInput");
			dev.init(0);
		}

		virtual ~CMidiInput() 
		{
		}

		virtual uint addEvent( uint event, uint channel, uint item )
		{
			items.push_back( Item(channel, event, item, 0) );
			return 0;
		}
		virtual uint numInputs() const
		{
			return 0;
		}
		virtual uint setInput( uint i, float v ) 
		{
			return 0;
		}
		virtual const char * getInputName( uint i ) const 
		{
			return 0;
		}
		virtual uint numOutputs() const 
		{
			return items.size();
		}
		virtual uint update( float t )
		{
			for ( uint i=0; i<items.size(); i++ ) 
			{
				items[i].val = dev.getState( items[i].event, items[i].channel, items[i].key ); 
			}
			return 1;
		}
		virtual float getOutput( uint i ) const 
		{
			if ( i>=items.size() ) return 0;
			return items[i].val / 127.0;
		}
		virtual const char * getOutputName( uint i ) const 
		{
			static char s[64];
			if ( i<items.size() )
			{
				s[0] = 0;
				switch( items[i].event )
				{
					case 0x90:
						sprintf(s, "note_%02i", items[i].key );
						break;
					case 0xb0:
						sprintf(s, "ctrl_%02i", items[i].key );
						break;
					default:
						sprintf(s, "[%02x]_%02i", items[i].event, items[i].key );
						break;							
				}
				return s;
			}
			return 0;
		}

		virtual void *cast( const char * str )
		{
			return _cast(str,"TimeLine.Filter", "e6.Name");
		}
		
	}; // CMidiInput
			
} // namespace Midi



extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Midi.Device",	 	"Midi",	Midi::CDevice::createInterface,Midi::CDevice::classRef	},
		{	"Midi.Input",		"Midi",	Midi::CMidiInput::createInterface,Midi::CMidiInput::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 2; // classses
}


#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ( "Midi 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
