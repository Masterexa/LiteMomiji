#include "DebugDrawer_Line.hlsli"

VSOut main(DebugDrawerLineVertex vertex)
{
	VSOut vs_out = (VSOut)0;

	vs_out.position = mul(g_view_mat, float4(vertex.position,1));
	vs_out.position = mul(g_projection_mat, vs_out.position);
	vs_out.color	= vertex.color;
	
	return vs_out;
}