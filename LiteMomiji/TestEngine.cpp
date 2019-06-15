#include "TestEngine.hpp"
#include "Model.hpp"
#include "DebugDrawer.hpp"
#include <iostream>
#include <thread>
#include <stdexcept>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <d3dcompiler.h>
#include <imgui.h>
#include <DirectXTex.h>

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"xinput.lib")
#pragma comment(lib,"DirectXTex.lib")

namespace{
	using Microsoft::WRL::ComPtr;
	using namespace DirectX;
	using namespace DirectX::PackedVector;

	const TCHAR* WNDCLASS_NAME = TEXT("TinyMomiji");

	TestEngine*	g_main=nullptr;

	constexpr XMVECTORF32 VECTOR_FORWARD	={0,0,1,0};
	constexpr XMVECTORF32 VECTOR_BACKWARD	={0,0,-1,0};
	constexpr XMVECTORF32 VECTOR_RIGHT		={1,0,0,0};
	constexpr XMVECTORF32 VECTOR_LEFT		={-1,0,0,0};
	constexpr XMVECTORF32 VECTOR_UP			={0,1,0,0};
	constexpr XMVECTORF32 VECTOR_DOWN		={0,-1,0,0};
}

template<typename T>
struct OctreeElement
{
	size_t	first_child;
	T		data;
};


struct InstancingData
{
	XMFLOAT4X4	world;
	XMFLOAT4	color;
	XMFLOAT4	pbs_param;
};
struct ShaderUniforms
{
	XMFLOAT4X4	view;
	XMFLOAT4X4	projection;
	XMFLOAT4X4	shadow_mat;
	XMFLOAT3	camera_position;
	float		ao;
	XMFLOAT3	lightdir;
};


