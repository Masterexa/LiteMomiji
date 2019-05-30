#include "Test.hlsli"

static float	PI				= 3.14159;
static float3	TEST_LIGHT		= {0.0,1.0,-1.0};
static float3	TEST_AMBIENT	= {0.1,0.1,0.1};


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
};

#define sqr(x) (x*x)

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

float smithG_GGX(float NdV, float alpha_g)
{
	float a = alpha_g*alpha_g;
	float b = NdV*NdV;
	return 1.0/(NdV + sqrt(a+b - a*b));
}

float3 physically(in BSDFIn pin)
{
	float NdL = dot(pin.N, pin.L);
	float NdV = dot(pin.N, pin.V);
	float NdH = dot(pin.N, pin.H);
	float LdH = dot(pin.L, pin.H);
	if(NdL<0 || NdV<0)
		return (float3)0;

	float lum		= 0.3*pin.albedo.r + 0.6*pin.albedo.g + 0.1*pin.albedo.b;
	float3 ctint	= pin.albedo/lum; //step(a,b) : a less then b
	//float3 ctint	= float3(1,1,1); //step(a,b) : a less then b
	float3 spec0	= lerp(pin.specular*0.08*lerp(float3(1, 1, 1), ctint, pin.specular_tint), pin.albedo, pin.metallic);

	// diffuse
	float FL	= fresnel(NdL);
	float FV	= fresnel(NdV);
	float Fd90	= 0.5 + 2*LdH*LdH*pin.roughness;
	float Fd	= lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);

	// Specular
	float bi_r	= max(0.001, sqr(pin.roughness));
	float Ds	= GTR2(NdH, bi_r);
	float FH	= fresnel(LdH);
	float3 Fs	= lerp(spec0, float3(1, 1, 1), FH);
	float Gs	= smithG_GGX(NdL,bi_r)*smithG_GGX(NdV, bi_r);

	// sheen
	float3 sheen = FH*sqr(pin.sheen);

	// clear coat
	float Dr = GTR1(NdH, lerp(0.1, 0.001, pin.clear_coat_gloss));
	float Fr = lerp(0.04, 1.0, FH);
	float Gr = smithG_GGX(NdL, 0.25) * smithG_GGX(NdV, 0.25);

	return(
		((1/PI) * Fd*pin.albedo) * (1-pin.metallic)
		+
		Gs*Fs*Ds + 0.25*pin.clear_coat*Gr*Fr*Dr
	)*NdL;
}

float3 phong(in BSDFIn pin)
{
	float NdL = saturate(dot(pin.N, pin.L));
	float NdH = saturate(dot(pin.N, pin.H));

	float lum		= 0.3*pin.albedo.r + 0.6*pin.albedo.g + 0.1*pin.albedo.b;
	float3 ctint	= lerp((pin.albedo/lum), float3(1, 1, 1), step(0, lum)); //step(a,b) : a less then b
	float3 spec0	= lerp(pin.specular*0.08*lerp(float3(1, 1, 1), ctint, pin.specular_tint), pin.albedo, pin.metallic);


	return (pin.albedo*(1-pin.metallic) + pow(NdH,lerp(1000,1,pin.roughness))*spec0 ) * NdL;
}


PSOut main(VSOut psin)
{
	PSOut psout = (PSOut)0;

	BSDFIn bsdf = (BSDFIn)0;
	bsdf.N = normalize(psin.normal);
	bsdf.V = normalize(psin.view);
	bsdf.L = normalize(TEST_LIGHT);
	bsdf.H = normalize(bsdf.L+bsdf.V);

	bsdf.albedo		= psin.color.rgb;
	bsdf.roughness	= 0.05;
	bsdf.specular	= 0.5;

	psout.color.rgb	= bsdf.albedo*TEST_AMBIENT + physically(bsdf);
	psout.color.a	= psin.color.a;


	return psout;
}