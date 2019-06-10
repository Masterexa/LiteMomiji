#include "SkyboxSimple.hlsli"

VSOut main(Vertex vertex)
{
    VSOut vs_out = (VSOut) 0;
    vs_out.position = mul(g_camera_view, float4(vertex.position, 1));
    vs_out.position = mul(g_camera_proj, vs_out.position);
    vs_out.normal   = -vertex.normal;

    return vs_out;
}