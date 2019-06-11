#include "DebugDrawer_Texture.hlsli"

PSOut main(VSOut pixel)
{
    PSOut ps_out = (PSOut) 0;
    ps_out.color = g_tex.Sample(g_tex_samp, pixel.uv);

    return ps_out;
}