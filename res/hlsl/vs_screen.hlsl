//
// just pipe it all through..
//

void VS
(
    in float4 Pos: POSITION,
    in float2 Tex: TEXCOORD0,

    out float4 pos: POSITION,
    out float3 vpos: TEXCOORD0,
    out float2 tex0: TEXCOORD1
)
{
    pos  = Pos; 
    vpos = Pos;
    tex0 = Tex;
}
