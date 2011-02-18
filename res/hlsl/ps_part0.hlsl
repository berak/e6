float     particleExp       : register(c0); 
sampler     samp0           : register(s0);

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
    float4 Col: TEXCOORD1;
    float  amp: TEXCOORD2;
};

float4 PS(VS_OUTPUT In) : COLOR 
{
    //float particleExp=0.15;
    return (In.Col - pow(dot(In.Tex, In.Tex), particleExp)) * tex2D(samp0, float2( In.amp, 0.5f ));
   // return tex2D(samp0, float2( In.Col, 0.5f ));
}

