#include <metal_stdlib>
using namespace metal;

//See Hable_John_Uncharted2_HDRLighting.pptx
//See http://filmicgames.com/archives/75
//See https://expf.wordpress.com/2010/05/04/reinhards_tone_mapping_operator/
/*constexpr constant float A = 0.15f;
constexpr constant float B = 0.50f;
constexpr constant float C = 0.10f;
constexpr constant float D = 0.20f;
constexpr constant float E = 0.02f;
constexpr constant float F = 0.30f;
constexpr constant float W = 11.2f;*/
constexpr constant float A = 0.22f;
constexpr constant float B = 0.3f;
constexpr constant float C = 0.10f;
constexpr constant float D = 0.20f;
constexpr constant float E = 0.01f;
constexpr constant float F = 0.30f;
constexpr constant float W = 11.2f;

inline float3 FilmicTonemap( float3 x )
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
inline float FilmicTonemap( float x )
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

inline float3 fromSRGB( float3 x )
{
	return x * x;
}

struct PS_INPUT
{
	float2 uv0;
};

struct Params
{
	float bloomIntensity;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texture2d<float>				rt0				[[texture(0)]],
	texture2d<float, access::read>	lumRt			[[texture(1)]],
	texture2d<float>				bloomRt			[[texture(2)]],
	sampler							samplerPoint	[[sampler(0)]],
	sampler							samplerBilinear	[[sampler(2)]],
	constant Params&				params			[[buffer(0)]]
)
{
	float fInvLumAvg = lumRt.read( uint2( 0, 0 ) ).x;

	float4 vSample = rt0.sample( samplerPoint, inPs.uv0 );

	//  Apply real exposure from luminance chain
	vSample.xyz *= fInvLumAvg;
	
	//  Add bloom (already in linear HDR space)
	vSample.xyz	+= fromSRGB( bloomRt.sample( samplerBilinear, inPs.uv0 ).xyz ) * 16.0 * params.bloomIntensity;
	
	//  Filmic tonemapping (Uncharted 2 style)
	vSample.xyz  = FilmicTonemap( vSample.xyz ) / FilmicTonemap( W );
	
	//  Carbon: removed hardcoded lift/contrast to preserve authored exposure
	//  Old: vSample.xyz  = ( vSample.xyz - 0.5f ) * 1.25f + 0.5f + 0.11f;
	//  The +0.11 was compensating for missing luminance - no longer needed

	return vSample;
}
