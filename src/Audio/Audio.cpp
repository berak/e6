//#include "e6.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_math.h"
#include "../e6/e6_sys.h"
#include "../Audio/Audio.h"
#include "../Audio/fft/fft.h"
#include "../Audio/RtAudio/rt_defs.h"
#include "../Audio/RtAudio/RtAudio.h"
#include "../Audio/Wav/wavread.h"
#include "../Audio/play/pwave.h"
#include "../TimeLine/TimeLine.h"



using e6::uint;
using e6::ClassInfo;




namespace Audio
{
	
	struct CPlayer : e6::CName< Player, CPlayer >
	{
		DSound dsound;
		uint num;
		
		
		CPlayer()
			: num(0)
		{
			dsound.InitDirectSound(GetDesktopWindow());
			dsound.caps();
		}

		virtual ~CPlayer()
		{
		}


		virtual uint loadVoice( const char * fileName ) 
		{
			HRESULT hr = dsound.LoadWaveFile(num,(char*)fileName);
			if ( hr == S_OK )
			{
				num++;
				printf( __FUNCTION__ " %s ok(%d).\n",fileName, num );
				return num - 1;
			}
			return 0;
		}
		virtual uint setState( uint voice, uint state ) 
		{
			if ( ! dsound.is_ok( voice ) )
			{
				printf( __FUNCTION__ " %s not ok(%d).\n",dsound.fName[voice], voice );
				return 0;
			}

			switch(state)
			{
				default: return 0;
				case Player::INACTIVE:
					dsound.Stop( voice, 1 );
					dsound.ClearVoice( voice );
					break;
				case Player::STOPPED:
					dsound.Stop( voice, 1 );
					break;
				case Player::PLAYING:
					dsound.Play( voice, false );
					break;
				case Player::LOOPED:
					dsound.Play( voice, true );
					break;
			}
			return 1;
		}
		virtual uint clear()
		{
			while(num)
			{
				-- num;
				dsound.ClearVoice(num);
			}
			return 1;
		}

		virtual uint setVolume( uint voice, uint vol )
		{
			return ( S_OK == dsound.SetVolume(voice,vol) );
		}
		virtual uint setPan( uint voice, uint pan )
		{
			return ( S_OK == dsound.SetPan(voice,pan) );
		}
		virtual uint setFreq( uint voice, uint f )
		{
			return ( S_OK == dsound.SetFrequency(voice,f) );
		}

	}; // CPlayer

	struct CWavRead : e6::Class< WavRead, CWavRead >
	{
		uint numBytes;
		uint bufferSize;
		uint numInputChannels;
		uint samplingFrequency;
		uint numBuffers;
		uint position; //  sample counter
		unsigned char * soundBytes; // the whole song

		CWavRead()
			: soundBytes(0)
			, bufferSize(1024)
			, numInputChannels(1)
			, samplingFrequency(22050)
			, numBuffers(16)
			, numBytes(0)
			, position(0)
		{}

		~CWavRead()
		{
			cleanup();
		}
			
		void cleanup()
		{
			E_DELETEA ( soundBytes );
		}

		virtual float load( const char *fileName, uint bufSize=1024 )
		{
			position = 0;
			bufferSize = bufSize;

			CWaveSoundRead * wave = new CWaveSoundRead();
			if( FAILED( wave->Open( (char*)fileName ) ) )
			{
				printf( "\nERROR : %s() , could not open soundfile '%s'\n", __FUNCTION__, fileName );
				delete wave;
				return 0;
			}

			// toto formazt check!!
			E_ASSERT( (wave->m_pwfx->wBitsPerSample == 16) && "we need 16-bit unsigned data ! " );

			numBytes = wave->m_ckIn.cksize;
			numInputChannels = wave->m_pwfx->nChannels;
			samplingFrequency = wave->m_pwfx->nSamplesPerSec;

			E_DELETEA ( soundBytes );
			soundBytes = new unsigned char[ numBytes ];
			if(  ! soundBytes )
			{
				printf( "ERROR : %s() , could not alloc %i bytes for soundfile '%s'\n", __FUNCTION__, numBytes, fileName );
				wave->Close();
				delete wave;
				return 0;
			}

			if( FAILED( wave->Read( numBytes, soundBytes, &numBytes ) ) )           
			{
				printf( "ERROR : %s() , could not read data from soundfile '%s' \n", __FUNCTION__, fileName );
				wave->Close();
				delete wave;
				E_DELETEA(soundBytes);
				return 0;
			}

			wave->Close();
			delete wave;

			//return numBytes;
			return numBytes / (2* samplingFrequency*numInputChannels);
		}

		virtual uint getBuffer( float songTime, float **p ) 
		{
			position = uint( songTime * samplingFrequency *2*numInputChannels ) ;
			int bytesLeft = numBytes - position;
			if ( bytesLeft < (int)(bufferSize/2) ) return 0;
			uint nBytes = bufferSize;
			if ( bytesLeft < (int)bufferSize ) nBytes = (uint)bytesLeft;
			char * sbuffer = 0;
			short *sb = (short *)sbuffer;
			static float fsamp[2048];
			float *buf = *p;
			for ( uint i = 0; i < nBytes; i++ )
			{
				fsamp[i] = (float)sb[i];
			}
			*p = fsamp;
			return nBytes;
		}
	};


	struct CDeviceRT : e6::Class< DeviceRT, CDeviceRT >
	{
		uint bufferSize;
		uint numInputChannels;
		uint samplingFrequency;
		uint numBuffers;
		RtAudio * audio;

		CDeviceRT()
			: audio(0)
			, bufferSize(1024)
			, numInputChannels(1)
			, samplingFrequency(22050)
			, numBuffers(16)
		{}

