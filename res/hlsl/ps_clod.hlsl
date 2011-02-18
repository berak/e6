float4 fTime		: register(c1);
sampler samp0		: register(s0);

float f(float3 o)
{
    float a=(sin(o.x)+o.y*.25)*.35;
    o=float3(cos(a)*o.x-sin(a)*o.y,sin(a)*o.x+cos(a)*o.y,o.z);
    return dot(cos(o)*cos(o),float3(1,1,1))-1.2;
}
float3 s(float3 o,float3 d)
{
    float t=0.,a,b;
    for(int i=0;i<75;i++)
    {
        if(f(o+d*t)<0.0)
        {
            a=t-.125;b=t;
            for(int i=0; i<10;i++)
            {
                t=(a+b)*.5;
                if(f(o+d*t)<0.0)
                    b=t;
                else 
                    a=t;
            }
            float3 e = float3(.1,0.0,0.0);
            float3 p = o + d * t;
			float sp = sin( p * 75. );
            float3 n = -normalize( float3(f(p+e),f(p+e.yxy),f(p+e.yyx))+float3(sp,sp,sp)*.01);
			float pt = pow(t/9.,5.);
            return float3(
				lerp( ((max(-dot(n,float3(.577,.577,.577)),0.)
					  + 0.125 * max(-dot(n,float3(-.707,-.707,0)),0.)))
							 * (fmod(length(p.xy)*20.,2.)<1.0?float3(.71,.85,.25):float3(.79,.93,.4))
                           ,float3(.93,.94,.85), float3(pt,pt,pt) ) 
			);
        }
        t+=.125;
    }
    return float3(.93,.94,.85);
}

void PS(
	in  float3 pos  : TEXCOORD0,
	out float4 col  : COLOR
)
{
	float t = fTime.x;
	float3 a = float3(sin(t*1.5)*.5,cos(t)*.5,t);
	float3 b = normalize(float3(pos.xy,1.0));
	float3 r = s( a, b );
    col = float4(r,1.0);
}


