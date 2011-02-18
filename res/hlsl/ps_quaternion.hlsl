sampler     samp0     : register(s0);
float4      fTime     : register(c1);

float jinteresct(in float3 rO, in float3 rD, in float4 c, out float ao)
{
    float mz2,md2,dist,t;
    float res=1000.0;
    float4 z,nz;

    ao = 0.0;
    for(t=0.0;t<6.0;t+=dist)
    {
        ao += 1.0;
        float3 p=rO+t*rD;

        // calc distance
        z=float4(p,(c.y+c.x)*.3);
        md2=1.0;
        mz2=dot(z,z);

        for(int i=0;i<9;i++)
        {
             // |dz|^2 -> 4*|dz|^2
             md2*=4.0*mz2;
             // z -> z2 + c
             nz.x=z.x*z.x-dot(z.yzw,z.yzw);
             nz.yzw=2.0*z.x*z.yzw;
             z=nz+c;

             mz2=dot(z,z);
             if(mz2>4.0)
                 break;
         }

         dist=0.25*sqrt(mz2/md2)*log(mz2);

         if(dist<0.0005)
         {
             res=t;
             break;
         }

     }

    return res;
}


float3 calcNormal(in float3 p, in float4 c)
{
    float4 nz,ndz,dz[4];

    float4 z=float4(p,(c.y+c.x)*.3);

    dz[0]=float4(1.0,0.0,0.0,0.0);
    dz[1]=float4(0.0,1.0,0.0,0.0);
    dz[2]=float4(0.0,0.0,1.0,0.0);
  //dz[3]=float4(0.0,0.0,0.0,1.0);

    for(int i=0;i<9;i++)
    {
        float4 mz = float4(z.x,-z.y,-z.z,-z.w);
        // derivative
        dz[0]=float4(dot(mz,dz[0]),z.x*dz[0].yzw+dz[0].x*z.yzw);
        dz[1]=float4(dot(mz,dz[1]),z.x*dz[1].yzw+dz[1].x*z.yzw);
        dz[2]=float4(dot(mz,dz[2]),z.x*dz[2].yzw+dz[2].x*z.yzw);
        //dz[3]=float4(dot(mz,dz[3]),z.x*dz[3].yzw+dz[3].x*z.yzw);

        // z = z2 + c
        nz.x=dot(z, mz);
        nz.yzw=2.0*z.x*z.yzw;
        z=nz+c;

        if(dot(z,z)>4.0)
            break;
        }

    return normalize(float3(dot(z,dz[0]),dot(z,dz[1]),dot(z,dz[2])));
}


void PS
(
    in  float3 vpos: TEXCOORD0,
    out float4 col:  COLOR
)
{
    float  time = fTime.x;
    float2 p = vpos.xy;
    float3 color = float3(0.0,0.0,0.0);
    float4 cccc = float4( .7*cos(.43*time), .7*sin(.23*time), 0.27*cos(.51*time), 0.0 );
    float3 edir = normalize(float3(p,1.0));
    float3 wori = float3(0.0,0.0,-2.0);

    float ao;
    float t = jinteresct(wori,edir,cccc,ao);
    if(t<100.0)
    {
        float3 inter = wori + t*edir;
        float3 nor = calcNormal(inter,cccc);

        float dif = .5 + .5*dot( nor, float3(0.27703,0.67703,0.57703) );
        ao = max( 1.0-ao*0.005, 0.0 );

        color = float3(1.0,.9,.5)*dif*ao +  .5*float3(.6,.7,.8)*ao;
    }
    else
    {
        color = float3(0.5,0.51,0.62)+float3(0.5,0.47,0.45)*p.y;
    }
   
    col.rgb = color;
    col.a = 1.0;
}



