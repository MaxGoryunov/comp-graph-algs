cbuffer ModelBuffer : register(b0)
{
    matrix m;
};

cbuffer VPBuffer : register(b1)
{
    matrix vp;
};

cbuffer LightingBuffer : register(b2)
{
    struct Lighting {
        float4 pos;
        float4 color;
        float4 attenuation;
    } Lightings[2];
};

cbuffer CameraBuffer : register(b3)
{
    float4 CameraPos;
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 tex : TEXCOORD;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3x3 tbn : TBN;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;

    float4 worldPos = mul(float4(input.pos, 1.0), m);
    output.worldPos = worldPos.xyz;
    output.pos = mul(worldPos, vp);
    output.tex = input.tex;

    float3 T = normalize(mul(input.tangent, (float3x3)m));
    float3 N = normalize(mul(input.normal, (float3x3)m));

    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T);

    output.tbn = float3x3(T, B, N);

    return output;
}

Texture2D cubeTex : register(t0);
Texture2D normalMap : register(t1);
SamplerState samLinear : register(s0);

float4 PS(PS_INPUT input) : SV_Target
{
    float3 normalMapSample = normalMap.Sample(samLinear, input.tex).rgb;
    normalMapSample = normalize(normalMapSample * 2.0 - 1.0);

    float3 normal = mul(normalMapSample, input.tbn);
    normal = normalize(normal);

    float3 ambient = float3(0.1, 0.1, 0.1);
    float3 diffuse = float3(0, 0, 0);

    for (int i = 0; i < 2; i++)
    {
        float3 lightDir = Lightings[i].pos.xyz - input.worldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        float attenuation = 1.0 / (Lightings[i].attenuation.x +
                                  Lightings[i].attenuation.y * distance +
                                  Lightings[i].attenuation.z * distance * distance);

        float diff = saturate(dot(normal, lightDir));
        diffuse += Lightings[i].color.rgb * diff * attenuation;
    }

    float3 texColor = cubeTex.Sample(samLinear, input.tex).rgb;

    float3 result = (ambient + diffuse) * texColor;
    return float4(result, 1.0);
}