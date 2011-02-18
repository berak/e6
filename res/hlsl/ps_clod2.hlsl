float4 fTime		: register(c1);
float4 param		: register(c2);
sampler samp0		: register(s0);


float f(float3 o)
{
    float a=(sin(o.x)+o.y*.25)*param.z;
    float sa = sin(a*param.y);
    float ca = cos(a);
    o=float3(ca*o.x-sa*o.y, sa*o.x+ca*o.y, o.z);
    float3 co = cos(o);
    float  d = dot(co*co,float3(param.w,param.w,param.w));
    return d - 1.2;
}

float3 s(float3 o,float3 d)
{
    float steps = param.x*100;
    float bright = param.y;
    float3 e = float3(.1,0.0,0.0);
	float pt = pow(bright, 5.);
    float3 basecol = float3(.93,.79,.4);
    float3 value = float3(.93,.94,.85);

    float t=0.,a,b;
    for(int i=0;i<steps;i++)
    {
        float3 p = o + d * t;
        float v = f(p+e);
        if(v<0.0)
        {
            float3 n = float3(v,f(p+e.yxy),f(p+e.yyx));
            n = -normalize( n );
            float3 light1 = -dot(n,float3(.577,.577,.577));
            float3 light2 = -dot(n,float3(.1,.1,.377));
            float3 light3 = -dot(n,float3(-.407,-.407,0));
            float3 x = (0.3 * max(light1,0.)+ 0.3 * max(light2,0.) + 0.3 * max(light3,0.)) * basecol;
            value = lerp( x, value, float3(pt,pt,pt) ) ;
            break;
        }
        t+=.125;
    }
    return value;
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
    //~ float3 tx = tex2D( samp0, r.xz );
    col = float4(r,1.0);
}


