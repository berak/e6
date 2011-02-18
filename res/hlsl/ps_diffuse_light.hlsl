sampler     samp0           : register(s0);

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
    float4 Col: COLOR0;
};

//-----------------------------------------------------------------------------
// pixelshader
//-----------------------------------------------------------------------------

float4 PS(VS_OUTPUT inp) : COLOR 
{
    return inp.Col * tex2D( samp0, inp.Tex );
}

