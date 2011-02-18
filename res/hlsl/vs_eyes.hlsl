float4x4    mWorldViewProj  : register(c0);  // World * View * Projection transformation
float4      quad            : register(c25);  // peak(x,y,v)

struct VS_INPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
};

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
//    float3 Col: COLOR0;
};




VS_OUTPUT VS( VS_INPUT inp )
{
    VS_OUTPUT outp;

    outp.Pos = mul( mWorldViewProj, inp.Pos );
    outp.Tex = inp.Tex + (quad.xy);
  //  outp.Tex = inp.Tex * (1.1 - quad.z) + (quad.xy);
  //  outp.Col = diffuse;
    return outp;
}

