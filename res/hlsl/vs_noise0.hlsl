//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);			// World * View * Projection transformation
float4x4    mView           : register(c8);			// View transformation
float4		vViewPosition   : register(c11);		// View pos
float4		cDiffuse        : register(c13);		// color
float4      fTime 		    : register(c22);		// t, sin(t), cos(t), dt
float4      fScale 		    : register(c25);		// 


struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float3 Tex: TEXCOORD0;
    float4 Col: TEXCOORD1;
};


VS_OUTPUT VS( float4 Pos: POSITION )
{
    VS_OUTPUT Out;

    Out.Pos = mul( mWorldViewProj, Pos);
    Out.Tex = (fTime.x  +  Pos.xyz) * fScale;
    Out.Col = cDiffuse;
   
    return Out;
}

