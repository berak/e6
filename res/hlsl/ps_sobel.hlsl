sampler     samp0           : register(s0);


float sobel( sampler RT, float2 TexCoord ) 
{
    // One pixel offset
    const float off = 1.0 / 512.0;
    const float thresholdSqr = 0.0025 * 0.0025;

    // Sample neighbor pixels
    float s00 = tex2D(RT, TexCoord + float2(-off, -off)).r;
    float s01 = tex2D(RT, TexCoord + float2( 0,   -off)).r;
    float s02 = tex2D(RT, TexCoord + float2( off, -off)).r;

    float s10 = tex2D(RT, TexCoord + float2(-off,  0)).r;
    float s12 = tex2D(RT, TexCoord + float2( off,  0)).r;

    float s20 = tex2D(RT, TexCoord + float2(-off,  off)).r;
    float s21 = tex2D(RT, TexCoord + float2( 0,    off)).r;
    float s22 = tex2D(RT, TexCoord + float2( off,  off)).r;

    // Sobel filter in X direction
    float sobelX = s00 + s10 + s10 + s20 - s02 - s12 - s12 - s22;
    // Sobel filter in Y direction
    float sobelY = s00 + s01 + s01 + s02 - s20 - s21 - s21 - s22;

    // Find edge, skip sqrt() to improve performance ...
    float edgeSqr = (sobelX * sobelX + sobelY * sobelY);

    // ... and threshold against a squared value instead.
    return (edgeSqr - thresholdSqr);
}


float4 PS( in float2 TexCoord : TEXCOORD0 ) : COLOR 
{
    return tex2D( samp0, TexCoord ) * sobel( samp0, TexCoord );
}

