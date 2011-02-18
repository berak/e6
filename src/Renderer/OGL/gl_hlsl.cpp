#include <stdio.h>
#include <string.h>

#include "gl_2.h"

#include "HLSL2GLSL.h"


namespace RGL
{

	//struct UniformInfo 
	//{
	//	char  name[64];
	//	float val[16];
	//	int   vtype;
	//	int   reg;
	//} uniform_table[128];
    int nUniformInfos = 0;
	UniformInfo uniform_table[128];

	//
	// Uniform element count lookup table by uniform type
	//
	const int uniform_elementCount[] = 
	{
	  0,
	  1,
	  2,
	  3,
	  4,
	  1,
	  2,
	  3,
	  4,
	  1,
	  2,
	  3,
	  4,
	  4,
	  9,
	  16,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0
	};
	//
	// Save the shader to the file
	//
	bool _saveShader( const char* fn, ShHandle handle, EShLanguage lang)
	{
	   FILE* fp = fopen( fn, "wb");

	   //__asm int 3;

	   if (!fp)
		  return false; // failed to open file

	   const char* text = Hlsl2Glsl_GetShader( handle, lang );

	   if (text)
		  fprintf( fp, "%s", text);

	   fclose(fp);
	   return !text;
	}


	//
	// Save the uniform info to a file
	//
	bool _saveUniforms( const char* fn, ShHandle handle)
	{
	   //
	   // Uniform type conversion string
	   //
	   const char typeStrings[][32] = 
	   {
		  "void",
		  "bool",
		  "bvec2",
		  "bvec3",
		  "bvec4",
		  "int",
		  "ivec2",
		  "ivec3",
		  "ivec4",
		  "float",
		  "vec2",
		  "vec3",
		  "vec4",
		  "mat2",
		  "mat3",
		  "mat4",
		  "sampler",
		  "sampler1D",
		  "sampler2D",
		  "sampler3D",
		  "samplerCube",
		  "struct"
	   };


	   FILE* fp = fopen( fn, "wb");

	   if (!fp)
		  return false; // failed to open file

	   const ShUniformInfo* uniforms = Hlsl2Glsl_GetUniformInfo( handle);
	   const int nUniforms = Hlsl2Glsl_GetUniformCount( handle);

	   for (int ii=0; ii<nUniforms; ii++)
	   {
		  fprintf( fp, "%s %s", typeStrings[uniforms[ii].type], uniforms[ii].name);
		  if ( uniforms[ii].arraySize)
			 fprintf( fp, "[%d]", uniforms[ii].arraySize);
		  if (uniforms[ii].semantic)
			 fprintf( fp, " : %s", uniforms[ii].semantic);

		  if (uniforms[ii].init)
		  {
			 int as = uniforms[ii].arraySize;
			 as = (as) ? as : 1;
			 fprintf( fp, " =\n  {");
			 for ( int jj=0; jj < uniform_elementCount[uniforms[ii].type]*as; jj++)
			 {
				fprintf( fp, " %f,", uniforms[ii].init[jj]);
			 }
			 fprintf( fp, "}\n");
		  }
		  else
			 fprintf( fp, "\n");
	   }

	   fclose(fp);
	   return true;
	}


	const static bool _doSave = 1;
	int uniformInfo(RGL::UniformInfo * ui, const char * name)
	{
		for (int ii=0; ii<nUniformInfos; ii++)
		{
			if ( strcmp(name,uniform_table[ii].name) ) continue;
			if ( ui )
				*ui = uniform_table[ii];
			return ii;
		}
		return -1;
	}

	RGL::UniformInfo * uniformInfo (int id)
	{
		for (int ii=0; ii<nUniformInfos; ii++)
		{
			if ( id == uniform_table[ii].reg )
				return &uniform_table[ii];
		}
		return 0;
	}
	int findReg(const char * txt, const char * name)
	{
		char *off=(char*)txt;
		while ( (off<txt+strlen(txt)) && (off=strstr( off,"register" )) )
		{
			char c;
			int r;

			if ( strstr( off-strlen(name)-8,name ) )
			{
				sscanf( off, "register(%c%d)",&c,&r);
				printf("%s : register(%c%d)",name,c,r);
				return r;
			}
			off += 8;
		}
		printf("%s : register NOT FOUND",name);
		return -1;
	}

