sampler   samp3d            : register(s0);

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float3 Tex: TEXCOORD0;
    float4 Col: TEXCOORD1;
};

float4 PS(VS_OUTPUT In) : COLOR 
{
    return In.Col * tex3D( samp3d, In.Tex  );
}

