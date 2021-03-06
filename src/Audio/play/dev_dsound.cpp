// pwave dsound

#include "../sound.h"
#include "dev_dsound.h"
#include <stdio.h>


#define COM_RELEASE(x) {if((x)) (x)->Release(); (x)=0;}

void dump(LPDIRECTSOUNDBUFFER buf,const char*name="sound")
{
	LONG v=0,p=0,c=0;
	DWORD s=0,f=0,o=0;
	buf->GetVolume(&v);
	buf->GetPan(&p);
	buf->GetFrequency(&f);
	buf->GetCurrentPosition(&o,0);
	buf->GetStatus(&s);
	//pDSBuffer[active]->GetFormat(&f);
	//pDSBuffer[active]->GetCaps(&c);
	printf( "%s %4d %4d %4d %4d %4d\n", name, v,p,f,o,s );
}




DSound::DSound() 
{
    _error_set   = 0;
    dw_Creation  = 0L;
    pDS          = NULL;
    for ( UINT i=0; i<MAX_SBUF; i++ ) {
        fName[i]     = NULL;
        pDSBuffer[i] = NULL;
    }
}



DSound::~DSound() 
{
    for ( UINT i=0; i<MAX_SBUF; i++ ) 
	{
        fName[i] = NULL;
		SAFE_RELEASE( pDSBuffer[i]);
    }
    SAFE_RELEASE( pDS ); 

	CoUninitialize();
}


void DSound::caps() 
{
    char  tit[80];
    char  tex[2048];
    DSCAPS *wc = new DSCAPS;
    tex[0] = '\0';
    wc->dwFlags = 0;

    if ( pDS ) 
	{
        pDS->GetCaps( wc );
        sprintf( tit, "DSCAPS : %x", wc->dwFlags );
        if ( wc->dwFlags & 0x00000001 )  
            strcat( tex, "\nDSCAPS_PRIMARYMONO" );
        if ( wc->dwFlags & 0x00000002 )
            strcat( tex, "\nDSCAPS_PRIMARYSTEREO" );
        if ( wc->dwFlags & 0x00000004 )  
            strcat( tex, "\nDSCAPS_PRIMARY8BIT" );
        if ( wc->dwFlags & 0x00000008 )  
            strcat( tex, "\nDSCAPS_PRIMARY16BIT");
        if ( wc->dwFlags & 0x00000010 )
            strcat( tex, "\nDSCAPS_CONTINUOUSRATE" );
        if ( wc->dwFlags & 0x00000020 )
            strcat( tex, "\nDSCAPS_EMULDRIVER" );
        if ( wc->dwFlags & 0x00000040 )
            strcat( tex, "\nDSCAPS_CERTIFIED ");
        if ( wc->dwFlags & 0x00000100)
            strcat( tex, "\nDSCAPS_SECONDARYMONO");
        if ( wc->dwFlags & 0x00000200 )
            strcat( tex, "\nDSCAPS_SECONDARYSTEREO");
        if ( wc->dwFlags & 0x00000400 )
            strcat( tex, "\nDSCAPS_SECONDARY8BIT" );
        if ( wc->dwFlags & 0x00000800 )
            strcat( tex, "\nDSCAPS_SECONDARY16BIT");
        //MessageBox( NULL, tex, tit, MB_OK );
		printf( "%s\n", tex );
    } else 
        MessageBox( NULL, "The device was not inititted !\nPlease click on 'Sound -> Open' first !", "ERROR", MB_OK );
}




int DSound::is_ok( UINT voice )
{
    return ( NULL != pDSBuffer[voice] );
}




HRESULT DSound::error( char *s ) 
{
	printf( "\n error %s %x\n", s, GetLastError() );
//	wsprintf( _error, " %s %x", s, GetLastError() );
    _error_set = 1;
    //MessageBox( NULL, s, _error, MB_OK);
    return -1; 
}


void printGUID( const GUID & ng2 )
{
   printf(  "{%x08-%x04-%x04-", ng2.Data1, ng2.Data2, ng2.Data3 );
   for (int i = 0 ; i < 8 ; i++) {
	  if (i == 2)
		 printf("-");
	  printf("%02x", ng2.Data4[i]);
   }
   printf("}\n");
}

