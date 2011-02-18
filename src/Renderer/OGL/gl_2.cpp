#include <stdio.h>
#include <string.h>

#include "gl_tk.h"
#include "gl_2.h"
#include "glew.h"
#include "wglew.h"
#include "../../e6/e6_sys.h"
#include "../../e6/e6_math.h"


namespace RGL
{
	// Declarations We'll Use
#define WGL_SAMPLE_BUFFERS_ARB	0x2041
#define WGL_SAMPLES_ARB		0x2042



	bool isExtensionSupported(const char *extension)
	{
		const size_t extlen = strlen(extension);
		const char *supported = NULL;

		PROC wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");
		if (wglGetExtString)
			supported = ((char*(__stdcall*)(HDC))wglGetExtString)(wglGetCurrentDC());

		if (supported == NULL)
			supported = (char*)glGetString(GL_EXTENSIONS);

		if (supported == NULL)
			return false;

		for (const char* p = supported; ; p++)
		{
			// Advance p Up To The Next Possible Match
			p = strstr(p, extension);

			if (p == NULL)
				return false;						// No Match

			// Make Sure That Match Is At The Start Of The String Or That
			// The Previous Char Is A Space, Or Else We Could Accidentally
			// Match "wglFunkywglExtension" With "wglExtension"

			// Also, Make Sure That The Following Character Is Space Or NULL
			// Or Else "wglExtensionTwo" Might Match "wglExtension"
			if ((p==supported || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' '))
				return true;						// Match
		}
		return true;
	}


	void _printShaderInfoLog(GLuint obj)
	{
	    int infologLength = 0;
	    int charsWritten  = 0;
	    char *infoLog = 0;

		glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
		CHECK_ERR;

	    if (infologLength > 1)
	    {
	        infoLog = new char[ infologLength ];
	        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
			printf("%s\n",infoLog);
	        delete [] (infoLog);
			CHECK_ERR;
	    }
	}

	void _printProgramInfoLog(GLuint obj)
	{
	    int infologLength = 0;
	    int charsWritten  = 0;
	    char *infoLog;

		glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
		CHECK_ERR;

	    if (infologLength > 1)
	    {
	        infoLog = new char[ infologLength ];
	        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
			printf("%s\n",infoLog);
	        delete [] (infoLog);
			CHECK_ERR;
	    }
	}

	uint _isCompiled( uint handle ) // shader 
	{
		int compiled = 0;
		glGetShaderiv( handle, GL_COMPILE_STATUS, &compiled); 
		CHECK_ERR;
		return compiled;
	}
	uint _isLinked( uint handle ) // program
	{
		int linked = 0;
		glGetProgramiv( handle, GL_LINK_STATUS, &linked); 
		CHECK_ERR;
		return linked;
	}



	//const char * _loadFile( const char * fn )
	//{
	//	static char buf[0xfffff];
	//	buf[0] = 0;

	//	FILE * f = fopen( fn, "rb" );
	//	if ( ! f ) 
	//	{
	//		printf( "File '%s' not found!\n ( ..on path '%s' )\n", fn, e6::sys::getCurrentDir() );
	//		return 0;
	//	}

	//	size_t r = fread( buf, 1, 0xfffff, f );
	//	buf[r] = 0;
	//	buf[r+1] = 0;
	//	fclose(f);

	//	if ( r<1 ) buf[0] = 0;
	//	return buf;
	//}


	uint _createShader( uint shType, const char * shaderFile ) 
	{  
		const char * src = e6::sys::loadFile( shaderFile ); 
		if ( ! src || !src[0] )
			return 0 ;

		GLuint  shHandle = glCreateShader( shType ); 
		CHECK_ERR;
		if ( ! shHandle )
			return 0;

		if ( strstr( shaderFile, ".hlsl" ) )
		{
			bool is_ps = (shType == GL_FRAGMENT_SHADER);
			const char * gl_src = RGL::convertHLSL( src, (is_ps?"PS":"VS"), is_ps, 0 );
			if ( ! gl_src  )
			{
				glDeleteShader( shHandle );
				return 0 ;
			}
			glShaderSource( shHandle, 1, &gl_src, NULL ); 
			CHECK_ERR;
		}
		else
		{
			glShaderSource( shHandle, 1, &src, NULL ); 
		}

		glCompileShader( shHandle ); 
		CHECK_ERR;

		uint vvs = _isCompiled( shHandle );
		if ( ! vvs )
		{
			_printShaderInfoLog( shHandle );
			glDeleteShader( shHandle ); 
			return 0;
		}
		return shHandle;
	}


