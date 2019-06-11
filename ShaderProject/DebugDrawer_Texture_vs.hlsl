#include "DebugDrawer_Texture.hlsli"

VSOut main(DebugDrawerTextureVertex vertex)
{
    VSOut vs_out = (VSOut) 0;
    vs_out.position.z   = vertex.z_order;
    vs_out.position.xy = vertex.position + vertex.rect.xy;
    //vs_out.position.xy  = (vertex.position * vertex.rect.zw) + vertex.rect.xy;
    //vs_out.position     = mul(g_projection_mat, vs_out.position);
    vs_out.uv           = vertex.uv;
    return vs_out;
}