#include "Common.hlsli"

cbuffer ShaderUniforms : register(b0)
{
    float4x4 g_view_mat;
    float4x4 g_projection_mat;
};
Texture2D       g_tex : register(t0);
SamplerState    g_tex_samp : register(s0);


struct DebugDrawerTextureVertex
{
    float2  position : POSITION;
    float2  uv       : UV;
    float4  rect     : INSTANCE_RECT;
    float   z_order  : INSTANCE_Z_ORDER;
};

struct VSOut
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

struct PSOut
{
    float4 color : SV_Target0;
};