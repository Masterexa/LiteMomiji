#pragma once

#include <stdint.h>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <stdexcept>

struct Vertex{
	DirectX::XMFLOAT3	position;
	DirectX::XMFLOAT2	uv;
	DirectX::XMFLOAT4	color;
	DirectX::XMFLOAT3	normal;
	DirectX::XMFLOAT3	tangent;
};

class Graphics;

#define THROW_IF_HFAIL(hr,msg) if( FAILED(hr) ){ throw std::runtime_error(msg); }