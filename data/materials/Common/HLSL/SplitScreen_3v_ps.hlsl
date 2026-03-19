// SplitScreen_3v_ps.hlsl - Converted from GLSL for Direct3D11

SamplerState sampler1 : register(s0);
SamplerState sampler2 : register(s1);
SamplerState sampler3 : register(s2);

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);
Texture2D tex3 : register(t2);

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float2 uv1 = uv;
    float2 uv2 = uv;
    float2 uv3 = uv;

    uv1.x *= 5.0;
    uv2.x -= 1.0/5.0;
    uv2.x *= 5.0;
    uv3.x -= 2.0/5.0;
    uv3.x *= 5.0;

    if (uv.x < 1.0/5.0)
        return tex1.Sample(sampler1, uv1);
    else if (uv.x < 2.0/5.0)
        return tex2.Sample(sampler2, uv2);
    else
        return tex3.Sample(sampler3, uv3);
}
