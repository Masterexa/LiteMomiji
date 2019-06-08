

#define sqr(x) (x*x)
static float	PI		= 3.141592653;

struct Vertex{
	float3		position		: POSITION;
	float4		color			: COLOR;
	float2		uv0				: UV;
	float3		normal			: NORMAL;
	float3		tangent			: TANGENT;
	float4x4	instance_world	: INSTANCE_WORLD;
	float4		instance_color	: INSTANCE_COLOR;
};