BOOL CALLBACK dsEnumCallback(
  LPGUID lpGuid,
  LPCSTR lpcstrDescription,
  LPCSTR lpcstrModule,
  LPVOID lpContext
)
{
	if ( lpGuid )
	{
		printGUID( *lpGuid );
		GUID *pid = (GUID*)lpContext;
		*pid = *lpGuid;
	}
	else
		printf("{0}\n");
	printf("\t[%-30s] [%-20s] [%x]\n", lpcstrDescription, lpcstrModule, lpContext );
	return 1;
};


    
HRESULT DSound::InitDirectSound( HWND hWnd ) 
{
    HRESULT hr;
	GUID guid;

    CoInitialize( NULL );
	DirectSoundEnumerate( dsEnumCallback, &guid );

	// Create IDirectSound using the last enumerated sound device
 //   if( FAILED( hr = DirectSoundCreate( &guid, &pDS, NULL ) ) )
	// Create IDirectSound using the primary sound device
    if( FAILED( hr = DirectSoundCreate( NULL, &pDS, NULL ) ) )
        return error("DirectSoundCreate");

    // Set coop level to DSSCL_PRIORITY
    if( FAILED( hr = pDS->SetCooperativeLevel( hWnd, DSSCL_PRIORITY ) ) )
        return error("SetCooperativeLevel");

    // Get the primary buffer 
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat   = NULL;
    dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER
					   ; //| DSBCAPS_CTRLVOLUME;
       
    LPDIRECTSOUNDBUFFER pDSBPrimary = 0;
    if( FAILED( hr = pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL ) ) )
        return error("Create Primary SoundBuffer");

    // Set primary  format to 22kHz and 16-bit output.
    ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
    wfx.wFormatTag      = WAVE_FORMAT_PCM; 
    wfx.nChannels       = 1; 
    wfx.nSamplesPerSec  = SampleRate; 
    wfx.wBitsPerSample  = 16; 
    wfx.nBlockAlign     = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if( FAILED( hr = pDSBPrimary->SetFormat(&wfx) ) )
        return error("Set Primary Format");

	SAFE_RELEASE( pDSBPrimary );
    return S_OK;
}








HRESULT DSound::AddVoice( UINT num, int nBytes )
{
    HRESULT hr; 

	DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwBufferBytes = nBytes;
    dsbd.lpwfxFormat   = &wfx;
    dsbd.dwFlags       = dw_Creation;
    dsbd.dwFlags      |= DSBCAPS_STATIC;
    dsbd.dwFlags      |= DSBCAPS_CTRLPAN;
    dsbd.dwFlags      |= DSBCAPS_CTRLVOLUME;
    dsbd.dwFlags      |= DSBCAPS_CTRLFREQUENCY;
	dsbd.dwFlags      |= DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwFlags      |= DSBCAPS_GLOBALFOCUS;
    //dsbd.dwFlags      |= DSBCAPS_CTRLPOSITIONNOTIFY;
    dsbd.dwFlags      |= DSBCAPS_CTRL3D;

	SAFE_RELEASE(pDSBuffer[num]);

	if ( S_OK != ( hr = pDS->CreateSoundBuffer( &dsbd, &pDSBuffer[num], NULL ) ) )
        return error( "CreateSoundBuffer" ); 

	return hr;
}

HRESULT DSound::LoadVoice( UINT num, const void * bytes, int nBytes )
{
    HRESULT hr; 

    if( ! pDSBuffer[num] )
        return error("no buffer");
    if( ! bytes )
        return error("no bytes");
    if( ! nBytes )
        return error("no nBytes");

	VOID*   pbData  = NULL;
    VOID*   pbData2 = NULL;
    DWORD   dwLength = 0;
    DWORD   dwLength2 = 0;
    DWORD   dwCurrentPlayCursor = 0;
    DWORD   dwCurrentWriteCursor = 0;

	hr = pDSBuffer[num]->GetCurrentPosition( &dwCurrentPlayCursor, &dwCurrentWriteCursor );

	hr = pDSBuffer[num]->Lock( dwCurrentWriteCursor, nBytes, &pbData, &dwLength, &pbData2, &dwLength2, 0L );
 //       return error("Lock");

	// If the buffer was lost, restore and retry lock. 
	if (DSERR_BUFFERLOST == hr) 
    { 
        hr = pDSBuffer[num]->Restore(); 
		if ( hr != S_OK )
			return error("Restore");
		hr = pDSBuffer[num]->Lock( dwCurrentWriteCursor, nBytes, &pbData, &dwLength, &pbData2, &dwLength2, 0L );
	}	
	if ( hr != S_OK )
        return error("Lock");

	memcpy( pbData, bytes, dwLength );
	if ( pbData2 )
	{
		memcpy( pbData2, bytes, dwLength2 );
	}
    hr = pDSBuffer[num]->Unlock( pbData, dwLength, pbData2, dwLength2 );

	if( hr != S_OK )
	{
		printf(__FUNCTION__ " FillBuffer error\n" );
	}

	return hr;
}



