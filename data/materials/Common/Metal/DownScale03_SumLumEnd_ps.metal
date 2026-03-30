#include <metal_stdlib>
using namespace metal;

constexpr constant float2 c_offsets[4] =
{
	float2( -1.0, -1.0 ), float2( 1.0, -1.0 ),
	float2( -1.0,  1.0 ), float2( 1.0,  1.0 )
};

struct PS_INPUT
{
	float2 uv0;
};

struct Params
{
	float3 exposure;
	float adaptSpeed;
	float4 tex0Size;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texture2d<float>				lumRt			[[texture(0)]],
	sampler							samplerBilinear	[[sampler(0)]],

	texture2d<float>				oldLumRt		[[texture(1)]],
	sampler							samplerPoint	[[sampler(1)]],

	constant Params &p [[buffer(PARAMETER_SLOT)]]
)
{
	float fLumAvg = lumRt.sample( samplerBilinear, inPs.uv0 + c_offsets[0] * p.tex0Size.zw ).x;

	for( int i=1; i<4; ++i )
		fLumAvg += lumRt.sample( samplerBilinear, inPs.uv0 + c_offsets[i] * p.tex0Size.zw ).x;

	fLumAvg *= 0.25f;

	// Clamp luminance to prevent extreme values
	float clampedLumAvg = clamp( fLumAvg, 0.1f, 5.0f );

	// Calculate new inverse luminance
	float newInvLum = p.exposure.x / exp( clamp( clampedLumAvg, p.exposure.y, p.exposure.z ) );
	
	// Read previous frame and apply temporal adaptation
	float oldInvLum = oldLumRt.sample( samplerPoint, float2( 0.0, 0.0 ) ).x;

	// Handle first frame (invalid oldInvLum)
	bool validOld = !isnan( oldInvLum ) && !isinf( oldInvLum ) && oldInvLum > 0.0f;
	
	// Use adaptSpeed to control adaptation rate (higher = faster)
	float adaptFactor = p.adaptSpeed * 0.01f;  // Scale to reasonable range
	float minLum = oldInvLum * (1.0f - adaptFactor);
	float maxLum = oldInvLum * (1.0f + adaptFactor);
	return float4( validOld ? clamp( newInvLum, minLum, maxLum ) : newInvLum, 1.0f, 1.0f, 1.0f );
}
