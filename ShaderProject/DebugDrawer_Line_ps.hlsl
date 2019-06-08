#include "DebugDrawer_Line.hlsli"

PSOut main(VSOut pixel)
{
	PSOut ps_out=(PSOut)0;
	ps_out.color = pixel.color;

	return ps_out;
}