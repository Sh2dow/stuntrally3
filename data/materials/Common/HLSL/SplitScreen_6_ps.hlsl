// SplitScreen_6_ps.hlsl - Converted from GLSL for Direct3D11

SamplerState sampler1 : register(s0);
SamplerState sampler2 : register(s1);
SamplerState sampler3 : register(s2);
SamplerState sampler4 : register(s3);
SamplerState sampler5 : register(s4);
SamplerState sampler6 : register(s5);

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);
Texture2D tex3 : register(t2);
Texture2D tex4 : register(t3);
Texture2D tex5 : register(t4);
Texture2D tex6 : register(t5);

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float2 uv1 = uv;
    float2 uv2 = uv;
    float2 uv3 = uv;
    float2 uv4 = uv;
    float2 uv5 = uv;
    float2 uv6 = uv;

    uv1.xy *= 2.0;
    uv2.xy -= float2(0.5, 0.0);
    uv2.xy *= 2.0;
    uv3.xy -= float2(0.0, 0.5);
    uv3.xy *= 2.0;
    uv4.xy -= 0.5;
    uv4.xy *= 2.0;
    uv5.xy -= float2(0.5, 0.0);
    uv5.xy = float2(uv5.x, uv5.y * 2.0 - 0.5);
    uv6.xy -= float2(0.0, 0.5);
    uv6.xy = float2(uv6.x * 2.0 - 0.5, uv6.y);

    if (uv.x < 0.5 && uv.y < 0.5)
        return tex1.Sample(sampler1, uv1);
    else if (uv.x >= 0.5 && uv.y < 0.5)
        return tex2.Sample(sampler2, uv2);
    else if (uv.x < 0.5 && uv.y >= 0.5)
        return tex3.Sample(sampler3, uv3);
    else if (uv.x >= 0.5 && uv.y >= 0.5)
        return tex4.Sample(sampler4, uv4);
    else if (uv.x < 0.5)
        return tex5.Sample(sampler5, uv5);
    else
        return tex6.Sample(sampler6, uv6);
}
