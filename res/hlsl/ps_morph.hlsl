sampler     samp0           : register(s0);


struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
};


float4 PS(VS_OUTPUT inp) : COLOR 
{
    float4 col = tex2D( samp0, inp.Tex );
    if ( col.r < 0.14 )
    {
        col.r = 1.0;
    }

    if ( (col.g>0.95) && (col.b>0.95) )
    {
        col.r = 1.0;
        col.g = 0.0;
        col.b = 0.0;
    }
    return col;
}

