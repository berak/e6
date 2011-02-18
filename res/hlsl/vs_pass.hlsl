//
// pass thru:
//

void VS(  in  float4 iPos : POSITION
        , in  float2 iTex : TEXCOORD
        , out float4 oPos : POSITION
        , out float2 oTex : TEXCOORD )
{
    oPos = iPos;
    oTex = iTex;
}

