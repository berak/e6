//-----------------------------------------------------------------------------
// vertexshader definitions ( global )
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);  // World * View * Projection transformation
float4      pModel          : register(c7);  //
float3      pCamera         : register(c11);
float4      diffuse		    : register(c13);  // 
float4      fTime 		    : register(c22);  // Time parameter. This keeps increasing
float4      lightDir        : register(c20);
//-----------------------------------------------------------------------------
// vertexshader definitions ( mesh )
//-----------------------------------------------------------------------------
float4      center          : register(c33);

struct VS_INPUT 
{
    float4 Pos: POSITION;
    float3 Nor: NORMAL;
    float2 Tex: TEXCOORD0;
};

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
//    float3 Nor: NORMAL;
    float2 Tex: TEXCOORD0;
    float3 Col: COLOR0;
};

//-----------------------------------------------------------------------------
// vertexshader
//-----------------------------------------------------------------------------


VS_OUTPUT VS( VS_INPUT inp )
{
    VS_OUTPUT outp;
    float4 p = inp.Pos;
    float4 d = p - pModel;
    p.xyz += inp.Nor * fTime.z * length(d);
    outp.Pos = mul( mWorldViewProj,p );
    outp.Tex = inp.Tex;
    outp.Col = diffuse;
    return outp;
}

