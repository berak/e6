
#include "FGraph.h"
#include <stdio.h>
#include <string.h>
//#include <mtype.h>

#ifdef TL_DEBUG
    FILE *_lopen() {FILE *lf=fopen("_dbg.txt","w");fprintf(lf,"%s\n\n",__argv[0]);return lf;}
    FILE *_lf = _lopen();
    void tl_trace( FILE *out, char *f, int l, char *fmt, ... ) {
        static char buffer[2048];
        buffer[0]=0;
        va_list args;
        va_start( args, fmt );
        vsprintf( buffer, fmt, args );
        va_end( args );
        fprintf(out, "%s(%d):\t",f,l);   
        fprintf(out, buffer);   
        fflush (out);
		printf( buffer );
    }
#endif

#ifdef TL_USE_DX
 #include <qedit.h>
 bool FGraph::saveGrf( char *szFile )
 {
    if ( ! szFile ) {
        hr=E_POINTER;
        TRACE_HR(hr);
        return 0; 
    }
    IXml2Dex *pXML = NULL; 
    TL_COCREATE(CLSID_Xml2Dex, IID_IXml2Dex, pXML);
    if ( ! pXML ) {
        // dx8 no found. nt4?
        hr=E_NOINTERFACE;
        TRACE_HR(hr);
        return 0;
    }
    // Convert the file name to a wide-character string.
    WCHAR wstr[200];
    MultiByteToWideChar(CP_ACP, 0, szFile, -1, wstr, MAX_PATH); 
    
    hr = pXML->WriteGrfFile( pGraph, wstr );
    TRACE_HR(hr);
    TL_RELEASE( pXML );
    return 1;
 }
#endif


static struct x_w32_video
{
	x_w32_video() 
	{
		CoInitialize(NULL);
	}
	~x_w32_video() 
	{
		CoUninitialize();
	}
} x__w32__video_singleton;


//
// helper:
//
//! get error string
TCHAR * am_error(HRESULT hr){
    static TCHAR str[0x200];
    str[0]=0;
    AMGetErrorText(hr,str,0x200);
    TCHAR *n=str;
    while (*n) { // nuke ret-chars: 
        if ( *n=='\r'|| *n=='\n') *n = ' ';
        n++;
    }
    return str;
}    





GUID fromStr( const char *gstr ) 
{
    GUID cls = {0};
    WCHAR ws[200];
    a2w( gstr, ws );
    CLSIDFromString( ws, &cls );
    return cls;
}

////////////////////////////////////////////////////////////////////////////////////
// intialization  //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
HRESULT FGraph::hr = 0;

FGraph::FGraph()
: pGraph(0)
, pMediaControl(0)
, pMP(0)
, pivw(0)
, pEvent(0)
, pBuilder(0)
, rot(0)
{
//    InitializeCriticalSection (&cs); 
}


FGraph::~FGraph()
{
	clear();
}

int FGraph::clear() 
{
	stop();

    removeFromRot();

    TL_RELEASE ( pEvent );
    TL_RELEASE ( pivw );
    TL_RELEASE ( pMP );
    TL_RELEASE ( pMediaControl );
    TL_RELEASE ( pGraph );
    TL_RELEASE( pBuilder );
    return 1;
}


int FGraph::setup(IGraphBuilder *pGr) 
{    
    //PPP clear(); ???
    if ( ! pGr ) {
        TL_COCREATE( CLSID_FilterGraph, IID_IGraphBuilder, pGraph );
    } else {
        pGraph = pGr;
    }

    if ( ! pGraph )
        return E_FAIL;

    TL_QUERY( pGraph, IID_IMediaControl,  pMediaControl );   
    TL_QUERY( pGraph, IID_IMediaPosition, pMP );
    addToRot();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// graph control ///////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
double FGraph::pos()
{  
    hr = 0;
    REFTIME pll = 0;
    if (pMP)
        hr = pMP->get_CurrentPosition( &pll );
    TRACE_HR(hr);
    return pll;
}

void FGraph::wait( long t )
{  
    if (!isValid())
        return;

    IMediaEvent *pEvent = NULL;
    TL_QUERY(pGraph, IID_IMediaEvent, pEvent);
 
    long evCode=0;
    hr = pEvent->WaitForCompletion( t, &evCode );
    TRACE_HR(hr);

    TL_RELEASE(pEvent);
}

double FGraph::length()
{  
    double len=0;
    if (!isValid())
        return 0;   
    hr = pMP->get_Duration(&len);
    TRACE_HR(hr);
    return len;
}

void FGraph::setPlayRate(double rate)
{  
    if (!isValid())
        return;
    hr = pMP->put_Rate(rate);
    TRACE_HR(hr);
}


//! start from last position, but rewind if near the end
int FGraph::play()
{  
    if (!isValid())
        return 0;
    
    REFTIME tCurrent=0, tLength=0xffff;
    hr = pMP->get_Duration(&tLength);
    TRACE_HR(hr);
    if (SUCCEEDED(hr)) {
        hr = pMP->get_CurrentPosition(&tCurrent);
        TRACE_HR(hr);
        if (SUCCEEDED(hr)) {
            // within 10.th sec of end? (or past end?)
            if ((tLength - tCurrent) < 0.1) {
                hr =pMP->put_CurrentPosition(0);
               // TRACE_HR(hr);
            }
        }
    }
    hr = pMediaControl->Run();

    if ( hr != 0x01 )     TRACE_HR(hr);
	return ( hr == S_OK || hr == S_FALSE );
}

int FGraph::stop()
{
    hr = 0;
    if (isValid())
	{
        hr = pMediaControl->Stop();
	    TRACE_HR(hr);
	}

    if ( pBuilder ) 
        TL_RELEASE( pBuilder );
	return ( hr == S_OK );
}

int FGraph::pause()
{
    hr = 0;
    if (isValid())
        hr = pMediaControl->Pause();
    TRACE_HR(hr);
	return ( hr == S_OK );
}

int FGraph::wind( double t )
{
    hr = 0;
    if ( pMediaControl )
        hr = pMediaControl->Pause();
    if ( pMP )
        hr = pMP->put_CurrentPosition( t );
    TRACE_HR(hr);
	return ( hr == S_OK );
}




void FGraph::stepForward(long t) 
{
    if ( !isValid() ) return;

    /*
    IVideoFrameStep *vst = 0;
    TL_QUERY( pGraph, IID_IVideoFrameStep, vst );
    if ( vst ) {   
        hr = vst->CanStep(t,0);
        if ( ! hr )  
            hr = vst->Step(t,0);
    
        TL_RELEASE(vst);
    } else 
    */
    {
        double p = pos() + (double)t/18.0;
        if ( p>0.0 && p<length() )
            wind( p );
    }
}







int  FGraph::readGrf( char *path )
{   
    if ( !path||path[0]==0 ) {
        hr = E_POINTER;
        TRACE_HR(hr);
        return 0;
    }

    setup(0);

    if (!addGrf(path)) {
        //MessageBox(GetActiveWindow(),"Error: Could not open file",path, 0);
        clear();
        return 0;
    }
    return 1;
}

int  FGraph::addGrf( char *path )
{   
    if ( !path||path[0]==0 ) {
        hr = E_POINTER;
        TRACE_HR(hr);
        return 0;
    }

    AutoCritSec lock(&cs);

    if ( ! isValid() ) 
        setup(0);

    WCHAR wsz[180];
    a2w( path, wsz );    
    hr = pGraph->RenderFile(wsz, NULL);
    TRACE_HR(hr);   
    return hr==S_OK;
}


int FGraph::showProps( IBaseFilter *pFilter, HWND parent ) 
{
    //if (!pGraph || !pFilter)
    if (!pFilter)
        return-1;
    hr = 0;    
   
    ISpecifyPropertyPages *pSpecify;
    hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpecify) ;
    TRACE_HR(hr);
    if ( FAILED(hr) )
        return hr;
   
    
    WCHAR *name = L"Props";
    FILTER_INFO FilterInfo;
    hr = pFilter->QueryFilterInfo(&FilterInfo);
    TRACE_HR(hr);
    if ( hr == S_OK ) {
        name = FilterInfo.achName;
        if ( FilterInfo.pGraph )
            FilterInfo.pGraph->Release(); 
    }
    
    CAUUID caGUID;
    hr = pSpecify->GetPages(&caGUID);
    TRACE_HR(hr);
    TL_RELEASE(pSpecify);
    
    hr = OleCreatePropertyFrame(
        parent,                 // Parent window
        0,                      // x (Reserved)
        0,                      // y (Reserved)
        name,                   // Caption for the dialog box
        1,                      // Number of filters
        (IUnknown **)&pFilter,  // Pointer to the filter 
        caGUID.cElems,          // Number of property pages
        caGUID.pElems,          // Pointer to property page CLSIDs
        0,                      // Locale identifier
        0,                      // Reserved
        NULL                    // Reserved
        );
    
    CoTaskMemFree(caGUID.pElems);
    TRACE_HR(hr);
    return 1;
}