		~CDeviceRT()
		{
			cleanup();
		}
			
		void cleanup()
		{
			if ( audio )
			{
				try
				{
					audio->stopStream();
				}
				catch (RtError &error)
				{
					printf( "ERROR: AudioImporter::cleanup() %s\n", error.getMessage().c_str() );
				}

				audio->closeStream();
				E_DELETE( audio );
			}
		}

		virtual uint init(uint nPrefered, uint bufSize=1024, uint sampFreq=22050) 
		{
			bufferSize=bufSize;
			samplingFrequency = sampFreq;
			uint probeDevice=nPrefered;
			for (  ; probeDevice<8; probeDevice++ )
			{
//				RtAudioDeviceInfo inf = getDeviceInfo(probeDevice);
				// probe device:
				try
				{
					audio = new RtAudio( 0, 0,	probeDevice, 
												numInputChannels, 
												RTAUDIO_SINT16, 
												samplingFrequency, 
												(int*) &bufferSize, 
												numBuffers );

				}
				catch ( RtError &error )
				{
					//printf( "ERROR: AudioImporter::init(%d) %s\n", probeDevice, error.getMessage().c_str() );
					//exit( EXIT_FAILURE );
					//return 0;
				}
			}
			if ( ! audio )
			{
				printf( "ERROR: AudioImporter::init(%d - %d) failed!.\n", nPrefered, probeDevice );
				return 0;
			}

			try
			{
				//audio->setStreamCallback( &inout, this );
				audio->startStream();
			}
			catch ( RtError & error )
			{
				printf( "ERROR: AudioImporter::init(%d) %s\n", probeDevice, error.getMessage().c_str() );
				cleanup();
				return 0;
			}

			printf( "AudioImporter::init(%d)\n", probeDevice );

			return 1;
		}

		virtual uint getBuffer( float **p ) 
		{
			char * sbuffer = 0;
			try {
				sbuffer = audio->getStreamBuffer();
				audio->tickStream(); ///PPP???
			}
			catch ( RtError &error )
			{
				printf( "ERROR: AudioImporter::update() %s", error.getMessage().c_str() );
				cleanup();
				return 0;
			}
			short *sb = (short *)sbuffer;
			static float fsamp[2048];
			for ( uint i = 0; i < bufferSize; i++ )
			{
				fsamp[i] = (float)sb[i];
			}
			*p = fsamp;
			return bufferSize;
		}
	};



	struct CBands
		: e6::Class< Bands, CBands >
	{
		enum NB	{  NUM_BANDS = 8, NUM_FREQ = 512  };
		float _bands[ NUM_BANDS ];
		float _freqs[ NUM_FREQ ];
		float _noise;
		FFT fft;

		CBands()
			: _noise(0.02f)
		{
			fft.Init(NUM_FREQ*2, NUM_FREQ, 1, 1.0f);
			memset( _bands, 0, NUM_BANDS*sizeof(float) );
		}
		virtual uint numBands() const
		{ 
			return NUM_BANDS; 
		}

		virtual float getBand(uint i) const 
		{ 
			E_ASSERT(i< NUM_BANDS);
			return _bands[i];
		}

		float _collect( uint start, uint stop ) 
		{
			float r = 0;
			uint  n = stop - start;
			while(start<=stop)
			{
				float s = _freqs[start];
				if ( s > _noise )
					r += s;
				start++;
			}
			return r/n;
		}

		virtual uint update( const float* samples, uint nSamples ) 
		{
			if ( nSamples < NUM_FREQ*2 ) return 0;;
			fft.time_to_frequency_domain( (float*)samples, _freqs );
			uint start = NUM_FREQ;
			uint stop  = NUM_FREQ;			
			// octave bands:
			//   start from top, collect upper half in 1 band,
			//   then recurse into lower half.
			for ( int i=NUM_BANDS-1; i>=0; i-- )
			{			
				start = start >> 1;
				_bands[i] = _collect( start, stop );
				stop = start;
			}
			return 1;
		}
	};



	struct CAudioBands
		: e6::CName< TimeLine::Filter , CAudioBands >
	{
		CBands  bands;
		CDeviceRT audio;

		CAudioBands() 
		{
			unique("AudioBands");
			audio.init(0);
		}

		virtual ~CAudioBands() 
		{}

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
			return 8;
		}
		virtual uint update( float t )
		{
			float * p = 0;
			uint nb = audio.getBuffer(&p);
			if ( nb < 1024 ) return 0;
			bands.update(p,nb);
			return 1;
		}
		virtual float getOutput( uint i ) const 
		{
			if ( i>7 ) return 0;
			return bands.getBand(i);
		}
		virtual const char * getOutputName( uint i ) const 
		{
			static const char *ss[] = { "band_0","band_1","band_2","band_3","band_4","band_5","band_6","band_7" };
			if ( i<8 )
			{
				return ss[i];
			}
			return 0;
		}

		virtual void *cast( const char * str )
		{
			return _cast(str,"TimeLine.Filter", "e6.Name");
		}
	};
			
} //namespace Audio



extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Audio.Bands",		 "Audio",	Audio::CBands::createInterface,Audio::CBands::classRef	},
		{	"Audio.DeviceRT",	 "Audio",	Audio::CDeviceRT::createSingleton,Audio::CDeviceRT::classRef	},
		{	"Audio.WavRead",	 "Audio",	Audio::CWavRead::createInterface,Audio::CWavRead::classRef	},
		{	"Audio.BandFilter",	 "Audio",	Audio::CAudioBands::createSingleton,Audio::CAudioBands::classRef	},
		{	"Audio.Player",		 "Audio",	Audio::CPlayer::createSingleton,Audio::CPlayer::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 5; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ( "Audio 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
