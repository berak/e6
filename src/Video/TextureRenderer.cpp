#include "TextureRenderer.h"
#include <uuids.h>
//#define     CheckPointer(pmt,e) if (!(pmt)) return e;


struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_TextureRenderer;

//-----------------------------------------------------------------------------
// CTextureRenderer constructor
//-----------------------------------------------------------------------------
//LPCTSTR ctName = TEXT("Texture Renderer"); 
TCHAR * ctName = TEXT("Texture Renderer"); 
CTextureRenderer::CTextureRenderer( LPDIRECT3DTEXTURE9 tex, LPUNKNOWN pUnk, HRESULT *phr )
      : CBaseVideoRenderer(__uuidof(CLSID_TextureRenderer), ctName, pUnk, phr)
      , m_bUseDynamicTextures(FALSE)
      , m_pTexture(tex)
                                  //, m_TextureFormat(0)
{
    // Store and AddRef the texture for our use.
    ASSERT(phr);
    if (phr)
        *phr = S_OK;
}


//-----------------------------------------------------------------------------
// CTextureRenderer destructor
//-----------------------------------------------------------------------------
CTextureRenderer::~CTextureRenderer()
{
    if ( m_pTexture ) m_pTexture->Release();
}

HRESULT CTextureRenderer::setTexture(LPDIRECT3DTEXTURE9 tex)
{
    if ( tex )
    {
        D3DSURFACE_DESC desc;
        if ( tex->GetLevelDesc( 0, &desc ) == S_OK )
        {
            //if ( ( desc.Width != m_lVidWidth ) || ( desc.Height != m_lVidHeight ) )
            //{
            //    return E_FAIL ;
            //}
            m_lVidWidth = desc.Width;
			m_lVidHeight = desc.Height;
            m_TextureFormat = desc.Format;
        }
    }
    if ( m_pTexture ) 
        m_pTexture->Release();
    m_pTexture = tex;
    if ( m_pTexture ) 
        m_pTexture->AddRef();
    return S_OK;
}
//-----------------------------------------------------------------------------
// CheckMediaType: This method forces the graph to give us an R8G8B8 video
// type, making our copy to texture memory trivial.
//-----------------------------------------------------------------------------
HRESULT CTextureRenderer::CheckMediaType(const CMediaType *pmt)
{
    HRESULT   hr = E_FAIL;
    VIDEOINFO *pvi=0;
    
    CheckPointer(pmt,E_POINTER);

    // Reject the connection if this is not a video type
    if( *pmt->FormatType() != FORMAT_VideoInfo ) {
        return E_INVALIDARG;
    }
    
    // Only accept RGB24 video
    pvi = (VIDEOINFO *)pmt->Format();

    if(IsEqualGUID( *pmt->Type(),    MEDIATYPE_Video)  &&
       IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_RGB24))
    {
        hr = S_OK;
    }
    
    return hr;
}

//-----------------------------------------------------------------------------
// SetMediaType: Graph connection has been made. 

