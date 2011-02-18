

uniform float4x4  mWorldViewProj  : register(c0);  // World * View * Projection transformation
uniform float4    vDiffuseColor   : register(c13);  // diffuse
uniform float4    vMove           : register(c33);  // move

void VS( 

    in float4 iPos: POSITION,
    in float2 iTex: TEXCOORD0,

    out float4 oPos: POSITION,
    out float2 oTex: TEXCOORD0
)
{
    //oPos = mul( iPos, mWorldViewProj );
    oPos = mul( mWorldViewProj, iPos );
    oTex = iTex;
}

