#include "Common.hlsli"
/* Uniforms */

cbuffer ShaderUniforms : register(b0)
{
	float4x4	g_camera_view;
	float4x4	g_camera_projection;
	float3		g_camera_position;
	float		g_ao;
};

Texture2D		g_ao_tex	: register(t0);
TextureCube		g_env_tex	: register(t1);
SamplerState	g_ao_samp	: register(s0);


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
	float3	normal		: TEXCOORD0;
	float3	binormal	: TEXCOORD1;
	float3	tangent		: TEXCOORD2;
	float3	view		: TEXCOORD3;
	float2	uv0			: TEXCOORD4;
	float4	color		: TEXCOORD5;
    float4  pbs_param : TEXCOORD6;
};

struct PSOut
{
	float4	color	: SV_Target;
};
