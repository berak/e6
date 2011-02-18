

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float3 Nor: NORMAL;
    float2 Tex: TEXCOORD0;
	float3 light0Dir   : TEXCOORD1;
};


float4 toonify(float intensity) {

	float4 color;

	if (intensity > 0.98)
		color = float4(0.8,0.8,0.8,1.0);
	else if (intensity > 0.5)
		color = float4(0.4,0.4,0.8,1.0);	
	else if (intensity > 0.25)
		color = float4(0.2,0.2,0.4,1.0);
	else
		color = float4(0.1,0.1,0.1,1.0);		

	return(color);
}

float4 PS( VS_OUTPUT inp ) : COLOR
{

	float3 n = normalize(inp.Nor);
	float intensity = max(dot(inp.light0Dir,n),0.0); 
	float4 c = float4(0.2, 0.2, 0.2, 0);
	return c + toonify( intensity );
}

