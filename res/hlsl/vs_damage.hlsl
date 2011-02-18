float4x4 mWorldViewProj  : register(c0);
float4   Anim            : register(c32); // (Timer*Speed), (.1+Timer*Speed), QuadTexOffset/(QuadScreenSize.x),QuadTexOffset/(QuadScreenSize.y)
//////////////



void VS(
		in  float3 inPosition   : POSITION, 
		in  float3 inTexCoord   : TEXCOORD0,

        out float4 outPosition	: POSITION,
        out float3 outUV		: TEXCOORD0,
        out float3 outUV2		: TEXCOORD1
) 
{
    outPosition = mul( mWorldViewProj, float4(inPosition,1) );
    outUV  = float3((inTexCoord.xy),Anim.x); 
    outUV2 = float3((inTexCoord.xy),Anim.y); 
}

