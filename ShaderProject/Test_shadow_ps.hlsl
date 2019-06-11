#include "Test.hlsli"

PSOut main(VSOut psin)
{
    PSOut psout = (PSOut) 0;
    psout.color.r = psin.position.z / psin.position.w;
    return psout;
}