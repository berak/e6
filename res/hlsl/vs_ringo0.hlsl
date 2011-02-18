//------------------------------------------------------------------------------------------
//
// vertexshader
//
//------------------------------------------------------------------------------------------

float4x4    mWorldViewProj      : register(c0);  // World * View * Projection transformation
float4x4    mView               : register(c8);  //
float3      vCamera             : register(c11);
float4      mdlDiffuse          : register(c13);
float4      fTime               : register(c22);        // SysTime, sinTime, cosTime, deltaTime
//~ float4      timeScale           : register(c40);        // wave1
float4      scale               : register(c33);


//~ float4      offset[4] = 
//~ {
    //~ 0.3,0.1,0.4,0,
    //~ -0.3,0.1,0.12,0,
    //~ -0.3,-0.4,0.2,0,
    //~ 0.3,0.1,-0.12,0
//~ };

struct VS_INPUT
{
    float4 Position   : POSITION;   // vertex position 
//    float3 Normal    : NORMAL;
//    float2 Texcoord : TEXCOORD0;  // uv1
};


struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float4 Diffuse    : COLOR0;     // vertex diffuse color
    float3 pos        : TEXCOORD0;  // uv1
    float3 eye        : TEXCOORD1;  // uv1
};


VS_OUTPUT VS( in VS_INPUT inp )
{
 	VS_OUTPUT Output;
    
    const float       depth = 1.0;

    float3 up    = float3(0,1,0);//mView[2].xyz;
    float3 right = mView[0].xyz;
    float3 front = mView[2].xyz;

    // Billboard the quads with respect to viewmatrix.
    //float4 Pos   = float4( ( ( inp.Position.x * right + inp.Position.z * front + inp.Position.y * up ) * depth ), 1 );
    float4 Pos   = inp.Position * 40.0;
    Pos.w = 1.0;
    Output.Position = mul(  mWorldViewProj, Pos );
    Output.Diffuse = mdlDiffuse;
    float3 dist =  (Pos-vCamera);
    Output.eye.yxz = normalize(dist);
    Output.pos.x = (0.5+((Pos.x*scale.x)*0.5));
    Output.pos.z = (0.5+((Pos.z*scale.z)*0.5));
    Output.pos.y = scale.y;
    
//    Output.pos = Pos * scale;
    return Output;
}


