sampler2D   samp0     : register(s0);


struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float4 Diffuse    : COLOR0;     // vertex diffuse color
    float3 pos        : TEXCOORD0;  // uv1
    float3 eye        : TEXCOORD1;  // uv1
};


//------------------------------------------------------------------------------------------
//
// pixelshader
//
//------------------------------------------------------------------------------------------






float4 PS ( VS_OUTPUT inp ) : COLOR
{
    float4 OutColor;

    float2 wpos = float2(cos(inp.eye.z*23.9),cos(inp.eye.x*13));
    float  dist = distance( inp.pos.xz, wpos ) / length(wpos);
    float2 tc   = float2( (dist), (inp.pos.y) );

    float2 tx =  2.0 * tex2D( samp0, tc ) - 1;
    float2 ty =  2.0 * tex2D( samp0, 0.2*tc-wpos ) - 1;
    OutColor.x = tx.x;
    OutColor.y = 0.1*tx.x;
    OutColor.z = ty.x;
    OutColor.a = ty.y;
    OutColor.z *= dist;
    OutColor.a *= (1-(-inp.pos.z)) * inp.Diffuse.a;

    return OutColor;
}












