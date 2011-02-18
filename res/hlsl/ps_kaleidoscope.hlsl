sampler     samp0     : register(s0);
float4      fTime     : register(c1);


void PS
(
    in  float3 vpos: TEXCOORD0,
    out float4 col:  COLOR
)
{
    float2 p = vpos.xy;
    float2 uv;
    
   
    float a = atan2(p.y,p.x);
    float r = sqrt(dot(p,p));
    float time = fTime.x;

    uv.x =          7.0*a/3.1416;
    uv.y = -time+ sin(7.0*r+time) + .7*cos(time+7.0*a);

    float w = .5+.5*(sin(time+7.0*r)+ .7*cos(time+7.0*a));

    float3 c  =  tex2D(samp0,uv*.5).xyz;
    col.rgb = c;// * r;
    col.a = 1.0;
}

