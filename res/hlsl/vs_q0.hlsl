//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);			// World * View * Projection transformation
float4x4    mModel          : register(c4);			//
float4x4    mView           : register(c8);			//
float4      diffuse		    : register(c13);		// 
float4      light0Pos       : register(c20);		//
float4      fTime 		    : register(c22);		// Time parameter. This keeps increasing

float4      offset 		    : register(c32);		// Time parameter. This keeps increasing

struct VS_INPUT 
{
    float4 Pos: POSITION;
    float3 Nor: NORMAL;
    float2 Tex: TEXCOORD0;
};

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float3 Tex: TEXCOORD0;
//    float3 Col: COLOR0;
};

//-----------------------------------------------------------------------------
// vertexshader
//-----------------------------------------------------------------------------



VS_OUTPUT VS( VS_INPUT inp )
{
    VS_OUTPUT outp;
	float3 viewPos = float3(0,0,offset.z); // offset mesh radius
	float3 viewDir = normalize( inp.Pos - viewPos );
//	float3 viewNorm = inp.Nor;
	float3 viewNorm = mul( (float3x3)mView,  inp.Nor );
	float3 refl = reflect( viewNorm, viewDir );
	outp.Tex = refl.xzy;
//	outp.Tex = normalize( reflect( viewNorm, viewDir ) );
//	  outp.Tex = float3( inp.Tex, 1.0 );
//    outp.Col = diffuse;
	float4 p = inp.Pos.xzyw;
	p.y = p.y;
    outp.Pos = mul( mWorldViewProj, p );
    return outp;
}