	uint createVertexShader( const char * shaderFile ) 
	{
		return _createShader( GL_VERTEX_SHADER, shaderFile );
	}
	uint createPixelShader( const char * shaderFile ) 
	{
		return _createShader( GL_FRAGMENT_SHADER, shaderFile );
	}


	uint createProgram( uint vs,  uint ps )
	{
		uint vvs = _isCompiled( vs );
		if ( ! vvs ) return 0;
		uint vps = _isCompiled( ps );
		if ( ! vps ) return 0;

		GLuint program = glCreateProgram(); 
		if ( ! program ) return 0;
		CHECK_ERR;

		glAttachShader( program, vs ); 
		CHECK_ERR;
		glAttachShader( program, ps ); 
		CHECK_ERR;
		glLinkProgram ( program ); 
		CHECK_ERR;
 
		uint vpp = _isLinked( program );
		if ( ! vpp )
		{
			_printProgramInfoLog( program );
			glDeleteProgram( program ); 
			return 0;
		}
		return program; 
	} 

	uint createProgram( const char * vs_name,  const char * ps_name )
	{
		uint vs = _createShader( GL_VERTEX_SHADER, vs_name );
		if ( ! vs ) return 0;

		uint ps = _createShader( GL_FRAGMENT_SHADER, ps_name );
		if ( ! ps ) return 0;

		return createProgram( vs, ps );
	} 

	uint bindProgram( uint program )
	{
		glUseProgram( program ); 
		CHECK_ERR;
		if ( glIsProgram( program ) )
		{
			return 1;
		}
		return 0;
	}

	uint removeShader( uint shader )
	{
		if ( ! glIsShader( shader ) )
		{
			return 0;
		}
		glDeleteShader( shader );
		CHECK_ERR;
		return 1;
	}

	uint removeProgram( uint program )
	{
		if ( ! glIsProgram( program ) )
		{
			return 0;
		}
		glDeleteProgram( program );
		CHECK_ERR;
		return 1;
	}

	// moved to hlsl2glsl
	//uint setConstants( uint program )
	//{
	//	for ( int i=0; i<nUniformInfos; i++ )
	//	{
	//		setConstant( program, uniform_table[i].name, uniform_table[i].val, 4 );
	//	}
	//}
	uint setConstant( uint program, const char * name, const float* v, uint n )
	{
		int handle = glGetUniformLocation( program, name );
		CHECK_ERR;
		if ( handle < 0 )
		{
			static char * lastName = 0;
			if ( (! lastName) || (strcmp(lastName,name)) )
			{
				printf( __FUNCTION__ " : no handle for [%2d] %s\n", program, name );
				lastName = (char*)name;
			}
			// return 0;
		}

		//printf( "%s(%d) : ", name, n );
		if ( n == 1 )
		{
			if ( v )
				glUniform1f( handle, *v );
			CHECK_ERR;
		}
		else 
		if ( n == 4 )
		{
			glUniform4fv( handle, 1, v );
			CHECK_ERR;
		}
		else 
		if ( n == 16 )
		{
			glUniformMatrix4fv( handle, 1, false, v );
			CHECK_ERR;
		}

		return 1;
	}


	uint createRenderTarget( uint w, uint h )
	{
		GLenum target = GL_TEXTURE_2D; //GL_TEXTURE_RECTANGLE_NV;
		GLuint tex = 0;
		glGenTextures(1, &tex);
		CHECK_ERR;
		glBindTexture(target, tex);
		CHECK_ERR;
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		CHECK_ERR;
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		CHECK_ERR;
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		CHECK_ERR;
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		CHECK_ERR;
		glTexImage2D(target, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		CHECK_ERR;
		return tex;
	}

	void flushRenderTarget( uint rtex, uint w, uint h )
	{
        //uint target = GL_TEXTURE_RECTANGLE_NV;
        uint target = GL_TEXTURE_2D;
		glEnable(target);
		CHECK_ERR;
		glBindTexture( target, rtex );
		CHECK_ERR;
		float vp[4];
		glGetFloatv( GL_VIEWPORT, vp );
		CHECK_ERR;
//		glCopyTexSubImage2D( GL_TEXTURE_RECTANGLE_NV, 0, vp[0], vp[1], vp[2], vp[3], w, h );
		glCopyTexSubImage2D( target, 0, 0,     0,     0,     0,     w, h );
		CHECK_ERR;
        glDisable(target);
		CHECK_ERR;
	}


} //namespace RGL
