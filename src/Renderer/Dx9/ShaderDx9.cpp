#include "Dx9.h"

namespace Dx9
{
	
	static const char * _defVertexShader = 
		  "vs.1.1\n"
		  "dcl_position0 v0\n"
		  "m4x4 oPos, v0, c0\n"
		  "mov oD0, c13\n\n";

	static const char * _defPixelShader = 
		  "ps.1.0\n"
		  "mov r0, v0\n\n";

    IDirect3DVertexShader9 * VertexShaderCache::createVertexShader( const void * pData, uint nSize, bool fromFile, bool isCompiled )
    {
        IDirect3DVertexShader9 * vShader = 0;
        LPD3DXBUFFER pCode = 0;
        LPD3DXBUFFER pDebug = 0;
        DWORD * pVS = NULL;
        HANDLE hFile, hMap;
		HRESULT hr;

#ifdef DX9_PLAIN
        LPCSTR pProfile = "vs_1_1";
#else
        LPCSTR pProfile = D3DXGetVertexShaderProfile( device );
#endif

		// (1): ALREADY ASSEMBLED
        if (isCompiled) 
        {
            // already compiled (from file)
            if ( fromFile ) 
            {
                hFile = CreateFile((LPCTSTR)pData, GENERIC_READ,
                                0, 0, OPEN_EXISTING, 
                                FILE_ATTRIBUTE_NORMAL, 0);
                if ( hFile == INVALID_HANDLE_VALUE )
                    return 0;

                hMap = CreateFileMapping(hFile,0,PAGE_READONLY, 0,0,0);
                pVS = (DWORD*)MapViewOfFile(hMap,FILE_MAP_READ,0,0,0);
            }
            // already compiled (from datapointer)
            else 
            {
                pVS = (DWORD*)pData; 
            }
        } // if

        // (2): STILL NEEDS ASSEMBLING
        else
        {
            // not yet compiled (from file)
            if (fromFile)
            {
                if ( strstr( (char*)pData, ".fx") ||
					 strstr( (char*)pData, ".hlsl") )
                {
                    hr = D3DXCompileShaderFromFile(     
                        (char *) pData,
                        0, //CONST D3DXMACRO* pDefines,
                        0, //LPD3DXINCLUDE pInclude,
                        "VS", //LPCSTR pFunctionName,
                        pProfile,
#ifdef _DEBUG
                        D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION, //DWORD Flags,
#else
                        0, //DWORD Flags,
#endif
                        &pCode, //LPD3DXBUFFER* ppShader,
                        &pDebug, //LPD3DXBUFFER *ppErrorMsgs,
                        0 //LPD3DXCONSTANTTABLE *ppConstantTable
                    );            
                    E_TRACE(hr);
                     if ( ! hr ) 
                     { 
#ifndef DX9_PLAIN
                         LPD3DXBUFFER pDisassembly=0;
                         hr = D3DXDisassembleShader((DWORD*) pCode->GetBufferPointer(), false, 0, &pDisassembly );
                         if ( hr == S_OK )
                         {
							 char * bufPtr = (char *)pDisassembly->GetBufferPointer();

							 char fn[200];
							 sprintf( fn, "%s.log", (char*)pData );
                             FILE * F = fopen( fn, "wb+" );
                             if ( F )
                             {
                                 fwrite( bufPtr, pDisassembly->GetBufferSize(), 1, F );
                                 fclose( F );
                             }
							#if DEBUG
                             printf( bufPtr );
							#endif
                             pDisassembly->Release();
                         }
                     }
#endif // DX9_PLAIN
                }
                else
                {
                    hr = D3DXAssembleShaderFromFile( (char *) pData, NULL, NULL, 
                                //0,
                                D3DXSHADER_DEBUG ,
                                & pCode, & pDebug );
                    E_TRACE(hr);
                }
            }
            // not yet compiled (from datapointer)
            else
            {
				// check for asm:
				if ( strstr( (char*)pData, "vs.1." ) || strstr( (char*)pData, "vs_1_" ) )
				{
					hr = D3DXAssembleShader(
								(char*)pData, 
								nSize,
								NULL, 
								NULL, 
#ifdef _DEBUG
								D3DXSHADER_DEBUG, //DWORD Flags,
#else
								0, //DWORD Flags,
#endif
								&pCode, &pDebug
							);
		            E_TRACE(hr);
				}
				else // hlsl from memory:
				{
                    hr = D3DXCompileShader(     
                        (char *) pData,
                        strlen((char*)pData),
						0, //CONST D3DXMACRO* pDefines,
                        0, //LPD3DXINCLUDE pInclude,
                        "VS", //LPCSTR pFunctionName,
                        pProfile,
#ifdef _DEBUG
                        D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION, //DWORD Flags,
#else
                        0, //DWORD Flags,
#endif
                        &pCode, //LPD3DXBUFFER* ppShader,
                        &pDebug, //LPD3DXBUFFER *ppErrorMsgs,
                        0 //LPD3DXCONSTANTTABLE *ppConstantTable
                    );            
                    E_TRACE(hr);
				}
            }
            // track errors if any
            if ((hr)!=S_OK)
            {
				char sb[100];
				GetCurrentDirectory(100,sb);
				printf( "cwd: %s \n", sb );
				if ( pDebug )
				{
					char *s = (char*)pDebug->GetBufferPointer();
					printf( "\n%s error : %s \n", __FUNCTION__, s );
				}


                return 0;
            }
            pVS = (DWORD*) pCode->GetBufferPointer();
        } // else

        // create the shader object
        hr = device->CreateVertexShader( pVS, & vShader );
        E_TRACE(hr);
        if ( FAILED( hr ) ) 
		{
            vShader = 0;
        }

        // free resources
        if (isCompiled && fromFile) 
        {
            UnmapViewOfFile(pVS);
            CloseHandle(hMap);
            CloseHandle(hFile);
        }
        else
        {
            if ( pCode ) pCode->Release();
            if ( pDebug ) pDebug->Release();
        }
        return vShader;
    } 

