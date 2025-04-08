cbuffer VPBuffer : register(b0)
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
    float3 tex : TEXCOORD;
};

PS_INPUT SkyboxVS(VS_INPUT input)
{
    PS_INPUT output;
    float4 pos = float4(input.pos, 1.0);
    output.pos = mul(pos, vp);
    output.tex = input.pos;
    return output;
}

TextureCube skyboxTexture : register(t0);
SamplerState samLinear : register(s0);


float4 SkyboxPS(PS_INPUT input) : SV_Target
{
    return skyboxTexture.Sample(samLinear, input.tex);
}