
float beat                      : register(c9); // ps-constant !
float bands[8]                  : register(c10); // ps-constant !


struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float2 Texcoord   : TEXCOORD0;  // uv1
};


// Color from vertical distance to function value:
float4 PS(VS_OUTPUT In) : COLOR 
{
   float f1 = In.Texcoord.y - bands[ In.Texcoord.x + 0.02 ];
   float f2 = In.Texcoord.y - bands[ In.Texcoord.x - 0.02 ];
   float d = saturate(1 - 0.2 * (f1*f2) );
   return float4( beat,  d*d, 0, 1);
}