HRESULT FGraph::displayProperties( IUnknown * pSrc, HWND hWnd)
{
	ISpecifyPropertyPages * pPages = 0;

	HRESULT hr = pSrc->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pPages);
	if (SUCCEEDED(hr))
	{
		CAUUID caGUID;
		pPages->GetPages(&caGUID);

		OleCreatePropertyFrame(
			hWnd,
			0,
			0,
			L"Property Sheet",
			1,
			&(pSrc),
			caGUID.cElems,
			caGUID.pElems,
			0,
			0,
			NULL);
		CoTaskMemFree(caGUID.pElems);
	}
	TL_RELEASE(	pPages );
	return(hr);
}

////////////////////////////////////////////////////////////////////////////////////
// filter info: ////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
int FGraph::filterName( IBaseFilter *fil, WCHAR *wsz ) 
{
    if ( !wsz ) return 0;
    wsz[0]=0;
    if ( ! fil ) return 0;
    FILTER_INFO inf = {0};
    HRESULT hr = fil->QueryFilterInfo( &inf );
    TRACE_HR(hr);
    if (inf.pGraph)
        inf.pGraph->Release(); //hmxxgnarff@*#*!!
    int wsl=0;
    WCHAR *ws = &wsz[0];
    WCHAR *wi = &inf.achName[0]; 
    while(1) {
        *ws = *wi;
        if ( ! *wi )
            break;
        wi++;
        ws++;
        wsl++;
    }   
    return wsl;
}

int FGraph::filterName( IBaseFilter *fil, char *sz ) 
{
    if ( ! sz ) return 0;
    sz[0]=0;
    if ( ! fil ) return 0;
    WCHAR w[200];
    int r = filterName(fil,w);
    return w2a(w,sz);
}

int FGraph::filterCLSID( IBaseFilter *fil, char *sz ) 
{
    if ( ! sz ) return 0;
    sz[0]=0;
    if ( ! fil ) return 0;

    GUID g;
    HRESULT hr = fil->GetClassID(&g);
    TRACE_HR(hr);
    
    WCHAR *wstr=0; //??? hmmmm, where's the memory??
    StringFromCLSID( g, &wstr );
    return w2a(wstr,sz);
}

int FGraph::filterState( IBaseFilter *fil ) 
{
    if ( ! fil ) return 0;
    FILTER_STATE st=State_Stopped;
    HRESULT hr = fil->GetState(0,&st);
    TRACE_HR(hr);
    return st;
}

////////////////////////////////////////////////////////////////////////////////////
// find existing filtes: ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
IBaseFilter *FGraph::findFilter( char *sz ) 
{
    if ( ! pGraph ) return 0;
    if ( ! sz )     return 0;
    if ( ! sz[0] )  return 0;
    // AutoCritSec lock(&cs);
    
    WCHAR wstr[180]={0};
    a2w( sz, wstr ); 

    IBaseFilter *pFilter=0;
    hr = pGraph->FindFilterByName( wstr, &pFilter );
    TRACE_HR(hr);
    
    #ifdef TL_DEBUG
        tl_trace(_lf,__FILE__,__LINE__,"TL_FIND_N   %p (%s)\n",pFilter, sz);
    #endif

    return ( hr ? 0 : pFilter );
}


IBaseFilter *FGraph::findFilter( GUID *gi ) 
{
    if ( ! pGraph ) return 0;
    
    // AutoCritSec lock(&cs);
    
    IBaseFilter  *pFilter = NULL;
    IEnumFilters *pEnum;
    pGraph->EnumFilters(&pEnum);
    while(pEnum->Next(1, &pFilter, NULL) == S_OK) {
        GUID chk;
        pFilter->GetClassID(&chk);
        if ( memcmp(&chk,gi,sizeof(GUID) ) )
            break;
        pFilter->Release();
    }
    pEnum->Release();
    
    #ifdef TL_DEBUG
        tl_trace(_lf,__FILE__,__LINE__,"TL_FIND_G   pFilter %p\n",pFilter);
    #endif
    return pFilter;
}

////////////////////////////////////////////////////////////////////////////////////
// remove filtes: ///////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
HRESULT FGraph::remove( IBaseFilter *fil, bool nukeDown ) 
{
    if ( !fil || !pGraph ) return E_POINTER;
   
    if ( nukeDown ) 
        tearDown(fil);

    // AutoCritSec lock(&cs);
    #ifdef TL_DEBUG
     char s[200];
     FGraph::filterName(fil,s);
     tl_trace(_lf,__FILE__,__LINE__,"REMOVED : %s\n", s );
    #endif

    return pGraph->RemoveFilter( fil );
}

//! remove anything downstream
HRESULT FGraph::tearDown( IBaseFilter *fil ) 
{
    if ( !fil || !pGraph ) FAIL_RET( E_POINTER );

    // AutoCritSec lock(&cs);

    hr = fil->Stop();
    TRACE_HR(hr);
    
    ULONG u;
    IPin *pP, *pTo;
    IEnumPins *pins = NULL;
    PIN_INFO pininfo;
    hr = fil->EnumPins(&pins);
    pins->Reset();
    int np=0;

    while (hr == NOERROR) {
        hr = pins->Next(1, &pP, &u);
        if (hr == S_OK && pP) {
            pP->ConnectedTo(&pTo);
            if (pTo)	{
                hr = pTo->QueryPinInfo(&pininfo);
                if (hr == NOERROR) {
                    if (pininfo.dir == PINDIR_INPUT)	{
                        // recurse (bottom up):
                        tearDown(pininfo.pFilter);                       

                        #ifdef TL_DEBUG
                         char str[200];
                         FGraph::filterName( pininfo.pFilter,str );
                         tl_trace(_lf,__FILE__,__LINE__,"TEARDOWN %s pin %d\n",str,np );
                        #endif
                        hr = pininfo.pFilter->Stop();
                        TRACE_HR(hr);
                        hr = pGraph->Disconnect(pTo);
                        TRACE_HR(hr);
    					hr = pGraph->Disconnect(pP);
                        TRACE_HR(hr);
                        hr = pGraph->RemoveFilter(pininfo.pFilter);
                        TRACE_HR(hr);
                    }
    				pininfo.pFilter->Release();
                    pininfo.pFilter=0;
                }
                pTo->Release();
                pTo=0;
            }
            pP->Release();
            pP=0;
        }
        np ++;
    }

    if(pins)
        pins->Release();
    return hr;
}

HRESULT FGraph::removeUnconnected() 
{
    if ( ! pGraph ) return 0;
    
    // AutoCritSec lock(&cs);
    
    IBaseFilter  *pFilter = NULL;
    IBaseFilter  *pNuke[100];
    IEnumFilters *pEnum;
    int nNuke = 0;

    hr = pGraph->EnumFilters(&pEnum);
    while( pEnum->Next(1, &pFilter, NULL) == S_OK) {   
        DWORD u;
        IEnumPins *pins = 0;
        IPin *pP=0, *pTo=0;
        hr = pFilter->EnumPins(&pins);
        TRACE_HR(hr);
        if ( !hr ) {       
            while ( !pins->Next(1,&pP,&u) ) { 
                hr = pP->ConnectedTo(&pTo);
                pP->Release();
                if ( !hr && pTo ) { // connected,don't kill
                    pTo->Release();
                    TL_RELEASE(pFilter);
                    break;
                }
            }
            pins->Release();
        }
        if ( pFilter ) // !connected
            pNuke[nNuke++]=pFilter;
    }
    pEnum->Release();   

    while ( nNuke-- ) {
        hr = remove( pNuke[nNuke] );
        TRACE_HR(hr);
        TL_RELEASE( pNuke[nNuke] );
    }

    return hr;
}

