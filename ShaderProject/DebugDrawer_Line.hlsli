#include "Common.hlsli"

cbuffer ShaderUniforms : register(b0)
{
	float4x4 g_view_mat;
	float4x4 g_projection_mat;
};

struct DebugDrawerLineVertex
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VSOut{
	float4 position : SV_Position;
	float4 color : TEXCOORD0;
};

struct PSOut{
	float4 color : SV_Target0;
};