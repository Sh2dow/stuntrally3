static const float2 c_offsets[4] =
{
	float2( -1.0, -1.0 ), float2( 1.0, -1.0 ),
	float2( -1.0,  1.0 ), float2( 1.0,  1.0 )
};

Texture2D<float> lumRt		: register(t0);
SamplerState samplerBilinear: register(s0);

Texture2D<float> oldLumRt	: register(t1);
SamplerState samplerPoint	: register(s1);

float4 main
(
	in float2 uv : TEXCOORD0,
	uniform float3 exposure,
	uniform float adaptSpeed,
	uniform float4 tex0Size
) : SV_Target
{
	float fLumAvg = lumRt.Sample( samplerBilinear, uv + c_offsets[0] * tex0Size.zw ).x;

	for( int i=1; i<4; ++i )
		fLumAvg += lumRt.Sample( samplerBilinear, uv + c_offsets[i] * tex0Size.zw ).x;

	fLumAvg *= 0.25f;

	// Clamp luminance to prevent extreme values
	float clampedLumAvg = clamp( fLumAvg, 0.1f, 5.0f );

	// Calculate new inverse luminance
	float newInvLum = exposure.x / exp( clamp( clampedLumAvg, exposure.y, exposure.z ) );
	
	// Read previous frame and apply temporal adaptation
	float oldInvLum = oldLumRt.SampleLevel( samplerPoint, float2( 0.0, 0.0 ), 0 ).x;

	// Handle first frame (invalid oldInvLum)
	bool validOld = !isnan( oldInvLum ) && !isinf( oldInvLum ) && oldInvLum > 0.0f;
	
	// Use adaptSpeed to control adaptation rate (higher = faster)
	float adaptFactor = adaptSpeed * 0.01f;  // Scale to reasonable range
	float minLum = oldInvLum * (1.0f - adaptFactor);
	float maxLum = oldInvLum * (1.0f + adaptFactor);
	return validOld ? clamp( newInvLum, minLum, maxLum ) : newInvLum;
}
