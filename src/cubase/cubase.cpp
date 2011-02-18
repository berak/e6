//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2005/11/15 15:14:03 $
//
// Category     : VST 2.x SDK Samples
// Filename     : e6.cpp
// Created by   : Steinberg Media Technologies
// Description  : Stereo plugin which applies Gain [-oo, 0dB]
//
// © 2005, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#include "cubase.h"



//-------------------------------------------------------------------------------------------------------
enum { nParams = 12 };
struct Param
{
	char  name[32];
	char  label[16];
	float value;
} params[nParams];


//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new e6 (audioMaster);
}

//-------------------------------------------------------------------------------------------------------
e6::e6 (audioMasterCallback audioMaster)
: AudioEffectX (audioMaster, 1, nParams )	// 1 program, 1 parameter only
{
	setNumInputs (0);		// no in
	setNumOutputs (0);		// no out
	setUniqueID ('e6e6');	// identify

	// we don't do audio !
	canProcessReplacing (false);	// supports replacing output
	canDoubleReplacing (false);	// supports double precision processing


	for ( int id=0; id<nParams; id++ ) 
	{
		sprintf( params[id].name,"param_%d", id );
		strcpy( params[id].label, "db" ); 
		params[id].value = 0.0f; 
	}

	vst_strncpy (programName, "Default", kVstMaxProgNameLen);	// default program name

	logFile = fopen("e6.log", "wb" );
	int res = sock.connect( "localhost", 9999 );
	if ( res = SOCKET_ERROR )
	{
		fprintf( logFile, "could not connect to localhost on 9999\n" );
		fflush( logFile );
	}
	else
	{
		fprintf( logFile, "connected to localhost on 9999\n" );
		fflush( logFile );
	}
}

//-------------------------------------------------------------------------------------------------------
e6::~e6 ()
{
	if ( logFile )
	{
		fflush( logFile );
		fclose( logFile );
	}
	logFile = 0;
}


//-------------------------------------------------------------------------------------------------------
VstInt32 e6::canDo(char*text)
{
	if ( !strcmp(text, "receiveVstMidiEvent") )
		return 1;
	if ( !strcmp(text, "receiveVstEvents") )
		return 1;
	return 0;
}

//-----------------------------------------------------------------------------------------
VstInt32 e6::getNumMidiInputChannels ()
{
	return 1; // we are monophonic
}

//-----------------------------------------------------------------------------------------
VstInt32 e6::processEvents (VstEvents* ev)
{
	char buf[8000];
	char buf1[200];
	buf[0] = 0;

	for (long i = 0; i < ev->numEvents; i++)
	{
		if ((ev->events[i])->type != kVstMidiType)
			continue;
		VstMidiEvent* event = (VstMidiEvent*)ev->events[i];
		char* midiData = event->midiData;
		//long status = midiData[0] & 0xf0;		// ignoring channel
		//if (status == 0x90 || status == 0x80)	// we only look at notes
		//{
		//	long note = midiData[1] & 0x7f;
		//	long velocity = midiData[2] & 0x7f;
		//	if (status == 0x80)
		//		velocity = 0;
		//	//if (!velocity && (note == currentNote))
		//	//	noteIsOn = false;	// note off by velocity 0
		//	//else
		//	//	noteOn (note, velocity, event->deltaFrames);
		//}

		unsigned char b0 = (unsigned char)( midiData[0] & 0xff );
		unsigned char b1 = (unsigned char)( midiData[1] & 0xff );
		unsigned char b2 = (unsigned char)( midiData[2] & 0xff );
		sprintf( buf1, "cubase_midi( %d, %d, %d );\r\n", b0, b1, b2 );  
		strcat( buf, buf1 );
		//sprintf( buf, "cubase_midi( %i, %i, %i );\r\n", midiData[0], midiData[1], midiData[2] );  

		//fprintf( logFile, buf ); 
		//fflush( logFile );

		//else if (status == 0xb0 && midiData[1] == 0x7e)	// all notes off
		//	noteIsOn = false;
	}

	if ( buf[0] )
	{
		sock.write( buf );
	}
	Sleep(3);
	return 1;	// want more
}
//-------------------------------------------------------------------------------------------------------
void e6::setProgramName (char* name)
{
	vst_strncpy (programName, name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void e6::getProgramName (char* name)
{
	vst_strncpy (name, programName, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void e6::setParameter (VstInt32 index, float value)
{
	if ( index < nParams )
	{
		params[ index ].value = value;

		char buf[200];
		sprintf( buf, "cubase_param( %i, %2.2f );\n", index, value ); 
		sock.write( buf );

	//	fprintf( logFile, buf ); 
	//	fflush(logFile );

	}
}

//-----------------------------------------------------------------------------------------
float e6::getParameter (VstInt32 index)
{
	if ( index < nParams )
	{
		return params[ index ].value;
	}
	return 0;
}

//-----------------------------------------------------------------------------------------
void e6::getParameterName (VstInt32 index, char* label)
{
	if ( index < nParams )
	{
		vst_strncpy (label, params[ index ].name, kVstMaxParamStrLen);
	}
}

//-----------------------------------------------------------------------------------------
void e6::getParameterDisplay (VstInt32 index, char* text)
{
	if ( index < nParams )
	{
		dB2string (params[ index ].value, text, kVstMaxParamStrLen);
	}
}

//-----------------------------------------------------------------------------------------
void e6::getParameterLabel (VstInt32 index, char* label)
{
	if ( index < nParams )
	{
		vst_strncpy (label, params[ index ].label, kVstMaxParamStrLen);
	}
}

//------------------------------------------------------------------------
bool e6::getEffectName (char* name)
{
	vst_strncpy (name, "e6", kVstMaxEffectNameLen);
	return true;
}

//------------------------------------------------------------------------
bool e6::getProductString (char* text)
{
	vst_strncpy (text, "e6", kVstMaxProductStrLen);
	return true;
}

//------------------------------------------------------------------------
bool e6::getVendorString (char* text)
{
	vst_strncpy (text, "e6 Technologies", kVstMaxVendorStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
VstInt32 e6::getVendorVersion ()
{ 
	return 1234; 
}

VstPlugCategory e6::getPlugCategory()
{
	return kPlugCategEffect;
}

////-----------------------------------------------------------------------------------------
void e6::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
    //float* in1  =  inputs[0];
    //float* in2  =  inputs[1];
    //float* out1 = outputs[0];
    //float* out2 = outputs[1];

    //while (--sampleFrames >= 0)
    //{
    //    (*out1++) = (*in1++) * fGain;
    //    (*out2++) = (*in2++) * fGain;
    //}
}

//-----------------------------------------------------------------------------------------
void e6::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
 //   double* in1  =  inputs[0];
 //   double* in2  =  inputs[1];
 //   double* out1 = outputs[0];
 //   double* out2 = outputs[1];
	//double dGain = fGain;

 //   while (--sampleFrames >= 0)
 //   {
 //       (*out1++) = (*in1++) * dGain;
 //       (*out2++) = (*in2++) * dGain;
 //   }
}
