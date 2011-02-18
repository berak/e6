sampler     samp0           : register(s0);
sampler     samp1           : register(s1);


struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex0: TEXCOORD0;
    float2 Tex1: TEXCOORD1;
};


float4 PS(VS_OUTPUT inp) : COLOR 
{
    float4 c0 = tex2D( samp0, inp.Tex0 );
    float4 c1 = tex2D( samp1, inp.Tex1 );
    return c0*c1;
}

