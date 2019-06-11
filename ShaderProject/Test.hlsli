#include "Common.hlsli"
/* Uniforms */

cbuffer ShaderUniforms : register(b0)
{
    float4x4    g_camera_view;
    float4x4    g_camera_projection;
    float4x4    g_shadow_mat;
    float3      g_camera_position;
    float       g_ao;
    float3      g_lightdir;
};

Texture2D		g_ao_tex	    : register(t0);
TextureCube		g_env_tex	    : register(t1);
Texture2D		g_shadowmap_tex	: register(t2);

SamplerState	        g_ao_samp	    : register(s0);
SamplerComparisonState	g_shadow_samp	: register(s1);


#if 0

cbuffer MaterialUniforms : register(b1)
{
};


cbuffer ObjectUniforms : register(b2)
{
};

#endif

struct Vertex
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv0 : UV;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4x4 instance_world : INSTANCE_WORLD;
    float4 instance_color : INSTANCE_COLOR;
    float4 instance_pbsparam : INSTANCE_PBS_PARAM;
};



/* Structures */

struct VSOut
{
	float4	position	: SV_Position;
	
    float3	normal		: NORMAL0;
    float3 binormal     : NORMAL1;
    float3 tangent      : NORMAL2;
    float3 view         : NORMAL3;

	float2	uv0			: TEXCOORD0;
	float4	color		: TEXCOORD1;
    //float4  pbs_param : TEXCOORD2;

    float4 shadow : SHADOW0;
};

struct PSOut
{
	float4	color	: SV_Target;
};
