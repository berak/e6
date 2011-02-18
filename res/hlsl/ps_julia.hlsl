
sampler2D  PalSampler         :  register(s0);

float4     par                :  register(c0); // cx,cy,scale,iter
#define cx x
#define cy y
#define scale z
#define iter w



float4 PS(
    in float2 UV		: TEXCOORD0
) : COLOR
{   
    float2 z;

    z.x = 3.0 * (UV.x - 0.5);
    z.y = 2.0 * (UV.y - 0.5);

    int i;
    for(i=0; i<par.iter; i++) {
        float x = (z.x * z.x - z.y * z.y) + par.cx;
        float y = (z.y * z.x + z.x * z.y) + par.cy;

        if((x * x + y * y) > 4.0) break;
        z.x = x;
        z.y = y;
    }

	float2 t = float2((i == par.iter ? 0.0 : float(i)) / 100.0, 0.5);
    return tex2D(PalSampler, t);
}  

