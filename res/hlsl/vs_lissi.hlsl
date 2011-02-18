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
float4      c2              : register(c32);
float4      c3              : register(c33);

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
    outp.Pos = mul( mWorldViewProj,  inp.Pos );
    outp.Tex = inp.Tex;
    outp.Col = c;
    return outp;
}

