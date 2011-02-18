
struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float3 pos        : TEXCOORD0;  // uv1
    float3 eye        : TEXCOORD1;  // uv1
};



//------------------------------------------------------------------------------------------
//
// pixelshader
//
//------------------------------------------------------------------------------------------


sampler2D   tex1;
samplerCUBE tex0;
float4      wave        : register(c0);        // wave1


float4 PS ( VS_OUTPUT inp ) : COLOR
{
    float4 OutColor;
    float3 bump = float3(0,1,0);

    float2 wpos = wave.xy; //  + offset[i];
    float  dist = distance( inp.pos.xz, wpos ) / length(wpos.xy);
    float2 tc   = float2( dist, 1-wave.z );
    bump.xz    += inp.pos.y * 2 * tex2D( tex1, tc ) - 1;

    float3 reflVec = reflect(inp.eye, normalize(bump));
    
    return  texCUBE( tex0, reflVec.zyx ); 
}












