//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);			// World * View * Projection transformation
float4      fTime 		    : register(c22);		// t, sin(t), cos(t), dt

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
};


VS_OUTPUT VS( float4 Pos: POSITION, float2 Tex : TEXCOORD0 )
{
    VS_OUTPUT Out;
    int x = floor(2 + 0.9*cos( 1.12 * fTime.x ) * 2);
    int y = floor(2 + 0.9*cos( 1.73 * fTime.x ) * 2);

    Out.Tex = ( Tex + float2( x, y ) ) * 0.25;
    Out.Tex.y *= -1.0;
    Out.Pos = mul(mWorldViewProj,Pos);

    return Out;
}

