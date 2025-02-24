//

// Constants

cbuffer ConstantBufferWorld : register(b0)
{
    matrix mWorld;
};

cbuffer ConstantBufferViewProjection : register(b1)
{
    matrix mView;
    matrix mProjection;
};



// Vertex Shader

struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Color : COLOR;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

VS_OUTPUT VS(float4 Pos : POSITION, float4 Color : COLOR)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul(Pos, mWorld);
    output.Pos = mul(output.Pos, mView);
    output.Pos = mul(output.Pos, mProjection);
    output.Color = Color;
    return output;
}



// Pixel Shader

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

float4 PS(VS_OUTPUT input) : SV_Target
{
    return input.Color;
}