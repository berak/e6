//------------------------------------------------------------------------------------------
//
// vertexshader
//
//------------------------------------------------------------------------------------------

float4x4    mWorldViewProj      : register(c0);  // World * View * Projection transformation
float3      worldCamera         : register(c7);
float4      scale               : register(c33);


struct VS_INPUT
{
    float4 Position   : POSITION;   // vertex position 
    float3 Normal    : NORMAL;
    float2 Texcoord : TEXCOORD0;  // uv1
};


struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float3 pos        : TEXCOORD0;  // uv1
    float3 eye        : TEXCOORD1;  // uv1
};


VS_OUTPUT VS( in VS_INPUT inp )
{
 	VS_OUTPUT Output;
    

    float3 Pos = inp.Position;
  
    Output.Position = mul( mWorldViewProj,  float4( Pos.x, Pos.y, Pos.z, 1.0f ) );
    Output.eye = normalize(worldCamera-Pos);
    Output.pos.xz = 0.5+((Pos.xz*scale.xz)*0.5);
    Output.pos.y = scale.y;
    
    return Output;
}
