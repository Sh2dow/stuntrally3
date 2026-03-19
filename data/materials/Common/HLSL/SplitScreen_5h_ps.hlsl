// SplitScreen_5h_ps.hlsl - Converted from GLSL for Direct3D11

SamplerState sampler1 : register(s0);
SamplerState sampler2 : register(s1);
SamplerState sampler3 : register(s2);
SamplerState sampler4 : register(s3);
SamplerState sampler5 : register(s4);

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);
Texture2D tex3 : register(t2);
Texture2D tex4 : register(t3);
Texture2D tex5 : register(t4);

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float2 uv1 = uv;
    float2 uv2 = uv;
    float2 uv3 = uv;
    float2 uv4 = uv;
    float2 uv5 = uv;

    uv1.y *= 10.0;
    uv2.y -= 1.0/10.0;
    uv2.y *= 10.0;
    uv3.y -= 2.0/10.0;
    uv3.y *= 10.0;
    uv4.y -= 3.0/10.0;
    uv4.y *= 10.0;
    uv5.y -= 4.0/10.0;
    uv5.y *= 10.0;

    if (uv.y < 1.0/10.0)
        return tex1.Sample(sampler1, uv1);
    else if (uv.y < 2.0/10.0)
        return tex2.Sample(sampler2, uv2);
    else if (uv.y < 3.0/10.0)
        return tex3.Sample(sampler3, uv3);
    else if (uv.y < 4.0/10.0)
        return tex4.Sample(sampler4, uv4);
    else
        return tex5.Sample(sampler5, uv5);
}
