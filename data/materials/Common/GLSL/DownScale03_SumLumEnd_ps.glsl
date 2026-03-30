#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out float fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

const vec2 c_offsets[4] = vec2[4]
(
	vec2( -1.0, -1.0 ), vec2( 1.0, -1.0 ),
	vec2( -1.0,  1.0 ), vec2( 1.0,  1.0 )
);

vulkan_layout( ogre_t0 ) uniform texture2D lumRt;
vulkan( layout( ogre_s0 ) uniform sampler samplerBilinear );

vulkan_layout( ogre_t1 ) uniform texture2D oldLumRt;
vulkan( layout( ogre_s1 ) uniform sampler samplerPoint );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec3 exposure;
	uniform float adaptSpeed;
	uniform vec4 tex0Size;
vulkan( }; )

void main()
{
	float fLumAvg = texture( vkSampler2D( lumRt, samplerBilinear ),
							 inPs.uv0 + c_offsets[0] * tex0Size.zw ).x;

	for( int i=1; i<4; ++i )
	{
		fLumAvg += texture( vkSampler2D( lumRt, samplerBilinear ),
							inPs.uv0 + c_offsets[i] * tex0Size.zw ).x;
	}

	fLumAvg *= 0.25;

	// Clamp luminance to prevent extreme values
	float clampedLumAvg = clamp( fLumAvg, 0.1, 5.0 );

	// Calculate new inverse luminance
	float newInvLum = exposure.x / exp( clamp( clampedLumAvg, exposure.y, exposure.z ) );
	
	// Read previous frame and apply temporal adaptation
	float oldInvLum = texture( vkSampler2D( oldLumRt, samplerPoint ), vec2( 0.0, 0.0 ) ).x;

	// Handle first frame (invalid oldInvLum)
	bool validOld = !isnan( oldInvLum ) && !isinf( oldInvLum ) && oldInvLum > 0.0;
	
	// Use adaptSpeed to control adaptation rate (higher = faster)
	float adaptFactor = adaptSpeed * 0.01;  // Scale to reasonable range
	float minLum = oldInvLum * (1.0 - adaptFactor);
	float maxLum = oldInvLum * (1.0 + adaptFactor);
	fragColour = validOld ? clamp( newInvLum, minLum, maxLum ) : newInvLum;
}