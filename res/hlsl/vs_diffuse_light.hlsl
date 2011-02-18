//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);  // World * View * Projection transformation
float4x4    mModel          : register(c4);  //
float4      diffuse		    : register(c13);  // 
float4      fTime 		    : register(c22);  // Time parameter. This keeps increasing
float4      light0Col       : register(c16);
float4      light0Pos       : register(c17);

struct VS_INPUT 
{
    float4 Pos: POSITION;
    float3 Nor: TEXCOORD0;
    float2 Tex: TEXCOORD0;
};

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
    float4 Col: COLOR0;
};

//-----------------------------------------------------------------------------
// vertexshader
//-----------------------------------------------------------------------------


VS_OUTPUT VS( VS_INPUT inp )
{
    VS_OUTPUT outp;

    outp.Pos = mul( mWorldViewProj, inp.Pos);
    outp.Tex = inp.Tex;
    float3 lightdir = light0Pos - inp.Pos;
    outp.Col = diffuse + light0Col * dot(inp.Nor,lightdir);
    return outp;
}


