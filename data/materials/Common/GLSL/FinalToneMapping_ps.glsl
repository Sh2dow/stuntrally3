#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

//See Hable_John_Uncharted2_HDRLighting.pptx
//See https://expf.wordpress.com/2010/05/04/reinhards_tone_mapping_operator/

/*const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const float W = 11.2;/**/

/*const float A = 0.22;  // saturated
const float B = 0.13;
const float C = 0.10;
const float D = 0.520;
const float E = 0.01;
const float F = 0.130;
const float W = 5.2;/**/

/*const float A = 0.122;  // bright
const float B = 0.43;
const float C = 0.30;
const float D = 0.120;
const float E = 0.1;
const float F = 0.530;
const float W = 11.2;/**/

/**/const float A = 0.22;  // org
const float B = 0.3;
const float C = 0.10;
const float D = 0.20;
const float E = 0.01;
const float F = 0.30;
const float W = 11.2;/**/

vec3 FilmicTonemap( vec3 x )
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
float FilmicTonemap( float x )
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 fromSRGB( vec3 x )
{
	return x * x;
}

vulkan_layout( ogre_t0 ) uniform texture2D rt0;
vulkan_layout( ogre_t1 ) uniform texture2D lumRt;
vulkan_layout( ogre_t2 ) uniform texture2D bloomRt;

vulkan( layout( ogre_s0 ) uniform sampler samplerPoint );
vulkan( layout( ogre_s2 ) uniform sampler samplerBilinear );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float bloomIntensity;
vulkan( }; )

void main()
{
	float fInvLumAvg = texture( vkSampler2D( lumRt, samplerPoint ), vec2( 0.0, 0.0 ) ).x;

	vec4 vSample = texture( vkSampler2D( rt0, samplerPoint ), inPs.uv0 );

	//  Apply real exposure from luminance chain
	vSample.xyz *= fInvLumAvg;
	
	//  Add bloom (already in linear HDR space)
	vSample.xyz	+= fromSRGB( texture( vkSampler2D( bloomRt, samplerBilinear ),
									  inPs.uv0 ).xyz ) * 16.0 * bloomIntensity;
	
	//  Filmic tonemapping (Uncharted 2 style)
	vSample.xyz  = FilmicTonemap( vSample.xyz ) / FilmicTonemap( W );

	//  Carbon: removed hardcoded lift/contrast to preserve authored exposure
	//  Old: vSample.xyz  = ( vSample.xyz - 0.5 ) * 1.25 + 0.5 + 0.11;
	//  The +0.11 was compensating for missing luminance - no longer needed

	fragColour = vSample;
}