HRESULT FGraph::walkDown( IBaseFilter *fil, TL_FILCB fcb, void *user ) 
{
    if ( !fil || !pGraph ) return E_POINTER;

    // AutoCritSec lock(&cs);
    
    if ( ! fcb(fil,user) ) return E_FAIL;

    ULONG u;
    IPin *pP, *pTo;
    IEnumPins *pins = NULL;
    PIN_INFO pininfo;
    
    hr = fil->EnumPins(&pins);
    if ( ! pins ) return E_FAIL;
    pins->Reset();

    while (hr == NOERROR) {
        hr = pins->Next(1, &pP, &u);
        if (hr == S_OK && pP) {
            hr = pP->ConnectedTo(&pTo);
            TRACE_HR(hr);
            if (pTo)	{
                hr = pTo->QueryPinInfo(&pininfo);
                if (hr == NOERROR) {
                    if (pininfo.dir == PINDIR_INPUT)	{
                        // recurse:
                        walkDown(pininfo.pFilter,fcb,user);
                    }
    				pininfo.pFilter->Release();
                }
                pTo->Release();
            }
            pP->Release();
        }
    }
    pins->Release();
    return hr;
}


IBaseFilter *FGraph::getConnection( IBaseFilter *fil, PIN_DIRECTION dir, int n ) 
{
    PIN_INFO pi      = {0};
    IPin    *pin     = 0;
    IPin    *pout    = 0;
    
    while (true) {    
        hr = getPin(fil,dir,n,&pout);
        TRACE_HR(hr);
        if ( ! hr ) break;

        hr = pout->ConnectedTo(&pin);
        TRACE_HR(hr);
        if ( ! hr ) break;
        
        hr = pin->QueryPinInfo(&pi);
        TRACE_HR(hr);
        break;
    }
    TL_RELEASE(pout);
    TL_RELEASE(pin);
    return pi.pFilter; 
}


/** connect pins;
 * <pre>
 *  after.out -> ins.in
 *  ins.out   -> old.in
 *  reconnects after.out to old.in if insert failed !
 * </pre>
**/

HRESULT FGraph::insert( IBaseFilter *after, IBaseFilter *ins )
{
    if ( !pGraph|| !after || !ins )
        return E_POINTER;

    AutoCritSec lock(&cs);

    HRESULT h0,h1;

    IBaseFilter *con = getConnection(after,PINDIR_OUTPUT,0);

    h0 = connect( after, ins );
    TRACE_HR(h0);

    if ( con ) {
        h1 = connect( (h0?after:ins), con );
        TRACE_HR(h1);
    }

    #ifdef TL_DEBUG
        char sz[200];
        filterName( after,sz ); 
        fprintf(_lf,"INSERTING %s %s",sz,(h0?"--":">>"));
        filterName( ins,sz ); 
        fprintf(_lf," %s",sz);
        if ( con ) {
            filterName( con,sz ); 
            fprintf(_lf," %s %s\n",(h1?"--":">>"),sz);
        }
    #endif

    TL_RELEASE(con);
    return ( hr = h0 ? h0 : h1 );    
}


HRESULT FGraph::connect( IBaseFilter*out, IBaseFilter*in, const GUID & guid ) 
{
    if ( !pGraph|| !out || !in )
        return E_POINTER;

    AutoCritSec lock(&cs);   

    hr = 0;
    IPin *pin  = 0;
    IPin *pout = 0;
    IPin *pcon = 0;

    hr = getPin( out, PINDIR_OUTPUT, guid, &pout );
    TRACE_HR(hr);
    if ( hr == S_OK ) {
        // output
        hr = pout->ConnectedTo( &pcon );
        // TRACE_HR(hr);

        if ( hr == S_OK && pcon ) {
            // break connection on both ends:
            hr = pGraph->Disconnect( pout );
            hr = pGraph->Disconnect( pcon );
        }
        TL_RELEASE(pcon);

        // input
        hr = getPin( in, PINDIR_INPUT, guid, &pin );
        TRACE_HR(hr);
        if ( hr == S_OK ) {
            hr = pin->ConnectedTo( &pcon );
            // TRACE_HR(hr);

            if ( hr == S_OK && pcon ) {
                // break connection on both ends:
                hr = pGraph->Disconnect( pin );
                hr = pGraph->Disconnect( pcon );
            }

            hr = pGraph->Connect( pout, pin );
            TRACE_HR(hr);
            // do_something_here ?
            TL_RELEASE(pcon);
            TL_RELEASE(pin);
        }
        TL_RELEASE(pout);
    }

    return hr;
}

HRESULT FGraph::connect( IBaseFilter*out, IBaseFilter*in, int no, int ni ) 
{
    if ( !pGraph|| !out || !in )
        return E_POINTER;

    AutoCritSec lock(&cs);   

    hr = 0;
    IPin *pin  = 0;
    IPin *pout = 0;
    IPin *pcon = 0;

    hr = getPin( out, PINDIR_OUTPUT, no, &pout );
    TRACE_HR(hr);
    if ( hr == S_OK ) {
        // output
        hr = pout->ConnectedTo( &pcon );
        // TRACE_HR(hr);

        if ( hr == S_OK && pcon ) {
            // break connection on both ends:
            hr = pGraph->Disconnect( pout );
            hr = pGraph->Disconnect( pcon );
        }
        TL_RELEASE(pcon);

        // input
        hr = getPin( in, PINDIR_INPUT, ni, &pin );
        TRACE_HR(hr);
        if ( hr == S_OK ) {
            hr = pin->ConnectedTo( &pcon );
            // TRACE_HR(hr);

            if ( hr == S_OK && pcon ) {
                // break connection on both ends:
                hr = pGraph->Disconnect( pin );
                hr = pGraph->Disconnect( pcon );
            }

            hr = pGraph->Connect( pout, pin );
            TRACE_HR(hr);
            // do_something_here ?
            TL_RELEASE(pcon);
            TL_RELEASE(pin);
        }
        TL_RELEASE(pout);
    }
    #ifdef TL_DEBUG
        char sz[200];
        filterName( out,sz ); 
        fprintf(_lf,"CONNECTING %s %s",sz,(hr?"--":">>"));
        filterName( in,sz ); 
        fprintf(_lf," %s %d %d\n",sz,no,ni);
    #endif

    return hr;
}



//
// PINDIR_INPUT or PINDIR_OUTPUT, 
// rev: or PINDIR_ALL;
//
HRESULT FGraph::getPin( IBaseFilter *pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin)
{
    *ppPin = NULL;

    IEnumPins *pEnum=0;
    hr = pFilter->EnumPins(&pEnum);
    TRACE_HR(hr);
    
    if ( hr == S_OK ) {
        ULONG ulFound=0;
        IPin *pPin=0;
        while( S_OK == (hr=pEnum->Next(1, &pPin, &ulFound)) ) {
            PIN_DIRECTION pindir = (PIN_DIRECTION)3; // 3:don't care
            if ( dirrequired != (PIN_DIRECTION)3 )
                hr = pPin->QueryDirection(&pindir);
            if ( pindir == dirrequired ) {
                if ( iNum == 0 ) {
                    *ppPin = pPin;
                    hr = S_OK;
                    break;
                }
                iNum --;
            } 
            pPin->Release();
        } 
        pEnum->Release();
    }
    #ifdef TL_DEBUG
        tl_trace(_lf,__FILE__,__LINE__,"TL_GETPIN   pPin %p\n",*ppPin);
    #endif

  //  TRACE_HR(hr);
    return hr;
}



