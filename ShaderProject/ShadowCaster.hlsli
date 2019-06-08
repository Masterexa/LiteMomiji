
cbuffer ShaderUniforms : register(b0)
{
	float4x4	g_camera_view;
	float4x4	g_camera_projection;
	float4x4	g_camera_position;
}

struct VSOut
{
	float4	position	: SV_Position;
	float	opacity		: TEXCOORD0;
};

struct PSOut
{
	float4	color	: SV_Target;
};