	void VertexShaderCache::setDevice( LPDIRECT3DDEVICE9 dev )
	{
		device = (dev);
		IDirect3DVertexShader9 * sh = createVertexShader( _defVertexShader, strlen(_defVertexShader), 0, 0 );
		insert( 0, sh, 0xffffffff );
	}

    IDirect3DVertexShader9 * VertexShaderCache::addShader( Core::Shader * shader )
	{
		if ( ! shader ) return 0;
		printf( "%s %s\n", __FUNCTION__, shader->getName());
		IDirect3DVertexShader9 * sh = 0;
		const char * code = shader->getCode();
		if ( code )
			sh = createVertexShader( code, strlen(code), 0, 0 );
		else
			sh = createVertexShader( shader->getPath(), 0, 1, 0 );
		if ( sh ) 
			insert( shader, sh );
		return sh;
	}

    IDirect3DVertexShader9 * VertexShaderCache::getBuffer( Core::Shader * s )
	{
		//printf( "%s %s\n", __FUNCTION__, (s?s->getName():"nil"));
		Item * it = find( s );
		if ( ! it || ! it->val )
		{
			return addShader(s);
		}
		it->ttl = 500;
		return it->val;
	}



	void PixelShaderCache::setDevice( LPDIRECT3DDEVICE9 dev )
	{
		device = (dev);
		IDirect3DPixelShader9 * sh = createPixelShader( _defPixelShader, strlen(_defPixelShader), 0, 0 );
		insert( 0, sh, 0xffffffff );
	}


    IDirect3DPixelShader9 * PixelShaderCache::addShader( Core::Shader * shader )
	{
		if ( ! shader ) return 0;
		printf( "%s %s\n", __FUNCTION__, shader->getName());
		IDirect3DPixelShader9 * sh = 0;
		const char * code = shader->getCode();
		if ( code )
			sh = createPixelShader( code, strlen(code), 0, 0 );
		else
			sh = createPixelShader( shader->getPath(), 0, 1, 0 );
		if ( sh )
			insert( shader, sh );
		return sh;
	}

    IDirect3DPixelShader9 * PixelShaderCache::getBuffer( Core::Shader * s )
	{
		//printf( "%s %s\n", __FUNCTION__, (s?s->getName():"nil"));
		Item * it = find( s );
		if ( ! it || ! it->val )
		{
			return addShader(s);
		}
		it->ttl = 500;
		return it->val;
	}


	
	IDirect3DPixelShader9 * PixelShaderCache::createPixelShader( const void * pData, uint nSize, bool fromFile, bool isCompiled )
    {
 		IDirect3DPixelShader9 * pShader = NULL;
        ID3DXBuffer * pCode = 0;
        ID3DXBuffer * pDebug = 0;
        DWORD        *pPS=NULL;
        HANDLE        hFile, hMap;
		HRESULT hr;

#ifdef DX9_PLAIN
        LPCSTR pProfile = "ps_1_1";
#else
        LPCSTR pProfile = D3DXGetPixelShaderProfile( device );
#endif
        // (1): ALREADY ASSEMBLED
        if ( isCompiled ) 
        {
            // already compiled (from file)
            if ( fromFile ) 
            {
                hFile = CreateFile((LPCTSTR)pData, GENERIC_READ,
                                0, 0, OPEN_EXISTING, 
                                FILE_ATTRIBUTE_NORMAL, 0);

                if ( hFile == INVALID_HANDLE_VALUE )
				{
                    return 0;
				}

                hMap = CreateFileMapping( hFile, 0, PAGE_READONLY, 0, 0, 0 );
                pPS = (DWORD *) MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );
            }
            // already compiled (from datapointer)
            else
			{
                pPS = (DWORD *) pData;
			}
        } // if

