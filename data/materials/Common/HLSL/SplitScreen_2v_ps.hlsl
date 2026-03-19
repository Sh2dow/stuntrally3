// SplitScreen_2v_ps.hlsl - Converted from GLSL for Direct3D11

SamplerState sampler1 : register(s0);
SamplerState sampler2 : register(s1);

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float2 uv1 = uv;
    float2 uv2 = uv;

    uv1.x *= 2.0;
    uv2.x -= 0.5;
    uv2.x *= 2.0;

    return (uv.x < 0.5)
        ? tex1.Sample(sampler1, uv1)
        : tex2.Sample(sampler2, uv2);
}
