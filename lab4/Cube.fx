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
    float2 tex : TEXCOORD;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    float4 pos = float4(input.pos, 1.0);
    output.pos = mul(pos, m);
    output.pos = mul(output.pos, vp);
    output.tex = input.tex;
    return output;
}

Texture2D cubeTex : register(t0);
SamplerState samLinear : register(s0);

float4 PS(PS_INPUT input) : SV_Target
{
    return cubeTex.Sample(samLinear, input.tex);
}