        // (2): STILL NEEDS ASSEMBLING
        else 
        {
            // not yet compiled (from file)
            if ( fromFile ) 
            {
                if ( strstr( (char*)pData, ".fx") ||
					 strstr( (char*)pData, ".hlsl") )
                {
					hr = D3DXCompileShaderFromFile(     
						(char *) pData,
						0, //CONST D3DXMACRO* pDefines,
						0, //LPD3DXINCLUDE pInclude,
						"PS", //LPCSTR pFunctionName,
						pProfile,
#ifdef _DEBUG
						D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION, //DWORD Flags,
#else
						0, //DWORD Flags,
#endif
						& pCode, //LPD3DXBUFFER* ppShader,
						& pDebug, //LPD3DXBUFFER *ppErrorMsgs,
						0 //LPD3DXCONSTANTTABLE *ppConstantTable
						);
						E_TRACE(hr);
#ifndef DX9_PLAIN
					if ( hr == S_OK )
                    { 
						LPD3DXBUFFER pDisassembly=0;
						hr = D3DXDisassembleShader((DWORD*) pCode->GetBufferPointer(), false, 0, &pDisassembly );
						if ( hr == S_OK )
						{
							char fn[200];
							sprintf( fn, "%s.log", (char*)pData );
							FILE * F = fopen( fn, "wb+" );
							if ( F )
							{
								fwrite( pDisassembly->GetBufferPointer(), pDisassembly->GetBufferSize(), 1, F );
								fclose( F );
							}
							#if DEBUG
							printf( (char*)(pDisassembly->GetBufferPointer()) );
							#endif
							pDisassembly->Release();
						}
					}
#endif //DX9_PLAIN
                }
                else // extension !=  "*.fx"
                {
                   hr = D3DXAssembleShaderFromFile( (char *) pData, NULL, NULL, 0,
                                                & pCode, & pDebug );
                    E_TRACE(hr);
                }
            }
            // not yet compiled (from datapointer)
            else 
            {
				// check for asm:
				if ( strstr( (char*)pData, "ps.1" ) || strstr( (char*)pData, "ps_1" ) )
				{
					hr = D3DXAssembleShader((char*)pData, nSize,
											NULL, NULL, 0,
											&pCode, &pDebug );
					E_TRACE(hr);
				}
				else // ps hlsl from memory
				{
                    hr = D3DXCompileShader(     
                        (char *) pData,
                        strlen((char *)pData),
						0, //CONST D3DXMACRO* pDefines,
                        0, //LPD3DXINCLUDE pInclude,
                        "VS", //LPCSTR pFunctionName,
                        pProfile,
#ifdef _DEBUG
                        D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION, //DWORD Flags,
#else
                        0, //DWORD Flags,
#endif
                        &pCode, //LPD3DXBUFFER* ppShader,
                        &pDebug, //LPD3DXBUFFER *ppErrorMsgs,
                        0 //LPD3DXCONSTANTTABLE *ppConstantTable
                    );            
                    E_TRACE(hr);
				}
            }

            // track errors if any
            if ( SUCCEEDED( hr ) && pCode )
			{
                pPS = (DWORD *) pCode->GetBufferPointer();
			}
            else
			{
				if ( pDebug )
				{
					printf( "\t %s\n", (char *) pDebug->GetBufferPointer() );
				}
                return 0;
			}
        } // else

        // create the shader object
        hr = device->CreatePixelShader( pPS, & pShader );
        E_TRACE(hr);
        if ( FAILED( hr ) )
		{
            pShader = 0;
        }

        // free resources
        if ( isCompiled && fromFile ) 
        {
            UnmapViewOfFile( pPS );
            CloseHandle( hMap );
            CloseHandle( hFile );
        }

        return pShader;
    }
	




}; // dx9

