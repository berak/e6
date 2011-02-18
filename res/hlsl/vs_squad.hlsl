float4x4  mWorldViewProj        : register(c0);  // World * View * Projection transformation

struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float2 Texcoord   : TEXCOORD0;  // uv1
};


VS_OUTPUT VS( 
   in float4 Position : POSITION,
   in float2 Texcoord : TEXCOORD0,
   )
{
 	VS_OUTPUT Output;

    Output.Position = float4( Position.x, Position.z, 0, 1 );
    Output.Texcoord = Texcoord; //0.5 + 0.5 * float2( Position.x , -Position.z ); 

    return Output;
}

