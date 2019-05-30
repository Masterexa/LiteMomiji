#include "Test.hlsli"

VSOut main(Vertex v)
{
	VSOut vout		= (VSOut)0;

	vout.position	= mul(v.instance_world, float4(v.position, 1.0));
	vout.view		= g_camera_position - vout.position.xyz;
	vout.position	= mul(g_camera_view, vout.position);
	vout.position	= mul(g_camera_projection, vout.position);

	/*
	INCIDENT

	vout.view		= g_camera_position - vout.position.xyz;
	vout.position	= mul(g_camera_view, vert.position);
	vout.position	= mul(g_camera_projection, vert.position);

	vout vert

	*/

	vout.normal		= mul(v.instance_world, float4(v.normal, 0.0)).xyz;
	vout.uv0		= v.uv0;
	vout.color		= v.color * v.instance_color;

	return vout;
}