#include "Common.hlsli"


cbuffer ShaderUniforms : register(b0)
{
    float4x4 g_view_mat;
    float4x4 g_projection_mat;
    float3 g_camera_position;
};

cbuffer MaterialUniforms : register(b1)
{
    float4 _BaseColor;
}

cbuffer ObjectUniforms : register(b2)
{
    float4x4 g_world_mat;
}

Texture2D _MainTex : register(t0);
Texture2D _ToonRamp : register(t1);






struct Vertex
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv0 : UV;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct SurfaceVSOut
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 binormal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 view : TEXCOORD3;
    float2 uv0 : TEXCOORD4;
};

struct OutlineVSOut
{
    float4 position : SV_Position;
    float4 color : TEXCOORD0;
};

struct PSOut
{
    float4 color : SV_Target;
};