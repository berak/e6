static const int MaxSamples = 16;
static float SampleOffsetsWeights[MaxSamples];

struct vsInput
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
};

struct vsOutput
{
    float4 position  : POSITION;
    float2 uv0 : TEXCOORD0;
};

sampler BloomSampler = sampler_state
{
    Texture = <DiffMap1>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
};

sampler BloomSampler2 = sampler_state
{
    Texture = <DiffMap2>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
};


float
GaussianDistribution(in const float x, in const float y, in const float rho)
{
    const float pi = 3.1415927f;
    float g = 1.0f / sqrt(2.0f * pi * rho * rho);
    g *= exp(-(x * x + y * y) / (2 * rho * rho));
    return g;
}


//------------------------------------------------------------------------------
/**
    UpdateSamplesBloom

    Get sample offsets and weights for a horizontal or vertical bloom filter.
    This is normally executed in the pre-shader.
*/
void
UpdateSamplesBloom(in bool horizontal, in int texSize, in float deviation, in float multiplier, out float3 sampleOffsetsWeights[MaxSamples])
{
    float tu = 1.0f / (float) texSize;

    // fill center texel
    float weight = multiplier * GaussianDistribution(0.0f, 0.0f, deviation);
    sampleOffsetsWeights[0]  = float3(0.0f, 0.0f, weight);
    sampleOffsetsWeights[15] = float3(0.0f, 0.0f, 0.0f);

    // fill first half
    int i;
    for (i = 1; i < 8; i++)
    {
        if (horizontal)
        {
            sampleOffsetsWeights[i].xy = float2(i * tu, 0.0f);
        }
        else
        {
            sampleOffsetsWeights[i].xy = float2(0.0f, i * tu);
        }
        weight = multiplier * GaussianDistribution((float)i, 0, deviation);
        sampleOffsetsWeights[i].z = weight;
    }

    // mirror second half
    for (i = 8; i < 15; i++)
    {
        sampleOffsetsWeights[i] = sampleOffsetsWeights[i - 7] * float3(-1.0f, -1.0f, 1.0f);
    }
}

//------------------------------------------------------------------------------
//  Pixel shader which performs a horizontal bloom effect.
//------------------------------------------------------------------------------
float4 psBloomHori(const vsOutput psIn) : COLOR
{
    // preshader...
    UpdateSamplesBloom(true, DisplayResolution.x, 3.0f, 2.0f, SampleOffsetsWeights);

    // shader...
    int i;
    float4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (i = 0; i < MaxSamples; i++)
    {
        color += SampleOffsetsWeights[i].z * tex2D(SourceSampler, psIn.uv0 + SampleOffsetsWeights[i].xy);
    }
    return color;
}

//------------------------------------------------------------------------------
//  Pixel shader which performs a vertical bloom effect.
//------------------------------------------------------------------------------
float4 psBloomVert(const vsOutput psIn) : COLOR
{
    // preshader...
    UpdateSamplesBloom(false, DisplayResolution.y, 3.0f, 2.0f, SampleOffsetsWeights);

    // shader...
    int i;
    float4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (i = 0; i < MaxSamples; i++)
    {
        color += SampleOffsetsWeights[i].z * tex2D(SourceSampler, psIn.uv0 + SampleOffsetsWeights[i].xy);
    }
    return color;
}