//
// strict PIN_DIRECTION.
//
HRESULT FGraph::getPin( IBaseFilter *pFilter, PIN_DIRECTION dirrequired,  const GUID & guid, IPin **ppPin)
{
    *ppPin = NULL;
	DWORD ulFound = 0;
    IEnumPins *pEnum=0;
    hr = pFilter->EnumPins(&pEnum);
    TRACE_HR(hr);
    
    if ( hr == S_OK ) 
	{
        IPin *pPin=0;
        while( S_OK == (hr=pEnum->Next(1, &pPin, &ulFound)) ) 
		{
			PIN_DIRECTION pindir;
            hr = pPin->QueryDirection(&pindir);
            if ( pindir == dirrequired ) 
			{
				AM_MEDIA_TYPE mt = {0};
				hr = pPin->ConnectionMediaType( &mt );
				if ( hr == S_OK )
                {
					if ( IsEqualGUID( mt.majortype, guid ) )
					{
						*ppPin = pPin;
						hr = S_OK;
						break;
					}
                }                   
            } 
            pPin->Release();
        } 
        pEnum->Release();
    }
    #ifdef TL_DEBUG
        tl_trace(_lf,__FILE__,__LINE__,"TL_GETPIN   pPin %p\n",*ppPin);
    #endif

  //  TRACE_HR(hr);
    return hr;
}




// returns FgClass enum
int FGraph::enumPins( IBaseFilter *fil, TL_PINCB pf, void *user )
{
    int res = 0;
    IEnumPins *pEnum=0;
    hr = fil->EnumPins(&pEnum);
    TRACE_HR(hr);
    if ( hr == S_OK ) {
        ULONG ulFound=0;
        IPin *pPin=0;
        hr = E_FAIL;
        while( S_OK == pEnum->Next(1, &pPin, &ulFound) ) {
            PIN_DIRECTION pindir = (PIN_DIRECTION)3; // query all
            pPin->QueryDirection(&pindir);
            if ( pindir == PINDIR_OUTPUT )
                res |= FG_SOURCE;
            if ( pindir == PINDIR_INPUT )
                res |= FG_SINK;
            if ( pf )
                pf( pPin, user );
            pPin->Release();
        } 
        pEnum->Release();
    }
    return res;
}


////////////////////////////////////////////////////////////////////////////////////
// query system for existing filtes: ///////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
int FGraph::enumFilters( bool(*fn)(IBaseFilter*,void*), void *ptr ) 
{
    if ( ! pGraph ) return 0;
    
    // AutoCritSec lock(&cs);
    
    int n=0, ok=0;
    IBaseFilter  *pFilter = NULL;
    IEnumFilters *pEnum;

    pGraph->EnumFilters(&pEnum);
    while(pEnum->Next(1, &pFilter, NULL) == S_OK)
    {
        ok = fn( pFilter, ptr );
        pFilter->Release();
        if ( ! ok )
            break;
        n ++;
    }
    pEnum->Release();
    
    return n;
}


HRESULT FGraph::getProp( IMoniker *pMoniker, WCHAR *key, char *val ) 
{
    IPropertyBag *pBag=0;
    hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
    if(SUCCEEDED(hr))
    {
        VARIANT var;
        var.vt = VT_BSTR;
        hr = pBag->Read(key, &var, NULL);
        if (SUCCEEDED(hr))
            b2a(var.bstrVal,val);
        
        pBag->Release();
    }
    TRACE_HR(hr);
    return hr;
}


void FGraph::sysEnumMoniker(IEnumMoniker *pEnum, TL_MONICB fn, void *user  )
{               
    hr;  
    ULONG cFetched;
    IMoniker *pMoniker = 0;
    while(S_OK == pEnum->Next(1, &pMoniker, &cFetched))
    {
        IPropertyBag *pBag;
        
        BSTR str;
        hr = pMoniker->GetDisplayName(0, 0, &str );
        char sz[5][300] = {0};
        char *szp[] = {sz[0],sz[1],sz[2],sz[3]};
        w2a(str,sz[3]);
        //SysFreeString(str);

        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        TRACE_HR(hr);
        if(SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->Read(L"FriendlyName", &var, NULL);
            TRACE_HR(hr);
            if (SUCCEEDED(hr))
            {
                w2a(var.bstrVal,sz[0]);
                SysFreeString(var.bstrVal);
            }
            hr = pBag->Read(L"CLSID", &var, NULL);
            if (SUCCEEDED(hr))
            {
                w2a(var.bstrVal,sz[1]);
                SysFreeString(var.bstrVal);
            }
            hr = pBag->Read(L"guid", &var, NULL);
            if (SUCCEEDED(hr))
            {
                w2a(var.bstrVal,sz[2]);
                SysFreeString(var.bstrVal);
            } 
            if ( fn ) 
                fn( szp, user );
            
            pBag->Release();
        }
       
        pMoniker->Release();
    }
}

void FGraph::sysEnumCategory(REFCLSID cls, TL_MONICB fn, void *user  )
{       
   
    ICreateDevEnum *pCreateDevEnum = 0;
    IEnumMoniker   *pEnumMoniker   = 0;
    
    
    TL_COCREATE(CLSID_SystemDeviceEnum, IID_ICreateDevEnum, pCreateDevEnum);
    
    hr = pCreateDevEnum->CreateClassEnumerator( cls, &pEnumMoniker, 0);            // Use CLSID_VideoEffects2Category for transitions.   
    TRACE_HR(hr);

    if ( !hr && pEnumMoniker ) 
        sysEnumMoniker( pEnumMoniker, fn, user );

    TL_RELEASE(pEnumMoniker);
    TL_RELEASE(pCreateDevEnum);
}

void FGraph::searchFilterMapper( bool isVideo, bool hasInput, bool hasOutput, bool exact, bool render, TL_MONICB fn, const GUID *mtype )
{
    hr=0;
    IFilterMapper2 *pMapper = NULL;
    TL_COCREATE(CLSID_FilterMapper2, IID_IFilterMapper2, pMapper);    

    GUID arrayInTypes[2];
    arrayInTypes[0] = isVideo ? MEDIATYPE_Video : MEDIATYPE_Audio;
    arrayInTypes[1] = mtype   ? (*mtype)        : MEDIATYPE_NULL;

    IEnumMoniker   *pEnum = NULL;
    hr = pMapper->EnumMatchingFilters(
            &pEnum,
            0,                         // Reserved.
            exact,                     // Use exact match?
            0,                         // Minimum merit.
            hasInput,                  // At least one input pin?
            hasInput,                  // Number of major type/subtype pairs for input.
           (hasInput?arrayInTypes:0),  // Array of major type/subtype pairs for input.
            NULL,                      // Input medium.
            NULL,                      // Input pin category.
            render,                    // Must be a renderer?
            hasOutput,                 // At least one output pin?
            hasOutput,                 // Number of major type/subtype pairs for output.
            (hasOutput?arrayInTypes:0),// Array of major type/subtype pairs for output.
            NULL,                      // Output medium.
            NULL);                     // Output pin category.

    sysEnumMoniker( pEnum, fn, 0 );

    TL_RELEASE( pEnum );
    TL_RELEASE( pMapper );
}


//
//! build the downstream graph (preview renderer)
//

HRESULT FGraph::render( IBaseFilter *flt, const GUID & mt )
{
    if ( !flt ) return E_POINTER;
    if ( filterState(flt) != State_Stopped )
        hr = flt->Stop();

	IPin *pin = 0;   
    hr = getPin( flt, PINDIR_OUTPUT, mt, &pin );
    TRACE_HR(hr);

    if ( hr == S_OK ) {
        hr = pGraph->Render( pin );    
        TRACE_HR(hr);
        TL_RELEASE( pin );
    }
    return hr;
}

HRESULT FGraph::render( IBaseFilter *flt, int pinNo )
{
    if ( !flt ) return E_POINTER;
/*    
    if ( filterState(flt) != State_Stopped )
        this->stop();
*/
    if ( filterState(flt) != State_Stopped )
        hr = flt->Stop();
    IPin *pin = 0;
    
    hr = getPin( flt, PINDIR_OUTPUT, pinNo, &pin );
    TRACE_HR(hr);

    if ( hr == S_OK ) {
        hr = pGraph->Render( pin );    
        TRACE_HR(hr);
        TL_RELEASE( pin );
    }
    
    //if ( hr ) MessageBox(GetActiveWindow(),am_error(hr),"render",MB_ICONERROR);
    return hr;
}

