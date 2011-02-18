float4 fTime		: register(c1);
//~ uniform float4 in_col;
//const float2 rez = float2( 20, 40 );;
//uniform float4 cos_t;
//uniform float4 sin_t;
//~ const float time=dot(in_col.xyz,float3(1.0,256.0,65536.0))*.25;
const float3 e = float3(0.01,.0,.0);
const float top=0.45;

float obj(float3 pos, float4 cos_t, float4 sin_t)
{
	float final=1.0;
	final*=distance(pos,float3(cos_t.x+sin_t.y,0.3,2.0+cos_t.y));
	final*=distance(pos,float3(-cos_t.z,0.3,2.0+sin_t.z));
	final*=distance(pos,float3(-sin_t.y*0.5,sin_t.x,2.0));
	final *=cos(pos.y)*cos(pos.x)-0.1-cos(pos.z*7.+fTime.x*7.)*cos(pos.x*3.)*cos(pos.y*4.)*0.1;
	return final;
}

float4 PS( in float3 vpos: TEXCOORD0 ) : COLOR 
{
	float4 cos_t = float4( cos(fTime.x), cos(fTime.x*0.5)*0.5, cos(fTime.x*0.7), 1);
	float4 sin_t = float4( sin(fTime.x), sin(fTime.x*0.2), sin(fTime.x*0.5), 1);
	//~ float2 v= ( vpos.xy - rez) / rez;
	float2 v= vpos.xy;
	float3 o=float3(v.x,v.y,0.0);
	float3 d=float3(v.x+cos_t.x*.3,v.y,1.0)/64.0;
	float4 color=float4(0.0,0.0,0.0,0.0);
	float t=0.0;
	for(int i=0;i<30;i++) {
		if(obj(o+d*t,cos_t,sin_t)<top){
			t-=5.0;
			for (int j=0; j<5; j++) if (obj(o+d*t,cos_t,sin_t)>=top) t+=1.0;
			float3 n = float3(0.0,0.0,0.0);
            float3 p = o+d*t;
            float op = obj(p,cos_t,sin_t);
			n.y=op-obj(float3(p+e.xyy),cos_t,sin_t);
			n.x=op-obj(float3(p+e.yxy),cos_t,sin_t);
			n.z=op-obj(float3(p+e.yyx),cos_t,sin_t);
			n=normalize(n);
			color+=max(dot(float3(0.6,0.0,-0.5),n),0.0)+max(dot(float3(-0.6,-0.0,-0.5),n),0.0)*0.5;
            color.a=1.0;
            break;
		}
		t+=5.0;
	}
	return color+float4(0.4,0.3,0.2,1.0)*(t*0.025);
}
