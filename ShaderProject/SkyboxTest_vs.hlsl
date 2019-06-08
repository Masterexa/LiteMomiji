#include "SkyboxTest.hlsli"

VSOut main(Vertex vertex)
{
	VSOut vsout=(VSOut)0;

	vsout.position	= float4(vsout.position.xyz,1.0);
	float3 view		= g_camera_position - vsout.position.xyz;
	vsout.position	= mul(g_camera_view, float4(vsout.position.xyz,0.0));
	vsout.position	= mul(g_camera_view, vsout.position);
	vsout.position	= mul(g_camera_projection, vsout.position);


	return vsout;
}