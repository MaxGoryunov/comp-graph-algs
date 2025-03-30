cbuffer ModelBuffer : register(b0)
{
    matrix m;
};

cbuffer VPBuffer : register(b1)
{
    matrix vp;
};

struct VS_INPUT
{
    float3 pos : POSITION;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(float4(input.pos, 1.0), m);
    output.pos = mul(output.pos, vp);
    return output;
}
cbuffer ColorBuffer : register(b0)
{
    float4 color;
};

float4 PS(PS_INPUT input) : SV_Target
{
    // use alpha
    return float4(color.rgb, color.a);
}