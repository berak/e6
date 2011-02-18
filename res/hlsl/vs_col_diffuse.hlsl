//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);  // World * View * Projection transformation
float4x4    mModel          : register(c4);  //
float4      diffuse		    : register(c13);  // 

struct VS_INPUT 
{
    float4 Pos: POSITION;
    float4 Col: DIFFUSE;
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

    outp.Pos = mul( mWorldViewProj, inp.Pos );
    outp.Tex = inp.Tex;
    outp.Col = diffuse * inp.Col;
    return outp;
}

