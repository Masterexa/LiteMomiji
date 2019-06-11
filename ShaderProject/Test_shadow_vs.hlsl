#include "Test.hlsli"

VSOut main(Vertex v)
{
    VSOut vs_out = (VSOut) 0;
    float4 world = mul(v.instance_world, float4(v.position, 1.0));

    // Position transform to clip space
    vs_out.position = mul(g_shadow_mat, world);

    return vs_out;
}