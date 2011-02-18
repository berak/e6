

uniform float4x4  mWorldViewProj  : register(c0);  // World * View * Projection transformation

void VS( 

    in float4 iPos:     POSITION,
    in float2 iTex0:    TEXCOORD0,
    in float2 iTex1:    TEXCOORD1,

    out float4 oPos:    POSITION,
    out float2 oTex0:   TEXCOORD0,
    out float2 oTex1:   TEXCOORD1
)
{
    //oPos = mul( iPos, mWorldViewProj );
    oPos = mul( mWorldViewProj, iPos );
    oTex0 = iTex0;
    oTex1 = iTex1;
}

