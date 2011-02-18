sampler     samp0           : register(s0);

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float3 Tex: TEXCOORD0;
};


float4 PS(VS_OUTPUT inp) : COLOR 
{
    return texCUBE( samp0, inp.Tex );
}

