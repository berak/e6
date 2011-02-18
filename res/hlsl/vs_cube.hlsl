//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);			// World * View * Projection transformation
float4x4    mView           : register(c4);			// View transformation
float4      offset 		    : register(c32);		// mesh offset ( radius )

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
	outp.Tex = normalize( inp.Pos.xyz - offset.xyz ); // offset mesh radius
    outp.Pos = mul( mWorldViewProj, inp.Pos );
    return outp;
}

