
/* Uniforms */

#if defined(VERTEX_SHADER)

cbuffer ShaderUniforms : register(b0)
{
	float4x4	g_camera_view;
	float4x4	g_camera_projection;
	float3		g_camera_position;
};

#endif

#if 0

cbuffer MaterialUniforms : register(b1)
{
};

#endif

#if 0

cbuffer ObjectUniforms : register(b2)
{
};

#endif



/* Structures */


struct Vertex{
	float3		position		: POSITION;
	float4		color			: COLOR;
	float2		uv0				: UV;
	float3		normal			: NORMAL;
	float4x4	instance_world	: INSTANCE_WORLD;
	float4		instance_color	: INSTANCE_COLOR;
};

struct VSOut
{
	float4	position	: SV_Position;
	float3	normal		: TEXCOORD0;
	float3	view		: TEXCOORD1;
	float2	uv0			: TEXCOORD2;
	float4	color		: TEXCOORD3;
};

struct PSOut
{
	float4	color	: SV_Target;
};
