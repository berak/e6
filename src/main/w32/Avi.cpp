#include "avi.h"
////////////////////////////////////////////////////////////////////////////////////////////
// Avi:
////////////////////////////////////////////////////////////////////////////////////////////

int Avi::refs = 0;

Avi::Avi() 
:   pfile(0),
    ps(0),
    psCompressed(0)
{
	if ( ! refs )
        AVIFileInit();	
    refs ++;
}


Avi::~Avi()
{
    close();
	refs --;
	if ( ! refs )
        AVIFileExit();
}


DWORD Avi::create( char* filename, int w, int h, int bpp, int fps ) {

#define CHECK_ERR(hr) \
    if ((hr) != AVIERR_OK){\
		close();\
		return (hr);\
	}

    
    BITMAPINFOHEADER bih = { sizeof(BITMAPINFOHEADER), w, h, 1, bpp*8, BI_RGB, w*h*bpp, 0, 0, 0, 0 };

	AVICOMPRESSOPTIONS opts;
	AVICOMPRESSOPTIONS FAR * aopts[1] = {&opts};

	AVISTREAMINFO strhdr;

	DeleteFile(filename);
	HRESULT hr = AVIFileOpen(&pfile,	// returned file pointer
		       filename,		        // file name
		       OF_WRITE | OF_CREATE,	// mode to open file with
		       NULL);					// use handler determined
										// from file extension....
    CHECK_ERR(hr);

	memset(&strhdr, 0, sizeof(strhdr));
	strhdr.fccType                = streamtypeVIDEO;// stream type
	strhdr.fccHandler             = 0;
	strhdr.dwScale                = 1;
	strhdr.dwRate                 = fps;		    // def:25 fps
	strhdr.dwSuggestedBufferSize  = bih.biSizeImage;
	SetRect(&strhdr.rcFrame, 0, 0, bih.biWidth, bih.biHeight);

	// create the stream;
	hr = AVIFileCreateStream(pfile, &ps, &strhdr);	
    CHECK_ERR(hr);

    // get compresor format & options:
	memset(&opts, 0, sizeof(opts));
	if (!AVISaveOptions(NULL, 0, 1, &ps, (LPAVICOMPRESSOPTIONS FAR *) &aopts)){
		close();
		return E_FAIL;
	}
    // create the stream:
	hr = AVIMakeCompressedStream(&psCompressed, ps, &opts, NULL);
    CHECK_ERR(hr);

    // set chosen format:
	hr = AVIStreamSetFormat(psCompressed, 0, &bih, bih.biSize );		       
    CHECK_ERR(hr);

#undef CHECK_ERR
    
	return AVIERR_OK;
}

void Avi::write(int framenum, long size, void *data ){

	HRESULT hr = AVIStreamWrite(
		psCompressed,					// stream pointer
		framenum,						// time of this frame
		1,								// number to write
		data,							// destbits
		size,							// size of this frame
		AVIIF_KEYFRAME,					// flags....
		NULL,
		NULL);
}



void Avi::close(){
	// alles schlieﬂen und freigeben
	if (ps)
		AVIStreamClose(ps);
	ps = 0;

	if (psCompressed)
		AVIStreamClose(psCompressed);
	psCompressed = 0;

	if (pfile)
		AVIFileClose(pfile);
	pfile = 0;
}