/*

HRESULT FGraph::insertTee( char *sz, IBaseFilter *pSrc, IBaseFilter *pDst, IBaseFilter *pRend ) 
{
    hr = 0;
    IBaseFilter *tee = createFilter( CLSID_SmartTee , sz );   
    while ( 1 ) {
        
        hr = connect(  pSrc, tee, 0, 0 ) ;
        TRACE_HR(hr);
        if ( hr ) break;

        hr = connect( tee, pDst, 0, 0 ) ;
        TRACE_HR(hr);
        if ( hr ) break;

        if ( pRend ) {
            hr = connect( tee, pRend, 1, 0 ) ;
            TRACE_HR(hr);
        }
        break;
    }
    TL_RELEASE(tee);
    return hr;
}

HRESULT FGraph::insertComp( IBaseFilter *pSrc, IBaseFilter *pDst, IBaseFilter *pComp ) 
{
    hr = 0;
    IBaseFilter *tee = createFilter( CLSID_SmartTee , "Tea Filter" );   
    while ( 1 ) {
        
        hr = connect( pSrc, pComp ) ;
        if ( hr ) break;

        hr = connect( pComp, pDst ) ;
        break;
    }
    TL_RELEASE(comp);
    TRACE_HR(hr);
    return hr;
}
*/
////////////////////////////////////////////////////////////////////////////////////
// capturing ///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
/**
 *<pre>
 * manually build the downstream graph (capture & preview renderer)
 * cap only:
 *     a_src - (comp) -
 *     v_src - (comp) - mux - file
 *     
 * preview:
 *                 - a_render
 *     a_src - tee - (comp) -    
 *                            mux - file
 *     v_src - tee - (comp) -
 *                 - v_render 
 *
 * sometimes it's easier to roll my own !! 
 *</pre>
**/ 
bool FGraph::capture2( char *szFile, IUnknown *pSrcVideo, IUnknown *pSrcAudio, long len, char *compressor, bool doPreview ) 
{
    hr = E_FAIL;
    IBaseFilter *vsrc = (IBaseFilter*)pSrcVideo;
    IBaseFilter *asrc = (IBaseFilter*)pSrcAudio;

    IBaseFilter *atee = 0;
    IBaseFilter *vtee = 0;
    IBaseFilter *comp = 0;
    IBaseFilter *out  = 0;
    IBaseFilter *vid  = 0;
    IBaseFilter *aud  = 0;
    IBaseFilter *mux  = createFilter( CLSID_AviDest, "Avi Muxer" );

    while (true) { // enables me to break out anytime:

        if ( doPreview ) {
            
            vtee = createFilter( CLSID_SmartTee, "Video Tee" );
            if ( ! vtee ) break;

            vid = createFilter(CLSID_VideoRenderer,"Video Renderer");
            if ( ! vid ) break;
            
            hr = connect( vsrc, vtee ) ;
            if ( hr ) break;

            hr = connect( vtee, vid, 1, 0 ) ;
            if ( hr ) break;

            vsrc = vtee;
        } 

        if ( compressor ) {
            comp = createFilter( compressor, "Video Compressor" );
            if ( ! comp ) break;

            hr = connect( vsrc, comp ) ;
            if ( hr ) break;

            vsrc = comp;
        }

        hr = connect( vsrc, mux, 0, 0 ) ;
        if ( hr ) break;

        if ( asrc ) {
            if ( doPreview ) {
            
                atee = createFilter( CLSID_SmartTee, "Audio Tea" );
                if ( ! atee ) break;

                aud = createFilter( CLSID_AudioRender, "Audio Renderer" );
                if ( ! aud ) break;
            
                hr = connect( asrc, atee ) ;
                if ( hr ) break;

                hr = connect( atee, aud, 1, 0 ) ;
                if ( hr ) break;

                asrc = atee;
            }
            hr = connect( asrc, mux, 0, 1 ) ;
            if ( hr ) break;
        } 


        out = createFilter( CLSID_FileWriter , "Avi Writer" );
        if ( !out ) break;

        hr = connect( mux, out ) ;
        if ( hr ) break;

        IFileSinkFilter *sink = 0;
        TL_QUERY( out, IID_IFileSinkFilter, sink );
        if ( ! sink ) break;

        AM_MEDIA_TYPE *pmt=0; //??? query output pin?
        WCHAR wsz[200];
        a2w(szFile, wsz);
        hr = sink->SetFileName( wsz,pmt );
        TL_RELEASE(sink);
        
        // we're ok here.
        break;    
    }
    if ( hr ) {
        MessageBox(GetActiveWindow(),am_error(hr),TEXT("capture"),MB_ICONERROR);
        remove( mux );
        remove( out );
        remove( atee );
        remove( vtee );
        remove( comp );
        remove( vid );
        remove( aud );
    }
    TL_RELEASE(out);
    TL_RELEASE(atee);
    TL_RELEASE(vtee);
    TL_RELEASE(vid);
    TL_RELEASE(aud);
    TL_RELEASE(mux);
    TL_RELEASE(comp);
    TRACE_HR(hr);
    return ( hr == S_OK );
}

#include <assert.h>
//
//! build the downstream graph (capture & preview renderer)
// 
bool FGraph::capture( char *szFile, IUnknown *pSrcVideo, IUnknown *pSrcAudio, long len, char *compressor, bool doPreview ) 
{
    if ( (!pSrcVideo)&&(!pSrcAudio) )
        return 0;
assert(0);
    hr = S_OK;
    IBaseFilter            *pMux = 0;
    IBaseFilter            *pVComp = 0;     // Video compressor filter

    TL_RELEASE(pBuilder);
    pBuilder = 0;
    
//    TL_COCREATE( CLSID_CaptureGraphBuilder2, IID_ICaptureGraphBuilder2, pBuilder );
//    if ( ! pBuilder ) 
    {
        // nt4.0 ?
        // IID_ICaptureGraphBuilder2 is in dx8 !
//        MessageBox(GetActiveWindow(),"No Capture GraphBuilder\n(trying manual connect)","capture",MB_ICONERROR);
        return capture2( szFile, pSrcVideo, pSrcAudio,len,compressor,doPreview );
    }


    
    WCHAR wstr[200];
    a2w( szFile, wstr ); 

    hr = pBuilder->SetFiltergraph(pGraph);
    hr = pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, wstr, &pMux, NULL);
    TRACE_HR(hr);
    if ( compressor )
    {
        // Add the compressor filter.
        pVComp = createFilter( compressor, "Compresor");
        if ( pVComp ) {
            // Set video compression properties.
            IAMStreamConfig         *pStreamConfig = NULL;
            hr = pBuilder->FindInterface(NULL, NULL, pVComp, IID_IAMStreamConfig, (void **)&pStreamConfig);                           
            TRACE_HR(hr);
            if ( hr == S_OK ) {
                // Compress at 100k/second data rate.
                AM_MEDIA_TYPE *pmt=0;
                hr = pStreamConfig->GetFormat(&pmt); 
                TRACE_HR(hr);
                if ( hr == S_OK && pmt->formattype == FORMAT_VideoInfo ) 
                {
                    ((VIDEOINFOHEADER *)(pmt->pbFormat))->dwBitRate = 400000;
                    pStreamConfig->SetFormat(pmt); 
                    // DeleteMediaType(pmt);
                }  
            
                // Request key frames every four frames.
                IAMVideoCompression     *pCompress = NULL;
                hr = pStreamConfig->QueryInterface(IID_IAMVideoCompression, (void **)&pCompress);
                TRACE_HR(hr);
                if ( hr == S_OK ) {
                    pCompress->put_KeyFrameRate(4); 
                   // pCompress->put_Quality( 0.2 );
                    pCompress->Release();
                }
                pStreamConfig->Release();    
            }            
        }       
    } 

    
    // video to file
    if ( pSrcVideo )
        hr = pBuilder->RenderStream(
            0,//&PIN_CATEGORY_CAPTURE,  // Pin category
            &MEDIATYPE_Video,       // Media type
            pSrcVideo,              // Capture filter
            pVComp,                 // Compression filter (optional)
            pMux                    // Multiplexer 
          );
    TRACE_HR(hr);

    // audio to file
    if ( pSrcAudio )
        hr = pBuilder->RenderStream(
            &PIN_CATEGORY_CAPTURE,  // Pin category
            &MEDIATYPE_Audio,       // Media type
            pSrcAudio,              // Capture filter
            NULL,                   // Compression filter (no)
            pMux                    // Multiplexer 
          );
    TRACE_HR(hr);
   
    // video to preview
    if ( pSrcVideo && doPreview )
        hr = pBuilder->RenderStream(        
            0, //&PIN_CATEGORY_PREVIEW,         
            &MEDIATYPE_Video, 
            pSrcVideo,              // input
            NULL,                   // No compression filter.
            NULL                    // Default renderer.    
          );
    TRACE_HR(hr);
   
    // audio to preview
    if ( pSrcAudio && doPreview )
        //hr |= pBuilder->RenderStream(
        // don't care if this fails.
        pBuilder->RenderStream(        
            0, //&PIN_CATEGORY_PREVIEW,         
            &MEDIATYPE_Audio, 
            pSrcAudio,              // input
            NULL,                   // No compression filter.
            NULL                    // Default renderer.    
          );

    

    TL_RELEASE(pVComp);
    TL_RELEASE(pMux);

    return pBuilder != 0;
}





