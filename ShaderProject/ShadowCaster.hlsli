
cbuffer ShaderUniforms : register(b0)
{
	float4x4	g_camera_view;
	float4x4	g_camera_projection;
	float4x4	g_camera_position;
}

struct Vertex{
	float3		position		: POSITION;
	float4		color			: COLOR;
	float2		uv0				: UV;
	float3		normal			: NORMAL;
	float3		tangent			: TANGENT;
	float4x4	instance_world	: INSTANCE_WORLD;
	float4		instance_color	: INSTANCE_COLOR;
};

struct VSOut
{
	float4	position	: SV_Position;
	float	opacity		: TEXCOORD0;
};

struct PSOut
{
	float4	color	: SV_Target;
};