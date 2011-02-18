float4x4  mWorldViewProj        : register(c0);  // World * View * Projection transformation

struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float2 Texcoord   : TEXCOORD0;  // uv1
};


VS_OUTPUT VS( in float4 Position : POSITION )
{
 	VS_OUTPUT Output;

    Output.Position = mul( mWorldViewProj, Position );
    Output.Texcoord = float2( 0.2*Position.x + 4, Position.y ); 

    return Output;
}

