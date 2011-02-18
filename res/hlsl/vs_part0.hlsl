//-----------------------------------------------------------------------------
// vertexshader definitions
//-----------------------------------------------------------------------------
float4x4    mWorldViewProj  : register(c0);			// World * View * Projection transformation
float4x4    mView           : register(c8);			// View transformation
float4		vViewPosition   : register(c11);		// View pos
float4		cDiffuse        : register(c13);		// color

float4      fTime 		    : register(c22);		// t, sin(t), cos(t), dt
float4      particleConsts  : register(c25);        // speed, depth, width, 0

struct VS_INPUT 
{
    float4 Pos: POSITION;
};

struct VS_OUTPUT 
{
    float4 Pos: POSITION;
    float2 Tex: TEXCOORD0;
    float4 Col: TEXCOORD1;
    float  amp: TEXCOORD2;
};


VS_OUTPUT VS( VS_INPUT In )
{
    VS_OUTPUT Out;

    float  speed = particleConsts.x;
    float  depth = particleConsts.y;
    float  width = particleConsts.z;
    float3 up    = mView[2].xyz;
    float3 right = mView[0].xyz;
    float3 front = mView[1].xyz;

    // Billboard the quads with respect to viewmatrix.
    float4 pos   = float4( ( ( In.Pos.x * right + In.Pos.z * front ) * depth * In.Pos.y ), 1 );

    
    // Move the quads around along some odd path
    float t = fTime.x + speed * In.Pos.y;
    pos.x += width * cos(1.24 * t) * sin(3.12 * t);
    pos.y += width * sin(2.97 * t) * cos(0.81 * t);
    pos.z += width * cos(t) * sin(t * 1.231);
    
    Out.Pos = mul(mWorldViewProj, pos);
    Out.Tex = In.Pos.xz;
    Out.Col = In.Pos.y * cDiffuse;
    Out.amp = In.Pos.y;
    
    return Out;
}