HRESULT DSound::ClearVoice( UINT v ) 
{
    if ( NULL == pDSBuffer[v] )
        return E_FAIL;

    pDSBuffer[v]->Stop();

	SAFE_RELEASE( pDSBuffer[v] );
    return TRUE;
}


BOOL DSound::IsPlaying( UINT v ) 
{
    DWORD dwStatus = 0;

    if ( NULL == pDSBuffer[v] )
        return FALSE;

    pDSBuffer[v]->GetStatus( &dwStatus );
    if( dwStatus & DSBSTATUS_PLAYING )
        return TRUE;
    else 
        return FALSE;
}





HRESULT DSound::Play( UINT v,  BOOL bLooped ) 
{
    if( NULL == pDSBuffer[v] )
        return error( "Soundbuffer empty" );

	HRESULT hr = 0;

    hr = pDSBuffer[v]->SetCurrentPosition(0);
    if( S_OK != hr )
        return error( "SetCurrentPosition" );
	
	hr = pDSBuffer[v]->Play( 0, 0, ( bLooped ? DSBPLAY_LOOPING : 0L ) );
    if( S_OK != hr )
        return error( "Play" );

	//dump(pDSBuffer[v],fName[v]);
    return S_OK;
}



VOID DSound::Stop(UINT v, BOOL bResetPosition ) 
{
    if( NULL == pDSBuffer[v] )
	{
        error( "Soundbuffer empty" );
        return;
	}

    pDSBuffer[v]->Stop();

    if( bResetPosition )
        pDSBuffer[v]->SetCurrentPosition( 0L );    
}



HRESULT DSound::SetFrequency( UINT v, long freq ) 
{
    if( NULL == pDSBuffer[v] )
        return error( "Soundbuffer empty" );
    if( S_OK != ( pDSBuffer[v]->SetFrequency( freq ) ) )
        return error( "Set frequency" );
    return S_OK;
}


HRESULT DSound::SetVolume( UINT v, long vol ) 
{
    if( NULL == pDSBuffer[v] )
        return error( "Soundbuffer empty" );
    if( S_OK != ( pDSBuffer[v]->SetVolume( vol ) ) )
        return error( "Set Volume" );
    return S_OK;
}

HRESULT DSound::SetPan( UINT v, long pan ) 
{
    if( NULL == pDSBuffer[v] )
        return error( "Soundbuffer empty" );
    if( S_OK != ( pDSBuffer[v]->SetPan( pan ) ) )
        return E_FAIL;//error( "Set Pan" );
    return S_OK;
}






/*
HRESULT DSound::Restore( UINT i ) {
    HRESULT hr;
    UINT    v;
    DWORD   dwStatus;
    active = i;
    for( v=0; v<MAX_VBUF; v++ ) {
        if( NULL == pDSBuffer[active][v] )
            return S_OK;

        if( FAILED( hr = pDSBuffer[active][v]->GetStatus( &dwStatus ) ) )
            return hr;

        if( dwStatus & DSBSTATUS_BUFFERLOST ) {
//        error("BUFFER LOST !");
            do  {
                hr = pDSBuffer[active][v]->Restore();
                if( hr == DSERR_BUFFERLOST )
                    Sleep( 10 );
            } while( hr = pDSBuffer[active][v]->Restore() );
        }
    }

    // sorry, have to reload  all the voice-buffers now ...
    if( FAILED( hr = Fill(active) ) )
        return hr;
    return S_OK;
}
*/
