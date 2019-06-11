#pragma once

#include <stdint.h>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <Xinput.h>
#include <stdexcept>
#include <memory>
#include <string>
#include <assert.h>

struct Vertex{
	DirectX::XMFLOAT3	position;
	DirectX::XMFLOAT2	uv;
	DirectX::XMFLOAT4	color;
	DirectX::XMFLOAT3	normal;
	DirectX::XMFLOAT3	tangent;
};

struct Graphics;
struct GraphicsContext;
struct PipelineState;
struct Shader;
struct Mesh;
struct Model;
struct Material;
struct RenderTarget;

struct ImguiModule;
struct DebugDrawer;


template<typename T>
constexpr T getAlignedSize(T size, T align)
{
	return ((size%align!=0) ? (align-size%align) : 0)+size;
}


#define THROW_IF_HFAILED(hr,msg) if( FAILED(hr) ){ throw std::runtime_error(msg); }