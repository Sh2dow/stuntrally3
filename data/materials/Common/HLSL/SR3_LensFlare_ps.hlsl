// SR3_LensFlare_ps.hlsl - Converted from GLSL for Direct3D11

SamplerState depthSampler : register(s0);
SamplerState texSampler : register(s1);

Texture2D depthTexture : register(t0);
Texture2D sceneTexture : register(t1);

cbuffer Params : register(b0)
{
    float4 uvSunPos_Fade;  // .w aspect
    float4 efxClrSun;
};

float getDepth(float2 uv)
{
    float depth = depthTexture.Sample(depthSampler, uv).x * 1000.0;
    return (depth < 0.01) ? 0.0 : 1.0;
}

float3 lensflare(float2 uv, float2 pos)
{
    float2 main_uv = uv - pos;
    float2 uvd = uv * (length(uv));

    float dist = length(main_uv), distUV = dist;
    dist = pow(dist, 0.1);
    float ang = atan2(main_uv.x, main_uv.y);

    float f0 = 1.0 / (distUV * 64.0 + 1.0);
    f0 = f0 + f0 * (sin(sin(ang * 2.0 + pos.x) * 4.0 - cos(ang * 3.0 + pos.y) * 16.0) * 0.1 + dist * 0.1 + 0.8);

    float f1 = max(0.01 - pow(length(uv + 1.2 * pos), 1.9), 0.0) * 7.0;

    float f2 = max(1.0 / (1.0 + 32.0 * pow(length(uvd + 0.8 * pos), 2.0)), 0.0) * 0.25;
    float f22 = max(1.0 / (1.0 + 32.0 * pow(length(uvd + 0.85 * pos), 2.0)), 0.0) * 0.23;
    float f23 = max(1.0 / (1.0 + 32.0 * pow(length(uvd + 0.9 * pos), 2.0)), 0.0) * 0.21;

    float2 uvx = lerp(uv, uvd, -0.5);

    float f4 = max(0.01 - pow(length(uvx + 0.4 * pos), 2.4), 0.0) * 6.0;
    float f42 = max(0.01 - pow(length(uvx + 0.45 * pos), 2.4), 0.0) * 5.0;
    float f43 = max(0.01 - pow(length(uvx + 0.5 * pos), 2.4), 0.0) * 3.0;

    uvx = lerp(uv, uvd, -0.4);

    float f5 = max(0.01 - pow(length(uvx + 0.2 * pos), 5.5), 0.0) * 2.0;
    float f52 = max(0.01 - pow(length(uvx + 0.4 * pos), 5.5), 0.0) * 2.0;
    float f53 = max(0.01 - pow(length(uvx + 0.6 * pos), 5.5), 0.0) * 2.0;

    uvx = lerp(uv, uvd, -0.5);

    float f6 = max(0.01 - pow(length(uvx - 0.3 * pos), 1.6), 0.0) * 6.0;
    float f62 = max(0.01 - pow(length(uvx - 0.325 * pos), 1.6), 0.0) * 3.0;
    float f63 = max(0.01 - pow(length(uvx - 0.35 * pos), 1.6), 0.0) * 5.0;

    float3 c = float3(0.0, 0.0, 0.0);

    c.r += f2 + f4 + f5 + f6;
    c.g += f22 + f42 + f52 + f62;
    c.b += f23 + f43 + f53 + f63;

    c = c * 1.3 - float3(length(uvd) * 0.05, length(uvd) * 0.05, length(uvd) * 0.05);
    c += float3(f0, f0, f0);

    return c * c;
}

float3 mod_clr(float3 color, float factor, float factor2)
{
    float w = color.x + color.y + color.z;
    return lerp(color, float3(w, w, w) * factor, w * factor2);
}

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float2 uvSun = uv;
    float2 uvScaled = uv - float2(0.5, 0.5);
    uvSun.x *= uvSunPos_Fade.w;

    float2 uvSun2 = uvSunPos_Fade.xy + float2(0.5, 0.5);

    // HQ smooth depth sampling
    float sum = getDepth(uvSun2) +
                getDepth(uvSun2 + float2(-0.0008, 0.0)) +
                getDepth(uvSun2 + float2(0.0008, 0.0)) +
                getDepth(uvSun2 + float2(0.0, -0.0008)) +
                getDepth(uvSun2 + float2(0.0, 0.0008));
    float dim = max(0.0, 1.0 - sum / 5.0);

    float3 color = efxClrSun.xyz * lensflare(uvScaled, uvSun);
    color = mod_clr(color, 0.5, 0.1) * dim * uvSunPos_Fade.z;

    float4 fragColor = sceneTexture.Sample(texSampler, uv);
    fragColor.xyz += color;

    return fragColor;
}
