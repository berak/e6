
sampler2D  SceneSampler         :  register(s0);
sampler3D  NoiseSampler         :  register(s1);


float4     Mix                  :  register(c0);
#define interpol x
#define distort y
#define scale z


float4 PS(
    in float3 UV		: TEXCOORD0,
    in float3 UV2		: TEXCOORD1
) : COLOR
{   
	float2 off1 = tex3D(NoiseSampler, UV  * Mix.scale).xy;
	float2 off2 = tex3D(NoiseSampler, UV2 * Mix.scale).xy;	// zw?
	float2 result = lerp( off1, off2, Mix.interpol );
	float2 d = Mix.distort * result;
	float2 nuv = (float2(UV.x, -UV.y) + d) * 0.97;
    return tex2D(SceneSampler,nuv);
}  

