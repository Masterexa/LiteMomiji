#include "Test.hlsli"

static float3	TEST_LIGHT			= {0.0,1.0,0.5};
static float3	TEST_AMBIENT		= {0.1,0.1,0.1};
static float3	TEST_AMBIENT_NADIR	= {0.01,0.01,0.01};


struct BSDFIn{
	float3	N, B, T, V, H, L;

	float3	albedo;
	float	metallic;
	float	roughness;
	float	specular;
	float	specular_tint;
	float	anisotropic;
	float	sheen;
	float	sheen_tint;
	float	clear_coat;
	float	clear_coat_gloss;
	float	occlusion;
};


float fresnel(float v)
{
	float m = clamp(1.0-v, 0.0, 1.0);
	return sqr(m)*sqr(m)*m;
}

float GTR1(float NdH, float a)
{
	if(a>=1)
		return 1/PI;

	float a2 = sqr(a);
	float t = 1+(a2-1)*sqr(NdH);
	return (a2-1)/(PI*log(a2)*t);
}

float GTR2(float NdH, float a)
{
	float a2	= sqr(a);
	float t		= 1 + (a2-1) * sqr(NdH);
	return a2/(PI*sqr(t));
}

float GTR2_aniso(float NdH, float HdX, float HdY, float ax, float ay)
{
	return 1/(PI * ax*ay * sqr(sqr(HdX/ax) + sqr(HdY/ay) + sqr(NdH)));
}

float smithG_GGX(float NdV, float alpha_g)
{
	float a = alpha_g*alpha_g;
	float b = NdV*NdV;
	return 1.0/(NdV + sqrt(a+b - a*b));
}

float smithG_GGX_aniso(float NdV, float VdX, float VdY, float ax, float ay)
{
	return 1 / (NdV + sqrt(sqr(VdX*ax) + sqr(VdY*ay) + sqr(NdV)));
}

float3 physically(in BSDFIn bsdf)
{
	float NdL = max(0,dot(bsdf.N, bsdf.L));
	float NdV = max(0,dot(bsdf.N, bsdf.V));
	float NdH = dot(bsdf.N, bsdf.H);
	float LdH = dot(bsdf.L, bsdf.H);

	float lum		= 0.3*bsdf.albedo.r + 0.6*bsdf.albedo.g + 0.1*bsdf.albedo.b;
	float3 ctint	= bsdf.albedo/lum; //step(a,b) : a less then b
	//float3 ctint	= float3(1,1,1); //step(a,b) : a less then b
	float3 spec0	= lerp(bsdf.specular*0.08*lerp(float3(1, 1, 1), ctint, bsdf.specular_tint), bsdf.albedo, bsdf.metallic);

	// diffuse
	float FL	= fresnel(NdL);
	float FV	= fresnel(NdV);
	float Fd90	= 0.5 + 2*LdH*LdH*bsdf.roughness;
	float Fd	= lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);

	// Specular
	float aspect = sqrt(1-bsdf.anisotropic*0.9);
	float ax	= max(0.001, sqr(bsdf.roughness)/aspect);
	float ay	= max(0.001, sqr(bsdf.roughness)*aspect);
	//float Ds	= GTR2_aniso(NdH, dot(bsdf.H,bsdf.B), dot(bsdf.H,bsdf.T), ax, ay);
	float Ds	= GTR2(NdH, sqr(bsdf.roughness));
	float FH	= fresnel(LdH);
	float3 Fs	= lerp(spec0, float3(1, 1, 1), FH);
	float Gs	=/*
		smithG_GGX_aniso(NdL,dot(bsdf.L,bsdf.B), dot(bsdf.L, bsdf.T), ax, ay)
		* smithG_GGX_aniso(NdV, dot(bsdf.V,bsdf.B), dot(bsdf.V,bsdf.T), ax, ay)*/
		smithG_GGX(NdL, sqr(bsdf.roughness))*smithG_GGX(NdV, sqr(bsdf.roughness))
	;

	// sheen
	float3 sheen = FH*sqr(bsdf.sheen);

	// clear coat
	float Dr = GTR1(NdH, lerp(0.1, 0.001, bsdf.clear_coat_gloss));
	float Fr = lerp(0.04, 1.0, FH);
	float Gr = smithG_GGX(NdL, 0.25) * smithG_GGX(NdV, 0.25);

	return(
		((1/PI) * Fd*bsdf.albedo) * (1-bsdf.metallic)
		+
		Gs*Fs*Ds + 0.25*bsdf.clear_coat*Gr*Fr*Dr
	)*NdL;
}

