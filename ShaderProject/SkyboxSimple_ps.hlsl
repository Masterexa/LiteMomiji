#include "SkyboxSimple.hlsli"

PSOut main(VSOut pixel)
{
    PSOut psout = (PSOut) 0;
    psout.color = g_env_tex.Sample(g_ao_samp, normalize(pixel.normal));

    return psout;
}