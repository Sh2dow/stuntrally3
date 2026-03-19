// SR3_SunBeams_ps.hlsl - Converted from GLSL for Direct3D11

SamplerState depthSampler : register(s0);
SamplerState texSampler : register(s1);

Texture2D depthTexture : register(t0);
Texture2D sceneTexture : register(t1);

cbuffer Params : register(b0)
{
    float4 uvSunPos_Fade;  // .w aspect
    float4 efxClrRays;
};

float getDepth(float2 uv, float val)
{
    float depth = depthTexture.Sample(depthSampler, uv).x * 1000.0;
    return (depth < 0.01) ? val : 0.0;
}

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float2 uvScaled = uv;
    float2 uvSun = uvSunPos_Fade.xy + float2(0.5, 0.5);
    float fade = pow(uvSunPos_Fade.z, 3.0);

    float sum = getDepth(uvSun, 1.0);

    float2 uvStep = (uvScaled - uvSun) * (1.0 / 64.0 * 0.97);
    float illumDecay = 1.0;

    for (int i = 0; i < 64; i++)
    {
        uvScaled -= uvStep;
        float s = getDepth(uvScaled, 0.2);
        s *= illumDecay * 0.25;
        sum += s;
        illumDecay *= 0.97;
    }

    float4 fragColor = sceneTexture.Sample(texSampler, uv);
    float3 rays = efxClrRays.xyz * sum * 0.24 * fade;
    fragColor.xyz += rays;

    return fragColor;
}
