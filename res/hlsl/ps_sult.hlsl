
// Inputs changed into more readable constants:
float shaderparm=5.0, fov=.9, pitch=0.0, heading=90.0, dheading=0.0;
float3 lightdir=float3(1,1,1), position=float3(0,0,1), speed=float3(0,0,1.5);

float  tmax=10.0;
float3 diffdark= float3(.19,.2,.24), difflight=float3(1,1,1), 
     diffrefl= float3(.45,.01,0),  background=float3(.17,0,0);

// constants for the other worm tunnel part:
//float shaderparm=8, fov=.8, pitch=0, heading=-90, dheading=0;
//float3 lightdir=float3(1,1,1), position=float3(0,0,0), speed=float3(0,0,0);

// shadertoy input
float4 fTime : register(c1);;


float3 rotatey(float3 r, float v)
{
	return float3(r.x*cos(v)+r.z*sin(v),r.y,r.z*cos(v)-r.x*sin(v));
}

float3 rotatex(float3 r, float v)
{
	return float3(r.y*cos(v)+r.z*sin(v),r.x,r.z*cos(v)-r.y*sin(v));
}

float2 eval(float3 p)
{
	////// this is the (only) part that changes for the scenes in Sult
	float t = fTime.x,r,c=0.0,g,r2,r3;
	float3 pp;
	p += ( sin(p.zxy*1.7+t)+sin(p.yzx+t*3.) )*.2;
	if (shaderparm<6.0)
		c = length(p.xyz*float3(1,1,.1)-float3(0,-.1,t*.15-.3))-.34;
	else
		c = length(p.xy+float2(.0,.7))-.3+ (sin(p.z*17.0+t*.6)+sin(p.z*2.0)*6.0)*.01;

	p.xy = float2( atan2(p.x,p.y)*1.113, 1.6-length(p.xy)-sin(t*2.0)*.3);
	pp = frac(p.xzz+.5).xyz -.5; pp.y=(p.y-.35)*1.3;
	r = max( abs(p.y-.3)-.05, abs(length(frac(p.xz)-.5)-.4)-.03);
	float2 res;
	res.x = step(c,r);
	res.y = min(min(r,c),p.y-.2);
	return res;
}
//////////


void PS
(
	in  float3 position	: TEXCOORD0,
	out float4 col	: COLOR
)
{
	float2 p = position;
	float3 vdir= normalize(
			   rotatey(rotatey(float3(p.y*fov,p.x*fov*1.33,1),
			   -pitch*.035).yxz,(heading+dheading*fTime.x)*.035));
	float3 vpos= position + speed*fTime.x;

	float cf=1.0,rf=0.0,t,stp,tmin=0.0,c,r,m,d,f;
	float3 e=float3(.01,0,0),cx=e.yyy,n;
	while (cf>.1)
	{
		for (t=tmin,stp=1.0;t<tmax && stp>.005;t+=stp)
		{
			stp = eval(vpos+vdir*t).y;
		}
		if (t<tmax)
		{ 
			vpos+= vdir*t;
			float2 ev = eval(vpos);
			c = ev.y;
			m = ev.x;
			n= normalize(-float3(c-eval(vpos+e.xyy).y,c-eval(vpos+e.yxy).y,	c-eval(vpos+e.yyx).y));
			r= clamp(eval(vpos+n*.05).y*4.+eval(vpos+n*.1).y*2.0+.5,.1,1.); // ao

			// shade
			rf = m*.3;
			n= normalize(n+step(4.,shaderparm)*m*sin(vpos.yzx*40.0)*.05);
			vdir=reflect(vdir,n);
			d=clamp(dot(normalize(lightdir),n),.0,1.);

			f = 0.7 * pow( clamp( dot( normalize(lightdir),vdir),.0,1.) ,12.);
			n= lerp(lerp(diffdark,difflight,d),diffrefl*(d+.2), m)	+float3(f,f,f); // n = col..

			cx += cf* lerp(n*r, background, t/tmax);
			cf*= rf*(1.0-t/tmax);
			tmin= .1;
		}
		else
		{
			cx += cf*background;
			cf=0.0;
		}
	}
	col.rgb= cx;
	col.a= 1.0;
}