// ppp new policy:
// since we got no texture here, just store input video properties
//
//-----------------------------------------------------------------------------
HRESULT CTextureRenderer::SetMediaType(const CMediaType *pmt)
{
//    HRESULT hr;

    UINT uintWidth = 2;
    UINT uintHeight = 2;

    // Retrive the size of this media type
    VIDEOINFO *pviBmp;                      // Bitmap info header
    pviBmp = (VIDEOINFO *)pmt->Format();

    m_lVidWidth  = pviBmp->bmiHeader.biWidth;
    m_lVidHeight = abs(pviBmp->bmiHeader.biHeight);
    m_lVidPitch  = (m_lVidWidth * 3 + 3) & ~(3); // We are forcing RGB24

//    D3DCAPS9 caps;
//    // here let's check if we can use dynamic textures
//    ZeroMemory( &caps, sizeof(D3DCAPS9));
//    hr = g_pd3dDevice->GetDeviceCaps( &caps );
//    if( caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES )
//    {
//        m_bUseDynamicTextures = TRUE;
//    }
//
//    if( caps.TextureCaps & D3DPTEXTURECAPS_POW2 )
//    {
//        while( (LONG)uintWidth < m_lVidWidth )
//        {
//            uintWidth = uintWidth << 1;
//        }
//        while( (LONG)uintHeight < m_lVidHeight )
//        {
//            uintHeight = uintHeight << 1;
//        }
/////PPP???        UpgradeGeometry( m_lVidWidth, uintWidth, m_lVidHeight, uintHeight);
//    }
//    else
//    {
//        uintWidth = m_lVidWidth;
//        uintHeight = m_lVidHeight;
//    }
//
//    // Create the texture that maps to this media type
//    hr = E_UNEXPECTED;
//    if( m_bUseDynamicTextures )
//    {
//        hr = g_pd3dDevice->CreateTexture(uintWidth, uintHeight, 1, D3DUSAGE_DYNAMIC, 
//                                         D3DFMT_X8R8G8B8,D3DPOOL_DEFAULT,
//                                         &g_pTexture, NULL);
//  //      g_pachRenderMethod = g_achDynTextr;
//        if( FAILED(hr))
//        {
//            m_bUseDynamicTextures = FALSE;
//        }
//    }
//    if( FALSE == m_bUseDynamicTextures )
//    {
//        hr = g_pd3dDevice->CreateTexture(uintWidth, uintHeight, 1, 0, 
//                                         D3DFMT_X8R8G8B8,D3DPOOL_MANAGED,
//                                         &g_pTexture, NULL);
////        g_pachRenderMethod = g_achCopy;
//    }
//    if( FAILED(hr))
//    {
//        //MG::NG_DebugLog(TEXT("Could not create the D3DX texture!  hr=0x%x"), hr);
//        return hr;
//    }
//
//    // CreateTexture can silently change the parameters on us
//    D3DSURFACE_DESC ddsd;
//    ZeroMemory(&ddsd, sizeof(ddsd));
//
//    if ( FAILED( hr = g_pTexture->GetLevelDesc( 0, &ddsd ) ) ) {
//        //MG::NG_DebugLog(TEXT("Could not get level Description of D3DX texture! hr = 0x%x"), hr);
//        return hr;
//    }
//
//
//    CComPtr<IDirect3DSurface9> pSurf; 
//
//    if (SUCCEEDED(hr = g_pTexture->GetSurfaceLevel(0, &pSurf)))
//        pSurf->GetDesc(&ddsd);
//
//    // Save format info
//    g_TextureFormat = ddsd.Format;
//
//    if (g_TextureFormat != D3DFMT_X8R8G8B8 &&
//        g_TextureFormat != D3DFMT_A1R5G5B5) {
//       // MG::NG_DebugLog(TEXT("Texture is format we can't handle! Format = 0x%x"), g_TextureFormat);
//        return VFW_E_TYPE_NOT_ACCEPTED;
//    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// DoRenderSample: A sample has been delivered. Copy it to the texture.
//-----------------------------------------------------------------------------
HRESULT CTextureRenderer::DoRenderSample( IMediaSample * pSample )
{

    BYTE  *pBmpBuffer, *pTxtBuffer; // Bitmap buffer, texture buffer
    LONG  lTxtPitch;                // Pitch of bitmap, texture

    BYTE  * pbS = NULL;
    DWORD * pdwS = NULL;
    DWORD * pdwD = NULL;
    UINT row, col, dwordWidth;
    
    CheckPointer(pSample,E_POINTER);
    CheckPointer(m_pTexture,E_UNEXPECTED);

    // Get the video bitmap buffer
    pSample->GetPointer( &pBmpBuffer );

    // Lock the Texture
    D3DLOCKED_RECT d3dlr;
    DWORD lockFlag = 0;
    if ( m_bUseDynamicTextures )
    {
        lockFlag =  D3DLOCK_DISCARD;
    }
    if ( FAILED(m_pTexture->LockRect(0, &d3dlr, 0,lockFlag)))
        return E_FAIL;

    // Get the texture buffer & pitch
    pTxtBuffer = static_cast<byte *>(d3dlr.pBits);
    lTxtPitch = d3dlr.Pitch;
    
    
    // Copy the bits    

    if (m_TextureFormat == D3DFMT_X8R8G8B8) 
    {
        // Instead of copying data bytewise, we use DWORD alignment here.
        // We also unroll loop by copying 4 pixels at once.
        //
        // original BYTE array is [b0][g0][r0][b1][g1][r1][b2][g2][r2][b3][g3][r3]
        //
        // aligned DWORD array is     [b1 r0 g0 b0][g2 b2 r1 g1][r3 g3 b3 r2]
        //
        // We want to transform it to [ff r0 g0 b0][ff r1 g1 b1][ff r2 g2 b2][ff r3 b3 g3]
        // below, bitwise operations do exactly this.

        dwordWidth = m_lVidWidth / 4; // aligned width of the row, in DWORDS
                                      // (pixel by 3 bytes over sizeof(DWORD))

        for( row = 0; row< (UINT)m_lVidHeight; row++)
        {
            pdwS = ( DWORD*)pBmpBuffer;
            pdwD = ( DWORD*)pTxtBuffer;

            for( col = 0; col < dwordWidth; col ++ )
            {
                pdwD[0] =  pdwS[0] | 0xFF000000;
                pdwD[1] = ((pdwS[1]<<8)  | 0xFF000000) | (pdwS[0]>>24);
                pdwD[2] = ((pdwS[2]<<16) | 0xFF000000) | (pdwS[1]>>16);
                pdwD[3] = 0xFF000000 | (pdwS[2]>>8);
                pdwD +=4;
                pdwS +=3;
            }

            // we might have remaining (misaligned) bytes here
            pbS = (BYTE*) pdwS;
            for( col = 0; col < (UINT)m_lVidWidth % 4; col++)
            {
                *pdwD = 0xFF000000     |
                        (pbS[2] << 16) |
                        (pbS[1] <<  8) |
                        (pbS[0]);
                pdwD++;
                pbS += 3;           
            }

            pBmpBuffer  += m_lVidPitch;
            pTxtBuffer += lTxtPitch;
        }// for rows
    }

    if (m_TextureFormat == D3DFMT_A1R5G5B5) 
    {
        for(int y = 0; y < m_lVidHeight; y++ ) 
        {
            BYTE *pBmpBufferOld = pBmpBuffer;
            BYTE *pTxtBufferOld = pTxtBuffer;   

            for (int x = 0; x < m_lVidWidth; x++) 
            {
                *(WORD *)pTxtBuffer = (WORD)
                    (0x8000 +
                    ((pBmpBuffer[2] & 0xF8) << 7) +
                    ((pBmpBuffer[1] & 0xF8) << 2) +
                    (pBmpBuffer[0] >> 3));

                pTxtBuffer += 2;
                pBmpBuffer += 3;
            }

            pBmpBuffer = pBmpBufferOld + m_lVidPitch;
            pTxtBuffer = pTxtBufferOld + lTxtPitch;
        }
    }

    // Unlock the Texture
    if (FAILED(m_pTexture->UnlockRect(0)))
        return E_FAIL;
    
    return S_OK;
}

