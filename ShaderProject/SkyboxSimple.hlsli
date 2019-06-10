cbuffer ShaderUniforms : register(b0)
{
    float4x4 g_camera_view;
    float4x4 g_camera_proj;
};
Texture2D g_ao_tex : register(t0);
TextureCube g_env_tex : register(t1);
SamplerState g_ao_samp : register(s0);

struct Vertex
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv0 : UV;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VSOut
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
};

struct PSOut
{
    float4 color : SV_Target;
};
