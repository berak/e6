//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);  // World * View * Projection transformation
float4x4    mModel          : register(c4);  // W
//float4      diffuse		    : register(c13);  // 
float4      light0   		    : register(c17);  // pos!
//float4      fTime 		    : register(c22);  // Time parameter. This keeps increasing

struct VS_INPUT 
{
    float4 Pos: POSITION;
    float3 Nor: NORMAL;
    float2 Tex: TEXCOORD0;
};

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float3 Nor: NORMAL;
    float2 Tex: TEXCOORD0;
	float3 light0Dir   : TEXCOORD1;
};

//-----------------------------------------------------------------------------
// vertexshader
//-----------------------------------------------------------------------------


VS_OUTPUT VS( VS_INPUT inp )
{
    VS_OUTPUT outp;

    outp.Pos = mul( mWorldViewProj, inp.Pos );
    outp.Tex = inp.Tex;
    outp.Nor = inp.Nor;
    outp.light0Dir = normalize( mul( (float3x3)(mModel), light0 ) );
    return outp;
}

