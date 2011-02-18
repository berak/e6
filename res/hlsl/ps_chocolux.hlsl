float4 fTime		: register(c1);
sampler samp0		: register(s0);

void PS(
	in  float3 pos  : TEXCOORD0,
	out float4 col  : COLOR
)
{
	int i,j;
	float t,b,c,h=0;
	float tScaled = fTime.x*0.2;
	float3 m,n,p=float3(fTime.y*.5+ .2, fTime.z*.2,0.3);
	float3 d=normalize((fTime.y*.2+.6) * pos.rgb-p);
	float3 s[4];
	s[0]=float3(0,0,0);
	//~ s[3]=float3(sin(abs(fTime.x*.0001)),cos(abs(fTime.x*.0001)),0);
	s[3]=float3(sin(tScaled),cos(tScaled),p.z);
	s[1]=s[3].zxy;
	s[2]=s[3].yzx;
	for(j=0;j<3;j++)
	{
		t=2.6;
		for(i=0;i<2;i++)
		{
			n=s[i]-p;
			b=dot(d,n);
			c=b*b+.2-dot(n,n);
			if(b-c<t)
			{
				if(c>0)
				{
					m=s[i];
					t=b-c;
				}
			}
		}
		p+=t*d;
		n=normalize(p-m);
		d=reflect(d,n);
		h+=pow(n.x*n.x,2.)+n.x*n.x*.32;
	}
	p.x -= tScaled;
	p += n * .002 * fTime.x;
	t =       0.5 * cos(s[3].y*0.1);
	b = 0.5 + 0.5 * b * s[3].y;
	col = tex2D( samp0, p*0.12 + d*.1 );
	//col += .4 * float4(b*b*h*h,cos(.2*t),h*c*h,h);
} 

