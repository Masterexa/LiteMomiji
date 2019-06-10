#include "Test.hlsli"

VSOut main(Vertex v)
{
	VSOut vs_out		= (VSOut)0;

	vs_out.position	= mul(v.instance_world, float4(v.position, 1.0));
	vs_out.view		= g_camera_position - vs_out.position.xyz;
	vs_out.position	= mul(g_camera_view,		vs_out.position);
	vs_out.position	= mul(g_camera_projection,	vs_out.position);

	/*
	INCIDENT

	vout.view		= g_camera_position - vout.position.xyz;
	vout.position	= mul(g_camera_view, vert.position);
	vout.position	= mul(g_camera_projection, vert.position);

	vout vert

	*/

	vs_out.normal	= mul(v.instance_world, float4(v.normal, 0.0)).xyz;
	vs_out.tangent	= mul(v.instance_world, float4(v.tangent, 0.0)).xyz;
	vs_out.binormal	= cross(vs_out.normal, vs_out.tangent);
	vs_out.uv0		= v.uv0;
	vs_out.color	= v.color * v.instance_color;
    vs_out.pbs_param = v.instance_pbsparam;

	return vs_out;
}