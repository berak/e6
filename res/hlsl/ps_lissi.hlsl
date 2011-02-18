float4      c2              : register(c2);
float4      c3              : register(c3);
sampler     samp0           : register(s0);
sampler     samp1           : register(s1);


struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
    float3 Col: COLOR0;
};

//-----------------------------------------------------------------------------
// pixelshader
//-----------------------------------------------------------------------------

float4 PS(VS_OUTPUT inp) : COLOR 
{
		float ca = c2.x * cos( inp.Pos.x * c3.x );
		float cb = c2.y * cos( inp.Pos.y * c3.y );
		float cc = c2.z * cos( inp.Pos.x * c3.y );
		float cd = c3.w * cos( inp.Pos.x * c3.y );
		float4 c = (
                    ( cc * ca - cd * cb ),
                    ( cb * cc - ca * cc ),
                    ( cc * cb - ca * cd ),
                    1 
                   );
    return c;
    //return float4( 1,0,0, 1 ); // red
}

