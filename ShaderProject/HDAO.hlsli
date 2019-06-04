
cbuffer PostEffectUniforms : register(b1)
{
	float4	g_scene_param;
	float4	g_proj_param;
	float	g_hdao_check_radius;
	float	g_hdao_reject_depth;
	float	g_hdao_accept_depth;
	float	g_hdao_intensity;
	float	g_hdao_accept_angle;
	int		g_use_normal;
	float	g_normal_scale;
};

Texture2D		tex_normal_depth		: register(t0);
Texture2D		tex_half_normal_depth	: register(t1);
Texture2D		tex_hdao				: register(t2);
SamplerState	g_sampler_point			: register(s0);
SamplerState	g_sampler_linear		: register(s1);


#define RING_MAX			(4)
#define NUM_RING_1_GATHERS	(2)
#define NUM_RING_2_GATHERS	(6)
#define NUM_RING_3_GATHERS	(12)
#define NUM_RING_4_GATHERS	(20)

// sampling pattern
static const float2 k_ring_pattern[NUM_RING_4_GATHERS]=
{
	// ring1
	{1,-1},
	{0,1},

	// ring 2
	{0,3},
	{2,1},
	{3,-1},
	{1,-3},

	// ring 3
	{1,-5},
	{3,-3},
	{5,-1},
	{4,1},
	{2,3},
	{0,5},

	// ring 4
	{0,7},
	{2,5},
	{4,3},
	{6,1},
	{7,-1},
	{5,-3},
	{3,-5},
	{1,-7},
};