////////////////////////////////////////////////////////////////////////////////////
// filter creation//////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
/**
 * create a filter and add it to the graph.  <br>
 * the pins are unconnected !!
**/
IBaseFilter *FGraph::createFilter( char *gstr, char *str ) 
{
    //GUID cls = {0};
    //WCHAR ws[200];
    //a2w( gstr, ws );
    //CLSIDFromString( ws, &cls );

    return createFilter( fromStr(gstr), str );
}

IBaseFilter *FGraph::createFilter( const GUID &cls, char *str ) 
{
    if ( ! pGraph ) 
        setup(0);
    //    return 0;

    hr = E_FAIL;
    IBaseFilter *fil = NULL; // return value;

    TL_COCREATE( cls, IID_IBaseFilter, fil);

    if ( fil ) {
        WCHAR wstr[255];
        a2w( str, wstr );
        hr = pGraph->AddFilter( fil, wstr );
        TRACE_HR(hr);
        if ( hr ) {
            TL_RELEASE( fil );    
        }
    }

    #ifdef TL_DEBUG
        fprintf(_lf,"CREATED %s %x\n",str,fil);
    #endif
    return fil;
}



////////////////////////////////////////////////////////////////////////////////////
// devices /////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
IBaseFilter *FGraph::getAudio() 
{
    if ( ! pGraph ) 
        setup(0);
    //    return 0;
    return getFirstDevice( CLSID_AudioInputDeviceCategory, L"Audio in" );
}

IBaseFilter *FGraph::getCam( const char * preferedDevice, int id) 
{
    if ( ! pGraph ) 
        setup(0);
    //    return 0;
  //  return getFirstDevice( CLSID_VideoInputDeviceCategory, L"Video in" );
    return getDevice( CLSID_VideoInputDeviceCategory, preferedDevice, id );
}

bool getStringProp(IPropertyBag *pPropBag, LPCOLESTR pszPropName, char *value)
{
    VARIANT varName;
    VariantInit(&varName);
    HRESULT hr = pPropBag->Read(pszPropName, &varName, 0);
    if (FAILED(hr)) {
        return 0;
    }
//    checkForDShowError(hr, string("DSHelper::getStringProp(")+pszPropName+")::Read PropBag");

    int lenWStr = SysStringLen(varName.bstrVal);
    int lenAStr = WideCharToMultiByte(CP_ACP, 0, varName.bstrVal, lenWStr, 0, 0, NULL, NULL);
    if (lenAStr > 0)
    {
        WideCharToMultiByte(CP_ACP, 0, varName.bstrVal, lenWStr, value, lenAStr, NULL, NULL);
        value[lenAStr] = 0;
    }

    VariantClear(&varName);
    return 1;
}


IBaseFilter *FGraph::getFirstDevice( const GUID &cat, const WCHAR *name ) 
{
    if ( ! pGraph ) 
        return 0;

    hr = 0;
    ULONG cFetched=0;
    IMoniker *pMoniker = NULL;
    IBaseFilter *pSrc = NULL; // return value;
    IEnumMoniker *pClassEnum = NULL;
    ICreateDevEnum *pDevEnum = NULL;
    TL_COCREATE( CLSID_SystemDeviceEnum, IID_ICreateDevEnum, pDevEnum );
   
    hr = pDevEnum->CreateClassEnumerator( cat, &pClassEnum, 0 );
    if ( hr == S_OK ) {
        while (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK){
            hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
            if ( pSrc && hr==S_OK ) {

				IPropertyBag *pPropBag;
				hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag));
				char dev[400];
				getStringProp(pPropBag, L"FriendlyName", dev);
				printf( "dev [0x%08x]:	%s\n", pSrc, dev );
				pPropBag->Release();

                // got the device driver, but is it really connected to a camera or mic ?
                // try to find an output pin :
                //IPin *pin = 0;
				//hr = getPin( pSrc, PINDIR_OUTPUT, 0, &pin);
                //TL_RELEASE( pin ); // just testing..

                WCHAR wstr[200];
                hr = filterName(pSrc,wstr);
                if ( wstr[0] )
                    hr = pGraph->AddFilter( pSrc, wstr );
                else
                    hr = pGraph->AddFilter( pSrc, name );

				pMoniker->Release();
                if ( hr == S_OK ) {
                    break; // 1st found dev. 
                }

            }
            TL_RELEASE(pSrc);
        } 
        pClassEnum->Release();
    }
    TL_RELEASE(pDevEnum);
    TRACE_HR(hr);
    return pSrc;
}

IBaseFilter *FGraph::getDevice( const GUID &cat, const char *name, int id ) 
{
    if ( ! pGraph ) 
        return 0;

    hr = 0;
    IBaseFilter *pSrc = NULL; // return value;

	ICreateDevEnum *pDevEnum = NULL;
    TL_COCREATE( CLSID_SystemDeviceEnum, IID_ICreateDevEnum, pDevEnum );
   
    IEnumMoniker *pClassEnum = NULL;
    hr = pDevEnum->CreateClassEnumerator( cat, &pClassEnum, 0 );
    if ( hr != S_OK ) 
	{
	    TRACE_HR(hr);
		return 0;
	}
	int current = 0;
    ULONG cFetched=0;
    IMoniker *pMoniker = NULL;
    while (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK)
	{
		char dev[400];
        hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
	    TRACE_HR(hr);
        if ( pSrc && hr==S_OK ) 
		{
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag));
			getStringProp(pPropBag, L"FriendlyName", dev);
			pPropBag->Release();
		}
		pMoniker->Release();
        if ( pSrc && hr==S_OK ) 
		{
			printf( "dev [0x%08x]:	%s_%d\n", pSrc, dev, current );

			if ( ( ! name )
			  || ( ! stricmp( dev, name ) ) )
			{
				if ( id != current++ )
					continue;

				WCHAR w[400];
				a2w(dev,w);
                hr = pGraph->AddFilter( pSrc, w );
			    TRACE_HR(hr);
				break; // return first found
			}
			else
			{
				current = 0;
			}
        }
        TL_RELEASE(pSrc);
    } 
    TL_RELEASE(pClassEnum);
    TL_RELEASE(pDevEnum);
    return pSrc;
}


bool FGraph::enumDevices( const char *guid, TL_ENUMCB cb ) 
{
    IBaseFilter *pSrc = NULL;
	ICreateDevEnum *pDevEnum = NULL;
	CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (LPVOID *)(&pDevEnum) );   
    IEnumMoniker *pClassEnum = NULL;
    hr = pDevEnum->CreateClassEnumerator( fromStr(guid), &pClassEnum, 0 );
    if ( hr != S_OK ) 
	{
	    pDevEnum->Release();
		return 0;
	}
    ULONG cFetched=0;
    IMoniker *pMoniker = NULL;
	bool r = true;
	while (r && (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK))
	{
		char dev[400], guid[400];
        hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
        if ( pSrc && hr==S_OK ) 
		{
			guid[0]=0;
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag));
			getStringProp(pPropBag, L"FriendlyName", dev);
			getStringProp(pPropBag, L"CLSID", guid);
			pPropBag->Release();
			r = cb( pSrc, dev, guid );
			pSrc->Release();
		}
		pMoniker->Release();
    } 
    pClassEnum->Release();
    pDevEnum->Release();
    return 1;
}



