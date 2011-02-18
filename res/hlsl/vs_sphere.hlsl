//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);			// World * View * Projection transformation
float4x4    mView           : register(c8);			// View transformation
float4		vViewPosition   : register(c11);			// View pos

struct VS_INPUT 
{
    float4 Pos: POSITION;
};

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float3 Tex: TEXCOORD0;
};



//-----------------------------------------------------------------------------
// vertexshader
//-----------------------------------------------------------------------------

VS_OUTPUT VS( VS_INPUT inp )
{
    VS_OUTPUT outp;
	float4 p = inp.Pos;
	p.xyz *= 2.0;
	p.xyz -= vViewPosition.xyz;
	outp.Tex = normalize( inp.Pos.xyz  );
    outp.Pos = mul( mWorldViewProj, p );
    return outp;
}

