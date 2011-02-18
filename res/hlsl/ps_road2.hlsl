sampler     samp0     : register(s0);
float4      fTime     : register(c1);


//Object A (tunnel)
float oa(float3 q)
{
    return cos(q.x)+cos(q.y*1.5)+cos(q.z)+cos(q.y*20.)*.05;
}

//Object B (ribbon)
float ob(float3 q)
{
    return length(max(abs(q-float3(cos(q.z*1.5)*.3,-.5+cos(q.z)*.2,.0))-float3(.125,.02,fTime.x+3.),float3(.0,.0,.0)));
}

//Scene
float o(float3 q)
{
    return min(oa(q),ob(q));
}

//Get Normal
float3 gn(float3 q)
{
    float3 f=float3(.01,0,0);
    return normalize(float3(o(q+f.xyy),o(q+f.yxy),o(q+f.yyx)));
}

void PS
(
    in  float3 vpos: TEXCOORD0,
    out float4 col:  COLOR
)
{
    float time = fTime.x;
    float2 p = vpos.xy;
    float3 org = float3(sin(time)*.5,cos(time*.5)*.25+.25,time),dir=normalize(float3(p.x*1.6,p.y,1.0)),q=org,pp;
    float  d = .0;

    //First raymarching
    for(int i=0;i<64;i++)
    {
        d=o(q);
        q+=d*dir;
    }
    pp=q;
    float f=length(q-org)*0.02;

    //Second raymarching (reflection)
    dir=reflect(dir,gn(q));
    q+=dir;
    for(int j=0;j<64;j++)
    {
        d=o(q);
        q+=d*dir;
    }
    float3 c = max(dot(gn(q),float3(.1,.1,.0)),.0)
             + float3(.3,cos(time*.5)*.5+.5,sin(time*.5)*.5+.5)*min(length(q-org)*.04,1.);

    //Ribbon Color
    if(oa(pp)>ob(pp))c=lerp(c,float4(cos(time*.3)*.5+.5,cos(time*.2)*.5+.5,sin(time*.3)*.5+.5,1.),.3);

    //Final Color
    col.xyz = ((c+float3(f,f,f))
            + (1.-min(pp.y+1.9,1.))*float3(1.,.8,.7))*min(time*.5,1.);
    col.a = 1;
}

