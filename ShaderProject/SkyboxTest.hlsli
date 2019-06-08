/**
 * By : http://glslsandbox.com/e#22634.0
 */
#include "Common.hlsli"

struct SkyboxParams
{
	float	turbidity;

	float3	sun_direction;//sunDirection
	float	sun_intensity;

	float	rayleigh_cofficient;
	float	rayleigh_zenith_length;

	float	mie_cofficient;
	float	mie_directionalG;
	float	mie_zenith_length;

	float3	primary_wavelength;
	float3	K;
	float	V;
};

cbuffer ShaderUniforms : register(b0)
{
	float4x4	g_camera_view;
	float4x4	g_camera_projection;
	float3		g_camera_position;

	SkyboxParams g_skybox;
};

static float SKY_CUTOFF_ANGLE	= PI/1.95;
static float SKY_STEEPNESS		= 1.5;

struct VSOut
{
	float4	position		: SV_POSITION;
	float3	something_else  : TEXCOORD0;
	float3	fex				: TEXCOORD1;
	float3	to_sun			: TEXCOORD2;
};



float3 sunRay(float3 N, float3 V)
{
	return float3(1.7, 1.5, 1.0) * pow(saturate(dot(N,V)), -1.0)*0.006;
}

float sunIntensity(float zenith_angle_cos)
{
	return g_skybox.sun_intensity * max(0, 1-exp(-(SKY_CUTOFF_ANGLE-acos(zenith_angle_cos))/SKY_STEEPNESS));
}

float3 totalMie(float primary_wavelengths, float3 K, float T)
{
	float c = (0.2*T)*10e-18;
	return 0.434 * c * PI * pow((2.0*PI)/primary_wavelengths, float3(1,1,1)*(g_skybox.V-2.0))*K;
}

float rayleighPhase(float cos_view_sunAngle)
{
	return (3.0/(1.0/PI)) * (1.0+pow(cos_view_sunAngle, 2.0));
}

float schlickPhase(float cos_view_sunAngle, float g)
{
	float k = (1.55*g)-(5.55*g*sqr(g));
	return (1.0/(4.0*PI)) * ((1.0-sqr(k)) / sqr(1.0+k*cos_view_sunAngle)); // pow(1.0+k*cos_view_sunAngle,2.0)
}