const D3D12_INPUT_ELEMENT_DESC VERTEX_ELEMENTS[]=
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

	{"INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
	{"INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
	{"INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
	{"INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
	{"INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
	{"INSTANCE_PBS_PARAM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
};
const size_t VERTEX_ELEMENTS_COUNT = sizeof(VERTEX_ELEMENTS)/sizeof(D3D12_INPUT_ELEMENT_DESC);



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	return g_main->wndProc(hwnd, msg, wp, lp);
}

LRESULT TestEngine::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if(m_imgui && m_imgui->sendWindowMessage(hwnd, msg, wp, lp))
	{
		return 1;
	}

	switch(msg)
	{
		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}



int TestEngine::run(int argc, char** argv)
{
	setlocale(LC_ALL, "ja_jp");
	m_phase			= 0.9f;
	m_time_delta	= m_time_counted = 0;
	m_time_prev		= Timer::now();
	m_config.width	= 1280;
	m_config.height	= 720;

	m_cnt			= 0;
	m_instance		= GetModuleHandle(nullptr);

	m_config.instancing_count	= 49;
	m_cam_pos.x = 0;
	m_cam_pos.y = 1.f;
	m_cam_pos.z = 30.f;
	m_cam_rot.x = m_cam_rot.y = m_cam_rot.z = 0.f;
	m_cam_rot.y = 3.14159f;
	m_cam_fov	= 60.f;
	m_ao		= 1.0f;

	m_shadowmap_resolution = XMINT2(1024, 1024);
	m_light_rot = XMFLOAT3(0.967f, 0.f, 0.f);

	try{
		initWindow();
		initGraphics();
		initResources();

		m_imgui.reset(new ImguiModule());
		m_imgui->init(m_gfx.get(), m_hwnd, 2, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		ImGui::StyleColorsDark();
		m_demo_window = false;

		m_debug_drawer.reset(new DebugDrawer());
		m_debug_drawer->init(m_gfx.get());
		m_debug_drawer->setResolution(m_config.width, m_config.height);

		m_input.reset(new Input());
	}
	catch(std::exception& e)
	{
		printf("%s\n", e.what());
		getchar();
		return -1;
	}


	// Main loop
	MSG msg;
	while(true)
	{
		// OS process
		auto ret = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
		if(ret)
		{
			// exit application if receive quit message.
			if(msg.message==WM_QUIT)
			{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// game process

		auto now = Timer::now();
		std::chrono::duration<double> dur = now-m_time_prev;
		m_time_prev		= now;
		m_time_delta	= dur.count();
		m_time_counted	+= m_time_delta;

		m_input->update();
		update();
		render();
		std::this_thread::yield();
	}
	this->shutdown();


	return 0;
}

void TestEngine::initWindow()
{
	// Register Window class
	{
		WNDCLASSEX	wc;
		wc.cbSize			= sizeof(wc);
		wc.hInstance		= m_instance;
		wc.lpszClassName	= WNDCLASS_NAME;
		wc.lpfnWndProc		= WndProc;
		wc.style			= CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
		wc.cbWndExtra		= 0;
		wc.cbClsExtra		= 0;
		wc.hCursor			= nullptr;
		wc.hIcon			= LoadIcon(nullptr, IDI_INFORMATION);
		wc.hIconSm			= wc.hIcon;
		wc.hbrBackground	= (HBRUSH)GetStockObject(GRAY_BRUSH);
		wc.lpszMenuName		= nullptr;
		RegisterClassEx(&wc);
	}

	// Create the main window
	{
		g_main = this;
		m_hwnd = CreateWindow(
			WNDCLASS_NAME, TEXT("TinyMomiji"), WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
			CW_USEDEFAULT, CW_USEDEFAULT, m_config.width, m_config.height,
			nullptr, nullptr, m_instance, 0
		);
		// Resize the window
		{
			RECT wr, cr;
			GetClientRect(m_hwnd, &cr);
			GetWindowRect(m_hwnd, &wr);

			int w = (wr.right-wr.left)-(cr.right-cr.left) + m_config.width;
			int h = (wr.bottom-wr.top)-(cr.bottom-cr.top) + m_config.height;
			SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOMOVE|SWP_NOZORDER);
			ShowWindow(m_hwnd, SW_SHOW);
		}
	}
}

void TestEngine::initGraphics()
{
	HRESULT hr;

	m_gfx.reset(new Graphics());
	m_gfx->init();

	m_gfx_context.reset(new GraphicsContext());
	m_gfx_context->init(m_gfx.get());

	auto device		= m_gfx->m_device.Get();
	auto factory	= m_gfx->m_dxgi_factory.Get();


	{
	}


	// Create Swapchain
	{
		SwapchainDesc desc={};
		desc.create_with_depthstencil	= true;
		desc.use_srgb					= true;

		auto& sw_desc = desc.dxgi_swapchain_desc;
		ZeroMemory(&sw_desc, sizeof(sw_desc));
		sw_desc.BufferCount			= 2;
		sw_desc.BufferDesc.Width	= m_config.width;
		sw_desc.BufferDesc.Height	= m_config.height;
		sw_desc.BufferDesc.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;
		sw_desc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sw_desc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sw_desc.OutputWindow		= m_hwnd;
		sw_desc.SampleDesc.Count	= 1;
		sw_desc.Windowed			= TRUE;
		sw_desc.BufferDesc.RefreshRate.Numerator	= 0;
		sw_desc.BufferDesc.RefreshRate.Denominator	= 1;

		m_swapchain.reset(new Swapchain());
		m_swapchain->init(m_gfx.get(), &desc);
	}
	m_vertex_views.reserve(8);
	m_vertex_views.resize(2);
}

void TestEngine::initResources()
{
	HRESULT hr;
	auto device = m_gfx->m_device.Get();

	// PLANE
	{
		MeshInitDesc desc={};
		Vertex		mesh_vrts[4];
		uint32_t	idxs[]={0,1,2,2,3,0};
		vertexFillQuad(mesh_vrts, 4, 0, XMVectorSet(0, 0, -1, 1), XMVectorSet(1, 0, 0, 1));
		for(size_t i = 0; i<4; i++)
		{
			// plane scale to (1,1,1) from (2,2,2)
			XMStoreFloat3(
				&mesh_vrts[i].position,
				XMVectorMultiply(XMLoadFloat3(&mesh_vrts[i].position), XMVectorSet(0.5f, 0.5f, 0.0f, 1.0f))
			);
		}
		mesh_vrts[0].uv = XMFLOAT2(1, 0);
		mesh_vrts[1].uv = XMFLOAT2(0, 0);
		mesh_vrts[2].uv = XMFLOAT2(0, 1);
		mesh_vrts[3].uv = XMFLOAT2(1, 1);

		desc.verts_ptr		= mesh_vrts;
		desc.verts_count	= 4;
		desc.indices_ptr	= idxs;
		desc.indices_count	= 6;
		desc.submesh_pairs_ptr= nullptr;
		desc.submesh_pairs_count = 0;

		m_mesh_plane = std::make_unique<Mesh>();
		hr = m_mesh_plane->init(m_gfx.get(), &desc);
		THROW_IF_HFAILED(hr, "Mesh creation fail.")
	}

	// CUBE
	{
		/* Axis of faces
		.    _______
		.   /      /|
		.  /  Y+  / |
		. +------+  |
		. |      |X+/
		. |  Z-  | /
		. +------+

		*/
		Vertex verts[24];
		size_t verts_cnt = sizeof(verts)/sizeof(Vertex);
		{

			/*
			.        Y+                      Y+
			. 1  +--------+ 0         5  +--------+ 4
			.    |        |              |        |
			.    |   X+   | Z+       Z+  |   X-   |
			.    |        |              |        |
			. 2  +--------+ 3         6  +--------+ 7

			.        Z+                      Z+
			. 9  +--------+ 8        13  +--------+ 12
			.    |        |              |        |
			.    |   Y+   | X+       X+  |   Y-   |
			.    |        |              |        |
			. 10 +--------+ 11       14  +--------+ 15

			.        Y+                      Y+
			. 17 +--------+ 16       21  +--------+ 20
			.    |        |              |        |
			. X+ |   Z+   |              |   Z-   | X+
			.    |        |              |        |
			. 18 +--------+ 19       22  +--------+ 23

			*/
			// X+
			vertexFillQuad(verts, verts_cnt, 0, XMVectorSet(1, 0, 0, 1), XMVectorSet(0, 0, 1, 1));
			// X-
			vertexFillQuad(verts, verts_cnt, 4, XMVectorSet(-1, 0, 0, 1), XMVectorSet(0, 0, -1, 1));
			// Y+
			vertexFillQuad(verts, verts_cnt, 8, XMVectorSet(0, 1, 0, 1), XMVectorSet(1, 0, 0, 1));
			// Y-
			vertexFillQuad(verts, verts_cnt, 12, XMVectorSet(0, -1, 0, 1), XMVectorSet(-1, 0, 0, 1));
			// Z+
			vertexFillQuad(verts, verts_cnt, 16, XMVectorSet(0, 0, 1, 1), XMVectorSet(-1, 0, 0, 1));
			// Z-
			vertexFillQuad(verts, verts_cnt, 20, XMVectorSet(0, 0, -1, 1), XMVectorSet(1, 0, 0, 1));
		}

		uint32_t indices[]=
		{
			0,1,2, 2,3,0, // Z-
			4,5,6, 6,7,4, // Z+
			8,9,10, 10,11,8, // X+
			12,13,14, 14,15,12, // X-
			16,17,18, 18,19,16, // Y+
			20,21,22, 22,23,20, // Y-
		};
		size_t indices_cnt = sizeof(indices)/sizeof(uint32_t);


		MeshInitDesc desc={};
		desc.verts_ptr		= verts;
		desc.verts_count	= verts_cnt;
		desc.indices_ptr	= indices;
		desc.indices_count	= indices_cnt;
		desc.submesh_pairs_ptr= nullptr;
		desc.submesh_pairs_count = 0;

		m_mesh_cube = std::make_unique<Mesh>();
		hr = m_mesh_cube->init(m_gfx.get(), &desc);
		THROW_IF_HFAILED(hr, "Mesh creation fail.")
	}

	// OWN MESH
	{
		Mesh* tmp=nullptr;
		if(!loadWavefrontFromFile("GameResources/Models/Suzanne_high.obj", m_gfx.get(), &tmp))
		{
			throw std::runtime_error("Wavefront load failed.");
		}
		m_mesh_file.reset(tmp);
	}

	// Sky Sphere
	{
		Mesh* tmp=nullptr;
		if(!loadWavefrontFromFile("GameResources/Models/SkySphere.obj", m_gfx.get(), &tmp))
		{
			throw std::runtime_error("Wavefront load failed.");
		}
		m_mesh_skysphere.reset(tmp);
	}

	// Load Texture
	{
		ScratchImage	image;
		TexMetadata		meta;
		hr = LoadFromWICFile(L"GameResources/Models/Suzanne_ao.png", WIC_FLAGS_NONE, &meta, image);
		THROW_IF_HFAILED(hr, "Texture load fail.")
			
		hr = CreateTextureEx(m_gfx->m_device.Get(), meta, D3D12_RESOURCE_FLAG_NONE, true, m_tex_ao.GetAddressOf());
		THROW_IF_HFAILED(hr, "Texture resource creation fail.")

		D3D12_SUBRESOURCE_DATA subres={};
		auto images			= image.GetImages();
		subres.pData		= images->pixels;
		subres.RowPitch		= images->rowPitch;
		m_gfx->updateSubresources(m_tex_ao.Get(), 0, 1, &subres);

		m_tex_ao_fmt = MakeSRGB(meta.format);
	}

	// Create white texture for default texture
	{
		D3D12_HEAP_PROPERTIES prop={};
		prop.Type					= D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask		= 1;
		prop.VisibleNodeMask		= 1;

		D3D12_RESOURCE_DESC desc ={};
		desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		desc.Alignment			= 0;
		desc.Width				= 1;
		desc.Height				= 1;
		desc.DepthOrArraySize	= 1;
		desc.MipLevels			= 1;
		desc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count	= 1;
		desc.SampleDesc.Quality	= 0;
		desc.Flags				= D3D12_RESOURCE_FLAG_NONE;
		desc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;

		// create buffer
		hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_tex_default_white.GetAddressOf())
		);
		THROW_IF_HFAILED(hr, "Default texture creation fail.")

		uint32_t rgba = 0xffffffff;
		D3D12_SUBRESOURCE_DATA subres={};
		subres.pData = &rgba;

		m_gfx->updateSubresources(m_tex_default_white.Get(), 0, 0, &subres);
	}

	// Load Cube Texture
	{
		ScratchImage	image;
		TexMetadata		meta;
		hr = LoadFromDDSFile(L"GameResources/Textures/cubemap.dds", DDS_FLAGS_NONE, &meta, image);
		THROW_IF_HFAILED(hr, "Texture load fail.")

		hr = CreateTextureEx(m_gfx->m_device.Get(), meta, D3D12_RESOURCE_FLAG_NONE, false, m_tex_env.GetAddressOf());
		THROW_IF_HFAILED(hr, "Texture resource creation fail.")

		if(meta.IsCubemap())
		{
			std::unique_ptr<D3D12_SUBRESOURCE_DATA[]> subres;

			subres.reset(new D3D12_SUBRESOURCE_DATA[image.GetImageCount()]);
			for(size_t i = 0; i<image.GetImageCount(); i++)
			{
				auto img = image.GetImage(0, i, 0);

				subres[i].pData			= img->pixels;
				subres[i].RowPitch		= img->rowPitch;
				subres[i].SlicePitch	= img->slicePitch;
			}

			m_gfx->updateSubresources(m_tex_env.Get(), 0, image.GetImageCount(), subres.get());
			m_tex_env_fmt = meta.format;
		}
	}

	// Create Shadow texture
	{
		ComPtr<ID3D12Resource>	sm;

		D3D12_HEAP_PROPERTIES prop={};
		prop.Type					= D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask		= 1;
		prop.VisibleNodeMask		= 1;

		D3D12_RESOURCE_DESC desc ={};
		desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment			= 0;
		desc.Width				= m_shadowmap_resolution.x;
		desc.Height				= m_shadowmap_resolution.x;
		desc.DepthOrArraySize	= 1;
		desc.MipLevels			= 1;
		desc.Format				= DXGI_FORMAT_R24G8_TYPELESS;
		desc.SampleDesc.Count	= 1;
		desc.SampleDesc.Quality	= 0;
		desc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		desc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_CLEAR_VALUE cval ={};
		cval.Format					= DXGI_FORMAT_D24_UNORM_S8_UINT;
		cval.DepthStencil.Depth		= 1.f;
		cval.DepthStencil.Stencil	= 0;

		// create buffer
		hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&cval,
			IID_PPV_ARGS(sm.GetAddressOf())
		);
		THROW_IF_HFAILED(hr, "Shadow buffer creation fail.")

		m_shadowmap_fmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;


		{
			RenderTargetDesc rt_desc={};
			rt_desc.backbuffer_count			= 1;
			rt_desc.target_count				= 0;
			rt_desc.targets_each_backbuffer_ptr	= nullptr;
			rt_desc.rendertarget_view_descs		= nullptr;
			rt_desc.depthstencil_buffer_ptr		= sm.Get();
			
			auto& dsd = rt_desc.depthstencil_view_desc;
			ZeroMemory(&dsd, sizeof(dsd));
			dsd.Format			= DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsd.ViewDimension	= D3D12_DSV_DIMENSION_TEXTURE2D;
			dsd.Flags			= D3D12_DSV_FLAG_NONE;

			m_shadowmap.reset(new RenderTarget());
			m_shadowmap->init(m_gfx.get(), &rt_desc);
		}
	}

	// Create instancing buffer
	{
		D3D12_HEAP_PROPERTIES	prop = {};
		prop.Type					= D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask		= 1;
		prop.VisibleNodeMask		= 1;

		D3D12_RESOURCE_DESC desc ={};
		desc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment			= 0;
		desc.Width				= sizeof(InstancingData) * m_config.instancing_count;
		desc.Height				= 1;
		desc.DepthOrArraySize	= 1;
		desc.MipLevels			= 1;
		desc.Format				= DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count	= 1;
		desc.SampleDesc.Quality	= 0;
		desc.Flags				= D3D12_RESOURCE_FLAG_NONE;
		desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_instancing_buffer.GetAddressOf())
		);
		THROW_IF_HFAILED(hr, "Instancing buffer creation fail.")

		m_vertex_views[1].BufferLocation	= m_instancing_buffer->GetGPUVirtualAddress();
		m_vertex_views[1].StrideInBytes		= sizeof(InstancingData);
		m_vertex_views[1].SizeInBytes		= sizeof(InstancingData) * m_config.instancing_count;
		hr = m_instancing_buffer->Map(0, nullptr, (void**)&m_instancing_ptr);
	}

	// Create PSO
	{
		D3D12_DESCRIPTOR_RANGE dr[1];
		dr[0].RangeType				= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		dr[0].NumDescriptors		= 3;
		dr[0].BaseShaderRegister	= 0;
		dr[0].RegisterSpace			= 0;
		dr[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rs_param[2];
		rs_param[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rs_param[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;
		rs_param[0].Constants.Num32BitValues	= sizeof(ShaderUniforms)/4;
		rs_param[0].Constants.RegisterSpace		= 0;
		rs_param[0].Constants.ShaderRegister	= 0;

		rs_param[1].ParameterType		= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rs_param[1].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;
		rs_param[1].DescriptorTable.NumDescriptorRanges = 1;
		rs_param[1].DescriptorTable.pDescriptorRanges	= dr;

		D3D12_STATIC_SAMPLER_DESC	sampler[2];
		// Texture
		sampler[0].Filter			= D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sampler[0].AddressU			= sampler[0].AddressV = sampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler[0].MipLODBias		= 0;
		sampler[0].MaxAnisotropy	= 0;
		sampler[0].ComparisonFunc	= D3D12_COMPARISON_FUNC_NEVER;
		sampler[0].BorderColor		= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler[0].MinLOD			= 0.0f;
		sampler[0].MaxLOD			= D3D12_FLOAT32_MAX;
		sampler[0].ShaderRegister	= 0;
		sampler[0].RegisterSpace	= 0;
		sampler[0].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;
		// Shadow map
		sampler[1].Filter			= D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sampler[1].AddressU			= sampler[1].AddressV = sampler[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler[1].MipLODBias		= 0;
		sampler[1].MaxAnisotropy	= 1;
		sampler[1].ComparisonFunc	= D3D12_COMPARISON_FUNC_LESS_EQUAL;
		sampler[1].BorderColor		= D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		sampler[1].MinLOD			= 0.0f;
		sampler[1].MaxLOD			= D3D12_FLOAT32_MAX;
		sampler[1].ShaderRegister	= 1;
		sampler[1].RegisterSpace	= 0;
		sampler[1].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		
		D3D12_ROOT_SIGNATURE_DESC	rs_desc={};
		rs_desc.NumParameters		= 2;
		rs_desc.pParameters			= rs_param;
		rs_desc.NumStaticSamplers	= 2;
		rs_desc.pStaticSamplers		= sampler;
		rs_desc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		;


		ComPtr<ID3DBlob>	vs_blob;
		ComPtr<ID3DBlob>	ps_blob;

		hr = D3DReadFileToBlob(L"GameResources/shaders/test_vs.cso", vs_blob.GetAddressOf());
		THROW_IF_HFAILED(hr, "Can not open the vertex shader file.")

		hr = D3DReadFileToBlob(L"GameResources/shaders/test_ps.cso", ps_blob.GetAddressOf());
		THROW_IF_HFAILED(hr, "Can not open the pixel shader file.")


		// rasterizer
		D3D12_RASTERIZER_DESC	raster;
		raster.FillMode					= D3D12_FILL_MODE_SOLID;
		raster.CullMode					= D3D12_CULL_MODE_BACK;
		raster.FrontCounterClockwise	= TRUE;
		raster.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
		raster.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		raster.SlopeScaledDepthBias		= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		raster.DepthClipEnable			= TRUE;
		raster.MultisampleEnable		= FALSE;
		raster.AntialiasedLineEnable	= FALSE;
		raster.ForcedSampleCount		= 0;
		raster.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// blending each target
		D3D12_RENDER_TARGET_BLEND_DESC tag_blend={
			FALSE, FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL
		};

		// blending
		D3D12_BLEND_DESC blend;
		blend.AlphaToCoverageEnable		= FALSE;
		blend.IndependentBlendEnable	= FALSE;
		for(auto& it : blend.RenderTarget)
		{
			it = tag_blend;
		}

		// PIPELINE STATE
		D3D12_GRAPHICS_PIPELINE_STATE_DESC	ps_desc ={};
		ZeroMemory(&ps_desc, sizeof(ps_desc));
		ps_desc.InputLayout						={VERTEX_ELEMENTS, VERTEX_ELEMENTS_COUNT};
		ps_desc.pRootSignature					= nullptr;
		ps_desc.VS								={reinterpret_cast<UINT8*>(vs_blob->GetBufferPointer()), vs_blob->GetBufferSize()};
		ps_desc.PS								={reinterpret_cast<UINT8*>(ps_blob->GetBufferPointer()), ps_blob->GetBufferSize()};
		ps_desc.RasterizerState					= raster;
		ps_desc.BlendState						= blend;
		ps_desc.DepthStencilState.DepthEnable	= TRUE;
		ps_desc.DepthStencilState.StencilEnable	= FALSE;
		ps_desc.DepthStencilState.DepthFunc		= D3D12_COMPARISON_FUNC_LESS;
		ps_desc.DepthStencilState.DepthWriteMask= D3D12_DEPTH_WRITE_MASK_ALL;
		ps_desc.SampleMask						= UINT_MAX;
		ps_desc.PrimitiveTopologyType			= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		ps_desc.NumRenderTargets				= 1;
		ps_desc.RTVFormats[0]					= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		ps_desc.DSVFormat						= DXGI_FORMAT_D24_UNORM_S8_UINT;
		ps_desc.SampleDesc.Count				= 1;
		
		// Create Forward PSO
		m_forward_pso.reset(new PipelineState());
		m_forward_pso->init(m_gfx.get(), &rs_desc, &ps_desc);

		// Shadow PSO
		ps_blob.Reset();
		hr = D3DReadFileToBlob(L"GameResources/shaders/Test_shadow_vs.cso", ps_blob.GetAddressOf());
		THROW_IF_HFAILED(hr, "Can not open the vertex shader file.")

		ps_blob.Reset();
		hr = D3DReadFileToBlob(L"GameResources/shaders/Test_shadow_ps.cso", ps_blob.GetAddressOf());
		THROW_IF_HFAILED(hr, "Can not open the pixel shader file.")

		ps_desc.VS	={reinterpret_cast<UINT8*>(vs_blob->GetBufferPointer()), vs_blob->GetBufferSize()};
		ps_desc.PS ={reinterpret_cast<UINT8*>(ps_blob->GetBufferPointer()), ps_blob->GetBufferSize()};
		ps_desc.NumRenderTargets				= 1;
		ps_desc.RTVFormats[0]					= DXGI_FORMAT_R8G8B8A8_UNORM;
		m_foward_shadow_caster.reset(new PipelineState());
		m_foward_shadow_caster->init(m_gfx.get(), &rs_desc, &ps_desc);


		vs_blob.Reset();
		ps_blob.Reset();

		hr = D3DReadFileToBlob(L"GameResources/shaders/SkyboxSimple_vs.cso", vs_blob.GetAddressOf());
		THROW_IF_HFAILED(hr, "Can not open the vertex shader file.")

		hr = D3DReadFileToBlob(L"GameResources/shaders/SkyboxSimple_ps.cso", ps_blob.GetAddressOf());
		THROW_IF_HFAILED(hr, "Can not open the pixel shader file.")

		ps_desc.RasterizerState.CullMode			= D3D12_CULL_MODE_NONE;
		ps_desc.DepthStencilState.DepthEnable		= TRUE;
		ps_desc.DepthStencilState.StencilEnable		= FALSE;
		ps_desc.DepthStencilState.DepthFunc			= D3D12_COMPARISON_FUNC_ALWAYS;
		ps_desc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
		ps_desc.NumRenderTargets				= 1;
		ps_desc.RTVFormats[0]					= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		ps_desc.VS	={reinterpret_cast<UINT8*>(vs_blob->GetBufferPointer()), vs_blob->GetBufferSize()};
		ps_desc.PS	={reinterpret_cast<UINT8*>(ps_blob->GetBufferPointer()), ps_blob->GetBufferSize()};

		m_pso_skybox.reset(new PipelineState());
		m_pso_skybox->init(m_gfx.get(), &rs_desc, &ps_desc);
	}

	// Create the SRV
	{
		auto handle = m_gfx_context->m_srv_heap->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC desc={};
		desc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format						= m_tex_ao_fmt;
		desc.Texture2D.MipLevels		= 1;
		desc.Texture2D.MostDetailedMip	= 0;
		// Create the SRV
		m_gfx->m_device->CreateShaderResourceView(m_tex_ao.Get(), &desc, handle);

		handle.ptr += m_gfx->m_SRV_INC;
		desc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURECUBE;
		desc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format							= m_tex_env_fmt;
		desc.TextureCube.MipLevels			= 1;
		desc.TextureCube.MostDetailedMip	= 0;
		// Create the SRV
		m_gfx->m_device->CreateShaderResourceView(m_tex_env.Get(), &desc, handle);

		handle.ptr += m_gfx->m_SRV_INC;
		desc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format							= m_shadowmap_fmt;
		desc.TextureCube.MipLevels			= 1;
		desc.TextureCube.MostDetailedMip	= 0;
		// Create the SRV
		m_gfx->m_device->CreateShaderResourceView(m_shadowmap->m_ds_buffer.Get(), &desc, handle);

	}
}

void TestEngine::shutdown()
{
	m_gfx->waitForDone();

	m_instancing_buffer->Unmap(0, nullptr);
	m_imgui->shutdown();

	m_gfx_context->setRenderTarget(nullptr);
	m_swapchain.reset();
}

void TestEngine::update()
{
	{
		auto move	= XMVectorSwizzle<0, 3, 1, 3>(m_input->getLStickVector(0));
		auto updown	= m_input->getTriggerVector(0);
		auto torque	= XMVectorSwizzle<1, 0, 2, 3>(XMVectorMultiply(m_input->getRStickVector(0), XMVectorSet(1, -1, 0, 0)));
		auto rot	= XMLoadFloat3(&m_cam_rot);

		updown = XMVectorSet(0, XMVectorGetY(updown)-XMVectorGetX(updown), 0, 0);

		move = XMVectorMultiply(XMVectorSet(1, 0, 1, 1),XMVectorAdd(
			XMVector3Rotate(move, XMQuaternionRotationRollPitchYawFromVector(rot)),
			XMVector3Rotate(move, XMQuaternionRotationRollPitchYawFromVector(rot))
		));
		move = XMVectorAdd(move, updown);

		XMStoreFloat3(
			&m_cam_pos,
			XMVectorAdd(XMLoadFloat3(&m_cam_pos), XMVectorScale(move, m_time_delta*5.f))
		);
		XMStoreFloat3(
			&m_cam_rot,
			XMVectorAdd(rot, XMVectorScale(torque, m_time_delta*2.f))
		);

		//m_phase = (float)m_xinput_state.Gamepad.bRightTrigger/(float)MAXBYTE;
	}
	

	// debug window
	
	m_imgui->newFrame();
	{
		if( m_demo_window )
			ImGui::ShowDemoWindow(&m_demo_window);

		ImGui::Begin("Debug");
		{
			ImGui::Text("FPS : %f", 1.0f/m_time_delta);
			ImGui::Checkbox("Show Demo Window", &m_demo_window);
			ImGui::SliderFloat("Phase", &m_phase, 0.f, 1.f);
			
			ImGui::SliderFloat3("Sun Rotation", reinterpret_cast<float*>(&m_light_rot), -3.14159, 3.14159);
			
			ImGui::Text("Camera");
			ImGui::InputFloat3("Position", reinterpret_cast<float*>(&m_cam_pos));
			ImGui::InputFloat3("Rotation", reinterpret_cast<float*>(&m_cam_rot));
			ImGui::SliderFloat("Fov", &m_cam_fov, 0.1f, 179.f);
			ImGui::SliderFloat("AO", &m_ao, 0.0f, 1.0f);
		}
		ImGui::End();
	}

	// debug drawer
	{
		XMFLOAT3 lp[]={
			{-5,0,0},
			{5,0,0},
			{0,-5,0},
			{0,5,0},
			{0,0,-5},
			{0,0,5},
		};
		
		m_debug_drawer->drawLines(&lp[0], 2, XMColorRGBToSRGB(Colors::Red), XMColorRGBToSRGB(Colors::Red));
		m_debug_drawer->drawLines(&lp[2], 2, XMColorRGBToSRGB(Colors::Green), XMColorRGBToSRGB(Colors::Green));
		m_debug_drawer->drawLines(&lp[4], 2, XMColorRGBToSRGB(Colors::Blue), XMColorRGBToSRGB(Colors::Blue));
		m_debug_drawer->drawWireCube(XMVectorNegate(g_XMOne), g_XMTwo, g_XMZero, XMColorRGBToSRGB(Colors::Gray));
		
		
		/*
		m_debug_drawer->drawTexture(
			m_shadowmap.Get(), m_shadowmap_fmt,
			XMVectorSet(0, 0, 128, 128), 0.1f
		);*/
	}
}

void TestEngine::setRTVCurrent()
{
	m_gfx_context->setRenderTarget( m_swapchain->m_window_rtv.get() );

	D3D12_VIEWPORT vp;
	vp.Width	= m_config.width;
	vp.Height	= m_config.height;
	vp.TopLeftX = vp.TopLeftY = 0;
	vp.MinDepth = 0.f;
	vp.MaxDepth	= 1.f;
	m_gfx_context->setViewports(1, &vp);

	D3D12_RECT sr;
	sr.left		= sr.top = 0;
	sr.right	= m_config.width;
	sr.bottom	= m_config.height;
	m_gfx_context->setScissorRects(1, &sr);
}

void TestEngine::drawObjects(ID3D12GraphicsCommandList* cmd_list)
{
	{
		auto p_mesh = m_mesh_file.get();

		m_vertex_views[0] = p_mesh->m_vb_view;
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_list->IASetVertexBuffers(0, m_vertex_views.size(), m_vertex_views.data());
		cmd_list->IASetIndexBuffer(&p_mesh->m_ib_view);

		cmd_list->DrawIndexedInstanced(p_mesh->m_submesh_pairs[0].count, m_config.instancing_count-1, p_mesh->m_submesh_pairs[0].offset, 0, 0);
	}
	{
		auto p_mesh = m_mesh_plane.get();

		m_vertex_views[0] = p_mesh->m_vb_view;
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd_list->IASetVertexBuffers(0, m_vertex_views.size(), m_vertex_views.data());
		cmd_list->IASetIndexBuffer(&p_mesh->m_ib_view);

		cmd_list->DrawIndexedInstanced(p_mesh->m_submesh_pairs[0].count, 1, p_mesh->m_submesh_pairs[0].offset, 0, m_config.instancing_count-1);
	}
}

void TestEngine::render()
{
	auto edge = floorf(sqrtf(m_config.instancing_count));
	for(uint32_t i=0; i<m_config.instancing_count-1; i++)
	{
		float	r		= (float)i/(float)m_config.instancing_count * 20.f;
		float	idx		= (float)i*10.f;
		auto	freq	= [&](float m){return (idx+m_time_counted)*m; };
		float	tan_f	= tanf(idx);

		auto pos = XMVectorLerp(
			XMVectorSet(
				cosf(freq(0.1f)*tan_f)*r,
				sinf(freq(sinf((float)idx)*0.1f)*tan_f)*r,
				-sinf(freq(0.1f)*tan_f)*r,
				1.f
			),
			XMVectorSet(
				3.0f * fmodf(i, edge),
				1.f,
				3.0f * (float)(i/(UINT32)edge),
				1.f
			),
			m_phase
		);
		auto rot = XMVectorLerp(XMQuaternionRotationRollPitchYawFromVector(pos), XMQuaternionIdentity(), m_phase);



		auto it = &((InstancingData*)m_instancing_ptr)[i];
		XMStoreFloat4x4(
			&it->world,
			XMMatrixTransformation(XMVectorZero(), XMQuaternionIdentity(), g_XMOne, XMVectorZero(), rot, pos)
		);
		XMStoreFloat4(
			&it->color,
			XMColorSRGBToRGB(XMColorHSVToRGB(XMVectorSet(fmodf(idx*0.01f,1.f), 0.7f, 1.0f, 1.0f)))
		);
		XMStoreFloat4(
			&it->pbs_param,
			XMVectorSet(sinf(idx)*0.5f+0.5f, sinf(idx+24.6f)*0.5f+0.5f, 0.0f, 0.0f)
		);
	}
	// draw plane
	{
		auto it = &((InstancingData*)m_instancing_ptr)[m_config.instancing_count-1];

		XMStoreFloat4x4(
			&it->world,
			XMMatrixTransformation(XMVectorZero(), XMQuaternionIdentity(), XMVectorSet(100, 100, 1, 1), XMVectorZero(), XMQuaternionRotationAxis(XMVectorSet(1,0,0,0),XMConvertToRadians(90.f)), XMVectorZero())
		);
		XMStoreFloat4(
			&it->color,
			XMColorSRGBToRGB(XMColorHSVToRGB(XMVectorSet(1.0f, 0.0f, 1.0f,1.0f)))
		);
		XMStoreFloat4(
			&it->pbs_param,
			XMVectorSet(0.1f, 0.1f, 0.0f, 0.0f)
		);
	}


	// set shader uniforms
	ShaderUniforms su;
	DirectX::XMFLOAT4X4 camera_view_mat;
	{
		su.camera_position	= m_cam_pos;
		su.ao				= m_ao;
		XMStoreFloat3(
			&su.lightdir,
			XMVector3Rotate(
				VECTOR_UP,
				XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_light_rot))
			)
		);
	}


	auto cmd_queue	= m_gfx->m_cmd_queue.Get();
	auto cmd_alloc	= m_gfx_context->m_cmd_alloc.Get();
	auto cmd_list	= m_gfx_context->m_cmd_list.Get();
	ID3D12CommandList* cmd_lists[] ={cmd_list};


	
	// Shadow maps
	{
		float scale = 50.f;
		auto origin	= XMLoadFloat3(&m_cam_pos);
		auto rot	= XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_light_rot));
		auto view	= XMMatrixLookAtLH(
			XMVectorAdd(origin,XMVectorScale(XMVector3Rotate(VECTOR_UP,rot),scale*0.5f)),
			origin,
			XMVector3Rotate(VECTOR_FORWARD,rot)
		);
		auto proj	= XMMatrixOrthographicLH(scale,scale,0,scale);

		XMStoreFloat4x4(&su.view, view);
		XMStoreFloat4x4(&su.projection, proj);
		XMStoreFloat4x4(&su.shadow_mat, XMMatrixMultiply(view,proj));
	}
	if(true){
		m_gfx_context->setRenderTarget(m_shadowmap.get());

		D3D12_VIEWPORT vp;
		vp.Width	= (float)m_shadowmap_resolution.x;
		vp.Height	= (float)m_shadowmap_resolution.y;
		vp.TopLeftX = vp.TopLeftY = 0;
		vp.MinDepth = 0.f;
		vp.MaxDepth	= 1.f;
		m_gfx_context->setViewports(1, &vp);

		D3D12_RECT sr;
		sr.left		= sr.top = 0;
		sr.right	= m_shadowmap_resolution.x;
		sr.bottom	= m_shadowmap_resolution.y;
		m_gfx_context->setScissorRects(1, &sr);
	}
	m_gfx_context->begin();
	do{
		// set pso
		m_gfx_context->setPipelineState(m_foward_shadow_caster.get());
		cmd_list->SetDescriptorHeaps(1, m_gfx_context->m_srv_heap.GetAddressOf());

		// set shader uniforms
		cmd_list->SetGraphicsRoot32BitConstants(0, sizeof(ShaderUniforms)/4, &su, 0);

		m_gfx_context->clearDepthStencil(D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
		drawObjects(cmd_list);
	}
	while(false);
	m_gfx_context->end(true);
	cmd_queue->ExecuteCommandLists(1, cmd_lists);
	m_gfx->waitForDone();
	
	
	
	setRTVCurrent();

	
	// Draw Skybox
	{
		auto rot		= XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_cam_rot));
		auto look_fwd	= XMVector3Rotate(VECTOR_FORWARD, rot);
		auto look_up	= XMVector3Rotate(VECTOR_UP, rot);
		auto aspect		= (float)m_config.width/(float)m_config.height;
		auto fov		= XMConvertToRadians(m_cam_fov);

		auto view = XMMatrixLookToLH(XMLoadFloat3(&m_cam_pos), look_fwd, look_up);
		auto proj = XMMatrixPerspectiveFovLH(fov, aspect, 0.1f, 100.f);

		XMStoreFloat4x4(&camera_view_mat, view);
		m_debug_drawer->setVPMatrix(view, proj);

		XMStoreFloat4x4(&su.view, XMMatrixLookToLH(g_XMZero, look_fwd, look_up));
		XMStoreFloat4x4(&su.projection, proj);
	}
	
	m_gfx_context->begin();
	{
		// set pso
		m_gfx_context->setPipelineState(m_pso_skybox.get());
		cmd_list->SetDescriptorHeaps(1, m_gfx_context->m_srv_heap.GetAddressOf());

		cmd_list->SetGraphicsRoot32BitConstants(0, sizeof(XMFLOAT4X4)*2/4, &su.view, 0);
		cmd_list->SetGraphicsRootDescriptorTable(1, m_gfx_context->m_srv_heap->GetGPUDescriptorHandleForHeapStart());

		const float cval[] ={0.5f,0.5f,0.5f,1.f};
		m_gfx_context->clearRenderTarget(0, cval, 0, nullptr);
		m_gfx_context->clearDepthStencil(D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
		{
			auto p_mesh = m_mesh_skysphere.get();

			m_vertex_views[0] = p_mesh->m_vb_view;
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd_list->IASetVertexBuffers(0, m_vertex_views.size(), m_vertex_views.data());
			cmd_list->IASetIndexBuffer(&p_mesh->m_ib_view);

			cmd_list->DrawIndexedInstanced(p_mesh->m_submesh_pairs[0].count, 1, p_mesh->m_submesh_pairs[0].offset, 0, 0);
		}
	}
	m_gfx_context->end(false);
	cmd_queue->ExecuteCommandLists(1, cmd_lists);
	
	m_gfx->waitForDone();


	// Foward Objects
	{
		su.view = camera_view_mat;
	}
	m_gfx_context->begin();
	{
		// set pso
		m_gfx_context->setPipelineState(m_forward_pso.get());
		cmd_list->SetDescriptorHeaps(1, m_gfx_context->m_srv_heap.GetAddressOf());

		// set shader uniforms
		cmd_list->SetGraphicsRoot32BitConstants(0, sizeof(ShaderUniforms)/4, &su, 0);
		cmd_list->SetGraphicsRootDescriptorTable(1, m_gfx_context->m_srv_heap->GetGPUDescriptorHandleForHeapStart());

		drawObjects(cmd_list);
	}
	m_gfx_context->end(false);
	cmd_queue->ExecuteCommandLists(1, cmd_lists);
	m_gfx->waitForDone();
	
	
	// Render DebugDrawer
	m_debug_drawer->render(m_gfx.get(), m_gfx_context.get());
	m_debug_drawer->flush();
	
	// render imgui
	
	m_gfx_context->begin();
	{
		m_imgui->render(m_gfx_context.get());
	}
	m_gfx_context->end(true);
	

	cmd_queue->ExecuteCommandLists(1, cmd_lists);
	m_gfx->waitForDone();


	m_swapchain->present();
}