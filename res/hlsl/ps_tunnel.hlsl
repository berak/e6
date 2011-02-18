sampler     samp0     : register(s0);
float4      fTime     : register(c1);


void PS
(
    in  float3 vpos: TEXCOORD0,
    out float4 col:  COLOR
)
{
    //~ float2 p = -1.0 + 2.0 * vpos.xy;
    float2 p = vpos.xy;
    float2 uv;
   
    float a = atan2(p.y,p.x);
    float r = sqrt(dot(p,p));
    float t = fTime.x * 0.2;
 
    float s = 0.5 + 0.5*cos(7.0*a);
    s = smoothstep(0.0,1.0,s);
    s = smoothstep(0.0,1.0,s);
    s = smoothstep(0.0,1.0,s);
    s = smoothstep(0.0,1.0,s);

    uv.x = t + 1.0/( r + sin(.3*t)*.02*s);
//    uv.x = t + .1/r;
    uv.y = a/3.1416 - sin(t + (1-r)/3.0);

    float3 c = tex2D(samp0,uv);
    col.rgb = c * r;
    col.a = 1.0;
}