float3 phong(in BSDFIn bsdf)
{
    float NdL = max(0,dot(bsdf.N, bsdf.L));
	float NdH = max(0,dot(bsdf.N, bsdf.H));

	float lum		= 0.3*bsdf.albedo.r + 0.6*bsdf.albedo.g + 0.1*bsdf.albedo.b;
	float3 ctint	= lerp((bsdf.albedo/lum), float3(1, 1, 1), step(0, lum)); //step(a,b) : a less then b
	float3 spec0	= lerp(bsdf.specular*0.08*lerp(float3(1, 1, 1), ctint, bsdf.specular_tint), bsdf.albedo, bsdf.metallic);
    
    float pw = lerp(10000, 1, sqr(bsdf.roughness));
    float D = (pow(NdH, pw) * (1.0 + pw)) / (2.0 * PI);

    return ((1 / PI)*(1 - bsdf.metallic)*bsdf.albedo + D*spec0)*NdL;
}

float3 hemiAmbient(float3 N, float3 L)
{
	return lerp(TEST_AMBIENT_NADIR, TEST_AMBIENT, dot(N, L)*0.5+0.5);
}

float3 environment(float3 N, float3 V, float3 spec, float m)
{
	float	FV = fresnel(dot(N, V));
	float3	Em = g_env_tex.Sample(g_ao_samp, reflect(-V, N)).rgb;

    return lerp(FV,1,m)*spec*Em*10.0;
}

float3 ambient(float3 N, float3 albedo, float m)
{
    return hemiAmbient(N, float3(0, 1, 0)) * albedo * (1-m);
}

PSOut main(VSOut psin)
{
	PSOut psout = (PSOut)0;

	BSDFIn bsdf = (BSDFIn)0;
	bsdf.N = normalize(psin.normal);
	bsdf.B = normalize(psin.binormal);
	bsdf.T = normalize(psin.tangent);
	bsdf.V = normalize(psin.view);
	bsdf.L = normalize(g_lightdir);
	bsdf.H = normalize(bsdf.L+bsdf.V);

	bsdf.albedo		= psin.color.rgb;
	bsdf.roughness	= 0.1;
	bsdf.specular	= 1.0;
    bsdf.metallic   = 0.0;
	bsdf.anisotropic= 0.0;
	bsdf.occlusion	= lerp(1.0, g_ao_tex.Sample(g_ao_samp, psin.uv0).g, g_ao);

    float lum = 0.3 * bsdf.albedo.r + 0.6 * bsdf.albedo.g + 0.1 * bsdf.albedo.b;
    float3 ctint = lerp((bsdf.albedo / lum), float3(1, 1, 1), step(0, lum)); //step(a,b) : a less then b
    float3 spec0 = lerp(bsdf.specular * 0.08 * lerp(float3(1, 1, 1), ctint, bsdf.specular_tint), bsdf.albedo, bsdf.metallic);

    float3 shadow_coord = psin.shadow.xyz / psin.shadow.w;
    shadow_coord.xy = shadow_coord.xy * 0.5 + 0.5;
    shadow_coord.y  = 1-shadow_coord.y;

    
    float max_depth_slope = max(abs(ddx(shadow_coord.z)), abs(ddy(shadow_coord.z)));
    float bias = 0.002;
    float slope_scaled_bias = 0.1;
    float depth_bias_clamp = 0.1;

    float shadow_bias = min(depth_bias_clamp, bias + slope_scaled_bias * max_depth_slope);
    float shadow_threshold  = g_shadowmap_tex.SampleCmpLevelZero(g_shadow_samp, shadow_coord.xy, shadow_coord.z-shadow_bias);
    


    psout.color.rgb += environment(bsdf.N, bsdf.V, spec0, bsdf.metallic) + ambient(bsdf.N, bsdf.albedo*bsdf.occlusion, bsdf.metallic);
	psout.color.rgb	+= phong(bsdf)*2.0*shadow_threshold;
	psout.color.a	= psin.color.a;

    //psout.color.rgb = float3(1, 1, 1) * shadow_threshold;
    //psout.color.rgb = shadow_coord;
	return psout;
}