	const char * convertHLSL( const char * shaderString, const char * entry, bool ps, int debugOptions )
	{
		// Initialize the HLSL to GLSL translator
		Hlsl2Glsl_Initialize();

		EShLanguage lang = (ps ? EShLangFragment:EShLangVertex);
		// Construct a vertex shader parser for the translator to use
		ShHandle parser = Hlsl2Glsl_ConstructParser( lang, debugOptions);

		// Construct a Translator to use for final code translation
		ShHandle translator = Hlsl2Glsl_ConstructTranslator(debugOptions);

		char head[500];
		sprintf( head, "//\r\n//  " __FUNCTION__ "(%s)\r\n//\r\n\r\n", "" );
		int re =  Hlsl2Glsl_SetShaderHeader ( translator, 
                                true, //bool bOutputShaderHeader, 
                                head ); //const char *shaderHeaderString );


		// Parse the shader – print out the info log on failure
		const char *shaderStrings[1] = { shaderString };
		if ( !Hlsl2Glsl_Parse(parser, shaderStrings, 1, debugOptions) )
		{
			printf( "%s(%i) : %s\n", __FUNCTION__,__LINE__,Hlsl2Glsl_GetInfoLog(parser) );
		   return 0;
		}


		// Now translate the parsed shader
		ShHandle parsers[1] = { parser };
		if (! Hlsl2Glsl_Translate(translator, parsers, 1, (!ps?entry:NULL), (ps?entry:NULL)) )
		{                         
			printf( "%s(%i) : %s\n", __FUNCTION__,__LINE__,Hlsl2Glsl_GetInfoLog(parser) );
		   return 0;
		}

	    
		// Finally, get the translated shader source (GLSL)
		const char* text = Hlsl2Glsl_GetShader( translator, lang );


		static char res[0xffff];
		res[0]=0;
		strcpy( res, text );
		printf( text );

		const ShUniformInfo* unis = Hlsl2Glsl_GetUniformInfo( translator );
		int nUniforms = Hlsl2Glsl_GetUniformCount( translator );

		for (int ii=0; ii<nUniforms; ii++)
		{
			if ( uniformInfo( 0, unis[ii].name ) < 0 )
				continue;

			uniform_table[nUniformInfos].vtype = unis[ii].type;
			strcpy(uniform_table[nUniformInfos].name, unis[ii].name);
			uniform_table[nUniformInfos].reg = findReg( text, unis[ii].name );
			if (unis[ii].init)
			{
				int as = unis[ii].arraySize;
				if ( as<1 ) as = 1;
				int asiz = uniform_elementCount[unis[ii].type]*as;
				//uniform_table[ii].vtype = unis[ii].;
				for ( int jj=0; jj < asiz; jj++)
				{
					uniform_table[nUniformInfos].val[jj] = unis[ii].init[jj];
				}
			}
			nUniformInfos ++;
		}

		if ( _doSave )
		{
			static int id = 0;
			char fn[300];
			const char * dir = "c:\\temp";
			sprintf( fn, "%s/%s_%i.glsl", dir,entry, id );
			_saveShader( fn, translator, lang );
			sprintf( fn, "%s/%s_%i.uni", dir,entry, id );
			_saveUniforms( fn, translator );
			++ id;
		}
		// ...shutdown 
		Hlsl2Glsl_Destruct( parser );
		Hlsl2Glsl_Destruct( translator );
		Hlsl2Glsl_Finalize( ); // this will kill 'text' !
		return res;
	}


	uint setConstants( uint program )
	{
		for ( int i=0; i<nUniformInfos; i++ )
		{
			setConstant( program, uniform_table[i].name, uniform_table[i].val, 4 );
		}
		return 1;
	}
} //namespace RGL