HRESULT FGraph::writeBitmap( TCHAR *bmpName, BYTE *pBuffer, int w, int h )
{
    HANDLE hf = CreateFile(
        bmpName, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, NULL, NULL );

    if( hf == INVALID_HANDLE_VALUE )
        return E_FAIL;

    BITMAPFILEHEADER bfh = {0};
    BITMAPINFOHEADER bih = {0};

    DWORD dwWritten = 0;
    DWORD s_bih = sizeof( BITMAPINFOHEADER );
    DWORD s_bfh = sizeof( BITMAPFILEHEADER );
	DWORD len = w*h*3;
    bfh.bfType     = 'MB';
    bfh.bfOffBits  = s_bih + s_bfh;
    bfh.bfSize     = bfh.bfOffBits + len;

    bih.biSize     = s_bih;
    bih.biWidth    = w;
    bih.biHeight   = h;
    bih.biPlanes   = 1;
    bih.biBitCount = 24;
    
    WriteFile( hf, &bfh, s_bfh, &dwWritten, NULL );
    WriteFile( hf, &bih, sizeof( bih ), &dwWritten, NULL );
    WriteFile( hf, pBuffer, len, &dwWritten, NULL );

    CloseHandle( hf );
    return 0;
}



////////////////////////////////////////////////////////////////////////////////////
// rot /////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

HRESULT FGraph::addToRot() 
{
#ifdef TL_DEBUG
    if ( rot )
        fprintf(_lf, "ROT! %08x\n",rot);
#endif
    rot = 0;

    IRunningObjectTable *pROT=0;
    if (FAILED(GetRunningObjectTable(0, &pROT))) 
        return E_FAIL;

    IMoniker * pMoniker=0;
    WCHAR wsz[128];

    DWORD pid = GetCurrentProcessId();
    wsprintfW(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)pGraph, pid );

    hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) {
        hr = pROT->Register(0, pGraph, pMoniker, &rot);
        pMoniker->Release();
    }
    pROT->Release();
#ifdef TL_DEBUG
    fprintf(_lf, "ROT+ %08x [%x]\n",rot,pid);
#endif
    TRACE_HR(hr);
    return hr;
}


