sampler     samp0     : register(s0);
float4      fTime     : register(c1);

float sierp(float3 q)
{
    int w= 3;
    float size= (1+q.z/2)*0.25;
    float x = q.x-abs(q.x) - size;//(int(q.x*w)%w);
    float y = q.y-abs(q.y) - size;//(int(q.x*w)%w);
    float z = q.z-abs(q.z) - size;//(int(q.x*w)%w);
    return max(max(abs(x)-size,abs(y)-size),abs(z)-size);
}

float cubus(float x,float y,float z,float size,float angle){
    float px=x,py=y,pz=z;
    float sa=sin(angle);
    float ca=cos(angle);
    // y Rotate 
    x = px*ca+pz*sa;
    z = pz*ca-px*sa; 
    px = x;
    // z Rotate 
    x = x*ca+py*sa;
    y = y*ca-px*sa; 
    return max(max(abs(x)-size,abs(y)-size),abs(z)-size);
}


float oa(float3 q)
{
    float4 p = float4(0.5,-0.8,1.2,0.25);
    float3 d = p.xyz-q;
    return cubus( d.x, d.y, d.z, (1+d.z/2)*0.25,fTime.x*0.1);
}

float ob(float3 q)
{
    float4  s = float4(0.2,0.5,0.8,0.25);
    float3  o = float3( .6*sin(fTime.x),0,.6*cos(fTime.x*1.2) );
    float3  d = s.xyz + o - q;
    float len = sqrt(d.x*d.x + d.y*d.y + d.z*d.z) - s.w*(1-d.z/2);
    return (len);
}

float oc(float3 q)
{
    float4  s = float4(-0.5,-0.6,0.7,0.30);
    float3  o = float3( 0,0,.3*cos(fTime.x*0.6) );
    float3  d = s.xyz + o - q;
    float len = sqrt(d.x*d.x + d.y*d.y + d.z*d.z) - s.w;
    return (len);
}
//~ //Object B (ribbon)
//~ float ob(float3 q)
//~ {
    //~ return length(max(abs(q-float3(cos(q.z*1.5)*.3,-.5+cos(q.z)*.2,.0))-float3(.125,.02,fTime.x+3.),float3(.0,.0,.0)));
//~ }

//Scene
float o(float3 q)
{
//    return sierp(q);
    return min(min(oa(q),ob(q)),oc(q));
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

    float3 l   = float3(-.4,1,-0.2);
    float3 l2  = float3(.4,1, 0.2);

    float3 p   = float3(vpos.x,vpos.y,0);
    float3 org = float3(p.x,p.y+0.4,p.z);
    float3 dir = float3(0,-0.3,0.7),q=org;
    float  d = 0.0;
    int nray = 23;
    //First raymarching
    for(int i=0;i<nray&&d>=0;i++)
    {
        d=o(q);
        q+=d*dir;
    }
    float f=1-length(org-q)*0.5;
    float3 n = gn(q);
    
    //Second raymarching (reflection)
    p = q;// cache
    dir=reflect(dir,n);
    q+=dir;
    for(int j=0;j<nray&&d>=0;j++)
    {
        d=o(q);
        q+=d*dir;
    }
    n = gn(q);
    float g=1.0/length(p-q);
    f = f*0.8 + g*0.031;
    float light1 = (1 - dot(-n,l ));
    float light2 = (1 - dot(-n,l2));
    //Final Color
    col = float4(0,f*light1,f*light2,1);
    //col = float4(f,f*light1,f,1);
}