HRESULT FGraph::removeFromRot()
{
    if ( !rot ) return E_FAIL; 

    IRunningObjectTable *pROT = 0;
    hr = GetRunningObjectTable(0, &pROT);
    if (SUCCEEDED(hr)) {
        pROT->Revoke(rot);
        pROT->Release();
    }
#ifdef TL_DEBUG
    fprintf(_lf, "ROT- %08x\n",rot);
#endif

    rot = 0;
    TRACE_HR(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////
// window //////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
void FGraph::resizeWin( RECT &grc ) 
{
    if ( ! pivw )
        return;
    pivw->SetWindowPosition(grc.left, grc.top, grc.right, grc.bottom);
}

void FGraph::resizeWin( HWND hwnd ) 
{
    RECT grc;        
    GetClientRect(hwnd, &grc);

    resizeWin(grc);
}

void FGraph::setupWin( HWND prnt ) 
{
    if ( ! pGraph )
        return;

    if ( ! pivw )
        TL_QUERY( pGraph, IID_IVideoWindow, pivw );
    
    pivw->put_Owner((OAHWND)prnt);
    pivw->put_WindowStyle(WS_VISIBLE | WS_CHILD);//|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
    pivw->put_Visible( OATRUE );
    resizeWin( prnt );
}


void FGraph::clearWin ()
{
    if ( pivw ) {
        pivw->put_Visible(OAFALSE);
        pivw->put_Owner(NULL);
    }
    TL_RELEASE(pivw);
}


////////////////////////////////////////////////////////////////////////////////////
// events //////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
void FGraph::setMessageWindow( HWND messageWin )
{
	HRESULT hr;
	if ( pivw ) 
		hr = pivw->put_MessageDrain(  (OAHWND) messageWin );
}


HWND FGraph::getMessageWindow() const
{
	HRESULT hr;
	HWND messageWin ;
	if ( pivw )
	{
		hr = pivw->get_MessageDrain( (OAHWND *) & messageWin );
		return messageWin;
	}

	return 0;
}


HRESULT FGraph::eventSetNotify( HWND hwnd )
{
    if (!pGraph) return E_POINTER;
    if (!pEvent)
        TL_QUERY(pGraph,IID_IMediaEventEx, pEvent );
    return pEvent->SetNotifyWindow((OAHWND)hwnd, WM_GRAPHNOTIFY, 0);
}

HRESULT FGraph::eventEnable( bool on )
{
    if (!pEvent) return E_POINTER;
    return pEvent->SetNotifyFlags(on?0:AM_MEDIAEVENT_NONOTIFY);
}


HRESULT FGraph::eventGet( TL_EVENTCB evcb, void *user )
{
    if (!pEvent) return E_POINTER;
    bool ok=1;
    long evCode, param1, param2;
    while (ok && SUCCEEDED(hr=pEvent->GetEvent(&evCode, &param1, &param2, 0))) {
        ok = evcb(&evCode, &param1, &param2, user);
        hr=pEvent->FreeEventParams(evCode, param1, param2);
        TRACE_HR(hr);
    }
    return hr=(ok?S_OK:E_FAIL);
}



////////////////////////////////////////////////////////////////////////////////////
// audio ///////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

//! call this on audio capture filture
HRESULT FGraph::audioMixer( IBaseFilter *pAudio, int iPin, int iProp, double &val )
{
    IPin *pin = 0;
    hr = getPin(pAudio,PINDIR_INPUT,iPin, &pin );
    if ( hr ) return hr;

	IAMAudioInputMixer *mixer = NULL;
    TL_QUERY(pin,IID_IAMAudioInputMixer,mixer);
    if ( mixer ) {
        int vi=0;
        switch ( iProp ) {
        case 0   :	hr = mixer->get_MixLevel(&val); break;					
        case 1   :	hr = mixer->put_MixLevel(val); break;					
        case 2   :	hr = mixer->get_Pan(&val); break;					
        case 3   :	hr = mixer->put_Pan(val); break;					
        case 4   :	hr = mixer->get_Treble(&val); break;					
        case 5   :	hr = mixer->put_Treble(val); break;					
        case 6   :	hr = mixer->get_Bass(&val); break;					
        case 7   :	hr = mixer->put_Bass(val); break;					
        case 8   :	hr = mixer->get_Loudness(&vi);val=vi; break;					
        case 9   :	hr = mixer->put_Loudness(int(val)); break;					
        case 10  :	hr = mixer->get_Mono(&vi);val=vi; break;					
        case 11  :	hr = mixer->put_Mono(int(val)); break;	
        default  :  hr = E_FAIL; break;
        }
	}
    TL_RELEASE(mixer);
	TL_RELEASE(pin);
    TRACE_HR(hr);
    return hr;
}


//////////////////////////////////////////////////////////////////
//
// helpers
//
//////////////////////////////////////////////////////////////////
// red rec led:
void FGraph::blink ( void *pData, int w, int red ) 
{
	if (  red < 5 ) return ;

    static int pt[40][2] = 
	{
                             {3,0}, {4,0},
                      {2,1}, {3,1}, {4,1}, {5,1},
               {1,2}, {2,2}, {3,2}, {4,2}, {5,2}, {6,2},
        {0,3}, {1,3}, {2,3}, {3,3}, {4,3}, {5,3}, {6,3}, {7,3},
        {0,4}, {1,4}, {2,4}, {3,4}, {4,4}, {5,4}, {6,4}, {7,4},
               {1,5}, {2,5}, {3,5}, {4,5}, {5,5}, {6,5},
                      {2,6}, {3,6}, {4,6}, {5,6},
                             {3,7}, {4,7},
    };
	
	struct _bgr {unsigned char b,g,r;} *prgb = (_bgr*) pData;

    for ( int n=0; n<40; n++ ) 
	{
        int k = pt[n][0] + pt[n][1] * w;
        prgb[k].r = red;
    }
}

void FGraph::printGUID( const GUID & ng2 )
{
   printf_s(  "{%x08-%x04-%x04-", ng2.Data1, ng2.Data2, ng2.Data3 );
   for (int i = 0 ; i < 8 ; i++) {
	  if (i == 2)
		 printf_s("-");
	  printf_s("%02x", ng2.Data4[i]);
   }
   printf_s("}\n");
}


IBaseFilter *  FGraph::addFilter(IBaseFilter * src, const char * guidstr, const char * name, bool doAlert)
{
	HRESULT hr = S_OK;
	if ( ! src ) return 0;

	IBaseFilter * filter = createFilter( (char*)guidstr, (char*)name );
	if ( filter )
	{
		// there might be audio, too, so start testing at last pin:
		hr = connect( src, filter, 1 );
		if ( hr == S_OK )
		{
			return filter;
		}
		hr = connect( src, filter, 0 );
		if ( hr == S_OK )
		{
			return filter;
		}
		// connection to src pin failed:
		char szn[200];
		filterName(src,szn);
		char szx[200];
		sprintf(szx,"could not connect %s (%s) to filter!", name, guidstr );
		if ( doAlert )
			MessageBoxA( 0, szx,szn,0);
		remove( filter );
		TL_RELEASE( filter );
	}
	return src;
}

// decompressors:
IBaseFilter *  FGraph::addAviDecomp(IBaseFilter * src)
{
	return addFilter( src, "{CF49D4E0-1115-11CE-B03A-0020AF0BA770}", "avi-decomp", false );
}
IBaseFilter *  FGraph::addWmvDecomp(IBaseFilter * src)
{
	return addFilter( src, "{63F8AA94-E2B9-11D0-ADF6-00C04FB66DAD}", "wmv-decomp" );
}
IBaseFilter *  FGraph::addMpgDecomp(IBaseFilter * src)
{
	IBaseFilter * tail = addFilter( src, "{336475D0-942A-11CE-A870-00AA002FEAB5}", "mpeg-splitter" );
	return addFilter( tail, "{FEB50740-7BEF-11CE-9BD9-0000E202599C}", "mpeg-decomp" );
}
IBaseFilter *  FGraph::addCConverter( IBaseFilter * src )
{
	return addFilter( src, "{1643E180-90F5-11CE-97D5-00AA0055595A}", "color-converter" );
}


// sinks:
IBaseFilter *  FGraph::addNullRenderer( IBaseFilter * src )
{
	return addFilter( src, "{C1F400A4-3F08-11D3-9F0B-006008039E37}", "NullRenderer" );
}


// sources:
IBaseFilter *  FGraph::addColorSource()
{
	IBaseFilter * src = createFilter( "{0cfdd070-581a-11d2-9ee6-006008039e37}", "ColorSource" );
	return src;
}

IBaseFilter *  FGraph::addCamera( int showPropSheet, const char * preferedDevice, int preferedId )
{
	IBaseFilter * cam = getCam(preferedDevice, preferedId);
	if ( cam && (showPropSheet==1) )
	{
		IPin * pin =0;
		// our pin might not be the first (audio,preview):
		if ( S_OK != getPin( cam,PINDIR_OUTPUT,0,&pin) )
			if ( S_OK != getPin( cam,PINDIR_OUTPUT,1,&pin) )
				pin = 0;
		if ( pin )
			displayProperties(pin,GetActiveWindow());

		TL_RELEASE(pin);
	}
	if ( cam && (showPropSheet==2) )
	{
		showProps(cam,GetActiveWindow());
	}

	// we need 24-bit uncompressed input , so insert decomp before grabber:
	return ( addAviDecomp( cam ) );
}

IBaseFilter *  FGraph::addFile( const char * srcName )
{
	HRESULT hr = S_OK;
	IBaseFilter * pSource = 0;
	WCHAR wsz[180];
	a2w( srcName, wsz );    
	hr = pGraph->AddSourceFilter( wsz, wsz, &pSource );
	if ( hr != S_OK ) return 0;

	if ( strstr( srcName, ".wmv" ) )
		return addWmvDecomp( pSource );
	if ( strstr( srcName, ".avi" ) )
		return addAviDecomp( pSource );
	if ( strstr( srcName, ".mpg" ) )
		return addMpgDecomp( pSource );
	if ( strstr( srcName, ".mpeg" ) )
		return addMpgDecomp( pSource );
	return pSource;
}

IBaseFilter * FGraph::addGrabber( Grabber * grab,  IBaseFilter * tail )
{
	if ( ! grab ) return 0;

	IBaseFilter * sample = createFilter( "{C1F400A0-3F08-11d3-9F0B-006008039E37}", "SampleGrabber" );

	if ( ! sample ) 
	{
		printf( __FUNCTION__ " : Could not create SampleGrabber !\n" );
		return 0;
	}

	HRESULT hr = 0;
    ISampleGrabber  *sampleGrabber = 0;
    TL_QUERY( sample, IID_ISampleGrabber, sampleGrabber );
	while ( sampleGrabber )
	{
		// request rgb: ( do this BEFORE CONNECTING ! )
		{
			AM_MEDIA_TYPE mt = {0};
			mt.majortype = MEDIATYPE_Video;
			mt.subtype   = MEDIASUBTYPE_RGB24;
			hr = sampleGrabber->SetMediaType( &mt );
			TRACE_HR(hr);
		}

		// connect:
		{
			hr = connect( tail, sample, 1 );
			if ( hr != S_OK )
			{
				hr = connect( tail, sample, 0 );
				if ( hr != S_OK )
				{
					TRACE_HR(hr);
					char szn[200];
					filterName(tail,szn);
					char szx[200];
					sprintf(szx,"could not connect to SampleGrabber!" );
					MessageBoxA( 0, szx,szn,0);
					remove( sample );
					TL_RELEASE( sample );
					break;
				}
			}
		}

		// check mediatype again:
		AM_MEDIA_TYPE mt;
		hr = sampleGrabber->GetConnectedMediaType( &mt );

		if ( hr != S_OK )
		{
			printf( __FUNCTION__ " : no mediatape !\n" ); 
			break; 
		}
		if ( mt.majortype != MEDIATYPE_Video ) 
		{
			printf( __FUNCTION__ " : majortape != video!\n" ); 
			break; 
		}
		if ( mt.subtype   != MEDIASUBTYPE_RGB24 ) 
		{
			printf( "subtype : " ); 
			FGraph::printGUID( mt.subtype );
			printf( __FUNCTION__ " : minortape != rgb24! \n" ); 
			break;
		}

		VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt.pbFormat;
		sampler.lWidth  = vih->bmiHeader.biWidth;
		sampler.lHeight = vih->bmiHeader.biHeight;
		assert( (sampler.lWidth<4012) && (sampler.lHeight<4012) );
		sampler.setGrabber( grab );
		hr = sampleGrabber->SetCallback( &sampler, 0 );
		hr = sampleGrabber->SetOneShot(FALSE);
		hr = sampleGrabber->SetBufferSamples(FALSE);

		break; // done ok.
	}
    TL_RELEASE(sampleGrabber);
//            TL_RELEASE(sample);
	return sample;
}

int FGraph::buildGraph( const char * srcName, Grabber * grab, const char * dstName )
{
	HRESULT hr = S_OK;
	IBaseFilter * tail = 0;

	char src[500];
	strcpy( src, srcName );

	int id = 0;
	int pdl = strlen(srcName);
	if ( srcName[pdl-2] == '_' )
	{
		id = atoi(&srcName[pdl-1]);
		src[ pdl-2 ] = 0;
	}

	if ( ! stricmp( srcName, "camera+" ) )
	{
		tail = addCamera(1);
	}
	else
	if ( ! stricmp( srcName, "camera#" ) )
	{
		tail = addCamera(2);
	}
	else
	if ( ! stricmp( srcName, "camera" ) )
	{
		tail = addCamera(0);
	}
	else
	if ( (tail = addCamera(0, src, id ) ) )
	{
		
	}
	else
	if ( ! stricmp( srcName, "Color" ) )
	{
		tail = addColorSource(); // black ;- as long as i don't know how to set the color..
	}
	else
	{
		tail = addFile(srcName);
	}

	if ( ! tail ) return 0;

	if ( grab )
	{
		tail = addGrabber( grab, tail );
	}

	if ( ! strcmp( dstName, "NullRenderer" ) )
	{
		tail = addNullRenderer( tail );
	}
	else // "Video Renderer"
	{
		hr = render( tail, 1 );
		if ( hr != S_OK )
		{
			hr = render( tail, 0 );
		}
	}

	TL_RELEASE( tail );
	return ( int(hr==S_OK) );
}


