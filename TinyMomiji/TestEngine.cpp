#include "TestEngine.hpp"
#include <iostream>
#include <thread>
#include <stdexcept>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"xinput.lib")



namespace{
	using Microsoft::WRL::ComPtr;
	using namespace DirectX;
	using namespace DirectX::PackedVector;

	const TCHAR* WNDCLASS_NAME = TEXT("TinyMomiji");
}


struct InstancingData
{
	XMFLOAT4X4	world;
	XMFLOAT4	color;
};
struct ShaderUniforms
{
	XMFLOAT4X4	view;
	XMFLOAT4X4	projection;
	XMFLOAT3	camera_position;
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
};
const size_t VERTEX_ELEMENTS_COUNT = sizeof(VERTEX_ELEMENTS)/sizeof(D3D12_INPUT_ELEMENT_DESC);



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
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

void TestEngine::setRTVCurrent()
{
	m_frame_index = m_swapchain->GetCurrentBackBufferIndex();

	// setup rtv
	D3D12_RENDER_TARGET_VIEW_DESC rdesc;
	rdesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rdesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
	rdesc.Texture2D.MipSlice	= 0;
	rdesc.Texture2D.PlaneSlice	= 0;

	auto handle = m_rt_heap->GetCPUDescriptorHandleForHeapStart();
	m_graphics.m_device->CreateRenderTargetView(m_sc_buffers[m_frame_index].Get(), &rdesc, handle);
}

void TestEngine::initGraphics()
{
	HRESULT hr;

	m_graphics.init();
	auto device		= m_graphics.m_device.Get();
	auto factory	= m_graphics.m_dxgi_factory.Get();

	// Create Swapchain
	{
		DXGI_SWAP_CHAIN_DESC	desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.BufferCount		= 2;
		desc.BufferDesc.Width	= m_config.width;
		desc.BufferDesc.Height	= m_config.height;
		desc.BufferDesc.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.OutputWindow		= m_hwnd;
		desc.SampleDesc.Count	= 1;
		desc.Windowed			= TRUE;
		desc.BufferDesc.RefreshRate.Numerator	= 1;
		desc.BufferDesc.RefreshRate.Denominator	= 60;

		ComPtr<IDXGISwapChain>	sc;
		hr = factory->CreateSwapChain(m_graphics.m_cmd_queue.Get(), &desc, sc.GetAddressOf());
		THROW_IF_HFAIL(hr, "CreateSwapChain() Fail.")

		// cast to 3
		hr = sc->QueryInterface(m_swapchain.GetAddressOf());
		THROW_IF_HFAIL(hr, "QueryInterface() Fail.")

		m_frame_index =  m_swapchain->GetCurrentBackBufferIndex();
	}

	// Create RTV
	{
		D3D12_DESCRIPTOR_HEAP_DESC hdesc;
		ZeroMemory(&hdesc, sizeof(hdesc));
		hdesc.NumDescriptors	= 1;
		hdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		hdesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		// Create RTV
		hr = device->CreateDescriptorHeap(&hdesc, IID_PPV_ARGS(m_rt_heap.GetAddressOf()));
		THROW_IF_HFAIL(hr, "RTV heap creation fail.")

		m_sc_buffers.reserve(2);
		for(size_t i=0; i<2; i++)
		{
			m_sc_buffers.emplace_back();
			m_swapchain->GetBuffer(i, IID_PPV_ARGS(m_sc_buffers.back().GetAddressOf()));
			THROW_IF_HFAIL(hr, "GetBuffer() Fail.")
		}
		//setRTVCurrent();

		// Create DSV
		hdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		hr = device->CreateDescriptorHeap(&hdesc, IID_PPV_ARGS(m_ds_heap.GetAddressOf()));
		THROW_IF_HFAIL(hr, "RTV heap creation fail.")


		// Create DS Buffer
		{
			D3D12_HEAP_PROPERTIES prop={};
			prop.Type					= D3D12_HEAP_TYPE_DEFAULT;
			prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			prop.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
			prop.CreationNodeMask		= 1;
			prop.VisibleNodeMask		= 1;

			D3D12_RESOURCE_DESC desc ={};
			desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Alignment			= 0;
			desc.Width				= m_config.width;
			desc.Height				= m_config.height;
			desc.DepthOrArraySize	= 1;
			desc.MipLevels			= 0;
			desc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
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
				IID_PPV_ARGS(m_ds_buffer.GetAddressOf())
			);
			THROW_IF_HFAIL(hr, "DS buffer creation fail.")


			D3D12_DEPTH_STENCIL_VIEW_DESC dsd;
			ZeroMemory(&dsd, sizeof(dsd));
			dsd.Format			= DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsd.ViewDimension	= D3D12_DSV_DIMENSION_TEXTURE2D;
			dsd.Flags			= D3D12_DSV_FLAG_NONE;

			// create view
			device->CreateDepthStencilView(
				m_ds_buffer.Get(),
				&dsd,
				m_ds_heap->GetCPUDescriptorHandleForHeapStart()
			);
		}
	}
	
}

void TestEngine::initResources()
{
	HRESULT hr;
	auto device = m_graphics.m_device.Get();

	/* Axis of faces
	   _______
	  /      /|
	 /  Y+  / |
	+------+  |
	|      |X+/
	|  Z-  | /
	+------+

	*/
	Vertex verts[24];
	size_t verts_cnt = sizeof(verts)/sizeof(Vertex);
	{

		/*
		       Y+                      Y+
		1  +--------+ 0         5  +--------+ 4
		   |        |              |        |
		   |   X+   | Z+       Z+  |   X-   |
		   |        |              |        |
		2  +--------+ 3         6  +--------+ 7

		       Z+                      Z+
		9  +--------+ 8        13  +--------+ 12
		   |        |              |        |
		   |   Y+   | X+       X+  |   Y-   |
		   |        |              |        |
		10 +--------+ 11       14  +--------+ 15

		       Y+                      Y+
		17 +--------+ 16       21  +--------+ 20
		   |        |              |        |
		X+ |   Z+   |              |   Z-   | X+
		   |        |              |        |
		18 +--------+ 19       22  +--------+ 23

		*/
		// X+
		vertexFillQuad(verts, verts_cnt, 0,	XMVectorSet(1,0,0,1),	XMVectorSet(0,0,1,1));
		// X-
		vertexFillQuad(verts, verts_cnt, 4,	XMVectorSet(-1,0,0,1),	XMVectorSet(0,0,-1,1));
		// Y+
		vertexFillQuad(verts, verts_cnt, 8,	XMVectorSet(0,1,0,1),	XMVectorSet(1,0,0,1));
		// Y-
		vertexFillQuad(verts, verts_cnt, 12,XMVectorSet(0,-1,0,1),	XMVectorSet(-1,0,0,1));
		// Z+
		vertexFillQuad(verts, verts_cnt, 16,XMVectorSet(0,0,1,1),	XMVectorSet(-1,0,0,1));
		// Z-
		vertexFillQuad(verts, verts_cnt, 20,XMVectorSet(0,0,-1,1),	XMVectorSet(1,0,0,1));
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


	// Create vertex buffer, index buffer and instacing buffer
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
		desc.Width				= sizeof(verts);
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
			IID_PPV_ARGS(m_vertex_buffer.GetAddressOf())
		);
		THROW_IF_HFAIL(hr, "Vertex buffer creation fail.")


		desc.Width = sizeof(indices);
		hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_index_buffer.GetAddressOf())
		);
		THROW_IF_HFAIL(hr, "Index buffer creation fail.")


		desc.Width = sizeof(InstancingData) * m_config.instacing_count;
		hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_instacing_buffer.GetAddressOf())
		);
		THROW_IF_HFAIL(hr, "Instacing buffer creation fail.")


		uint8_t* mapped;
		
		hr = m_vertex_buffer->Map(0, nullptr, (void**)&mapped);
		{
			memcpy(mapped, verts, sizeof(verts));
			m_vertex_views[0].BufferLocation	= m_vertex_buffer->GetGPUVirtualAddress();
			m_vertex_views[0].StrideInBytes		= sizeof(Vertex);
			m_vertex_views[0].SizeInBytes		= sizeof(verts);
		}
		m_vertex_buffer->Unmap(0, nullptr);

		hr = m_index_buffer->Map(0, nullptr, (void**)&mapped);
		{
			memcpy(mapped, indices, sizeof(indices));
			m_index_view.BufferLocation		= m_index_buffer->GetGPUVirtualAddress();
			m_index_view.SizeInBytes		= sizeof(indices);
			m_index_view.Format				= DXGI_FORMAT_R32_UINT;
			m_index_count					= indices_cnt;
		}
		m_index_buffer->Unmap(0, nullptr);

		m_vertex_views[1].BufferLocation	= m_instacing_buffer->GetGPUVirtualAddress();
		m_vertex_views[1].StrideInBytes		= sizeof(InstancingData);
		m_vertex_views[1].SizeInBytes		= sizeof(InstancingData) * m_config.instacing_count;
		hr = m_instacing_buffer->Map(0, nullptr, (void**)&m_instacing_ptr);
	}

	// create root signature
	{
		D3D12_ROOT_PARAMETER param;
		param.ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		param.ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;
		param.Constants.Num32BitValues	= sizeof(ShaderUniforms)/4;
		param.Constants.RegisterSpace	= 0;
		param.Constants.ShaderRegister	= 0;

		D3D12_ROOT_SIGNATURE_DESC	desc={};
		desc.NumParameters		= 1;
		desc.pParameters		= &param;
		desc.NumStaticSamplers	= 0;
		desc.pStaticSamplers	= nullptr;
		desc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		;

		ComPtr<ID3DBlob>	signature;
		ComPtr<ID3DBlob>	error;
		hr = D3D12SerializeRootSignature(
			&desc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			signature.GetAddressOf(),
			error.GetAddressOf()
		);
		THROW_IF_HFAIL(hr, "root signature serialization fail.")


		hr = device->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(m_root_signature.GetAddressOf())
		);
		THROW_IF_HFAIL(hr,"root signature creation fail.")
	}

	// create pipeline state
	{
		ComPtr<ID3DBlob>	vs_blob;
		ComPtr<ID3DBlob>	ps_blob;

		hr = D3DReadFileToBlob(L"shaders/test_vs.cso", vs_blob.GetAddressOf());
		THROW_IF_HFAIL(hr, "Can not open the vertex shader file.")

		hr = D3DReadFileToBlob(L"shaders/test_ps.cso", ps_blob.GetAddressOf());
		THROW_IF_HFAIL(hr, "Can not open the pixel shader file.")


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
		D3D12_GRAPHICS_PIPELINE_STATE_DESC	desc = {};
		ZeroMemory(&desc, sizeof(desc));
		desc.InputLayout					= {VERTEX_ELEMENTS, VERTEX_ELEMENTS_COUNT};
		desc.pRootSignature					= m_root_signature.Get();
		desc.VS								= {reinterpret_cast<UINT8*>(vs_blob->GetBufferPointer()), vs_blob->GetBufferSize()};
		desc.PS								= {reinterpret_cast<UINT8*>(ps_blob->GetBufferPointer()), ps_blob->GetBufferSize()};
		desc.RasterizerState				= raster;
		desc.BlendState						= blend;
		desc.DepthStencilState.DepthEnable		= TRUE;
		desc.DepthStencilState.StencilEnable	= FALSE;
		desc.DepthStencilState.DepthFunc		= D3D12_COMPARISON_FUNC_LESS;
		desc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ALL;
		desc.SampleMask							= UINT_MAX;
		desc.PrimitiveTopologyType				= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets					= 1;
		desc.RTVFormats[0]						= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.DSVFormat							= DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.SampleDesc.Count					= 1;

		hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pipeline_state.GetAddressOf()));
		THROW_IF_HFAIL(hr, "PSO creation fail.")
	}
}

void TestEngine::draw()
{
	auto cmd_alloc	= m_graphics.m_cmd_alloc.Get();
	auto cmd_queue	= m_graphics.m_cmd_queue.Get();
	auto cmd_list	= m_graphics.m_cmd_list.Get();

	auto edge = floorf(sqrtf(m_config.instacing_count));
	for(uint32_t i=0; i<m_config.instacing_count; i++)
	{
		float	r		= (float)i/(float)m_config.instacing_count * 20.f;
		float	idx		= (float)i*10.f;
		auto	freq	= [&](float m){return (idx+m_time_counted)*m; };
		float	tan_f	= tanf(idx);

		auto pos =
		XMVectorLerp(
			XMVectorSet(
				cosf(freq(0.1f)*tan_f)*r,
				sinf(freq( sinf((float)idx)*0.1f )*tan_f)*r,
				-sinf(freq(0.1f)*tan_f)*r,
				1.f
			),
			XMVectorSet(
				2.2f * fmodf(i,edge),
				0.f,
				2.2f * (float)(i/(UINT32)edge),
				1.f
			),
			m_phase
		);
		auto rot = XMVectorLerp(XMQuaternionRotationRollPitchYawFromVector(pos), XMQuaternionIdentity(), m_phase);

		auto it = &((InstancingData*)m_instacing_ptr)[i];
		XMStoreFloat4x4(
			&it->world,
			XMMatrixTransformation(XMVectorZero(), XMQuaternionIdentity(), XMVectorSet(1,1,1,1), XMVectorZero(), rot, pos)
		);
		XMStoreFloat4(
			&it->color,
			XMColorSRGBToRGB(XMColorHSVToRGB(XMVectorSet(fmodf(idx*0.01f,1.f), 0.6f, 1.0f, 1.0f)))
		);
	}


	setRTVCurrent();

	do{
		cmd_alloc->Reset();
		cmd_list->Reset(cmd_alloc, m_pipeline_state.Get());

		// setup pso
		cmd_list->SetGraphicsRootSignature(m_root_signature.Get());

		// set shader uniforms
		{
			ShaderUniforms su;

			auto look_forward	= XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_cam_rot)));
			auto look_up		= XMVector3Rotate(XMVectorSet(0, 1, 0, 0), XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_cam_rot)));
			auto aspect			= (float)m_config.width/(float)m_config.height;
			auto fov			= XMConvertToRadians(60.f);

			XMStoreFloat4x4(
				&su.view,
				XMMatrixLookToLH(XMLoadFloat3(&m_cam_pos), look_forward, look_up)
			);
			XMStoreFloat4x4(
				&su.projection,
				XMMatrixPerspectiveFovLH(fov,aspect,0.1f,1000.f)
			);
			su.camera_position = m_cam_pos;

			cmd_list->SetGraphicsRoot32BitConstants(0, sizeof(ShaderUniforms)/4, &su, 0);
		}

		D3D12_VIEWPORT vp;
		vp.Width	= m_config.width;
		vp.Height	= m_config.height;
		vp.TopLeftX = vp.TopLeftY = 0;
		vp.MinDepth = 0.f;
		vp.MaxDepth	= 1.f;
		cmd_list->RSSetViewports(1, &vp);

		D3D12_RECT sr;
		sr.left		= sr.top = 0;
		sr.right	= m_config.width;
		sr.bottom	= m_config.height;
		cmd_list->RSSetScissorRects(1, &sr);


		// barrier
		D3D12_RESOURCE_BARRIER bar[1];
		ZeroMemory(&bar, sizeof(bar));
		bar[0].Type						= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		bar[0].Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
		bar[0].Transition.pResource		= m_sc_buffers[m_frame_index].Get();
		bar[0].Transition.StateBefore	= D3D12_RESOURCE_STATE_PRESENT;
		bar[0].Transition.StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET;
		bar[0].Transition.Subresource	= D3D12_RESOURCE_BARRIER_FLAG_NONE;
		cmd_list->ResourceBarrier(1, bar);

		auto rt_handle = m_rt_heap->GetCPUDescriptorHandleForHeapStart();
		auto ds_handle = m_ds_heap->GetCPUDescriptorHandleForHeapStart();
		cmd_list->OMSetRenderTargets(1, &rt_handle, TRUE, &ds_handle);

		const float cval[] ={0.5f,0.5f,0.5f,1.f};
		cmd_list->ClearRenderTargetView(rt_handle, cval, 0, nullptr);
		cmd_list->ClearDepthStencilView(ds_handle, D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
		{
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd_list->IASetVertexBuffers(0, 2, m_vertex_views);
			cmd_list->IASetIndexBuffer(&m_index_view);

			cmd_list->DrawIndexedInstanced(m_index_count, m_config.instacing_count, 0, 0, 0);
		}

		bar[0].Transition.StateBefore	= bar[0].Transition.StateAfter;
		bar[0].Transition.StateAfter	= D3D12_RESOURCE_STATE_PRESENT;
		cmd_list->ResourceBarrier(1, bar);

		cmd_list->Close();
	}
	while(false);

	ID3D12CommandList* cmd_lists[] ={cmd_list};
	cmd_queue->ExecuteCommandLists(1, cmd_lists);

	m_swapchain->Present(1, 0);
	m_graphics.waitForDone();
}

int TestEngine::run(int argc, char** argv)
{
	m_phase			= 0.f;
	m_time_delta	= m_time_counted = 0;
	m_time_prev		= Timer::now();
	m_config.width	= 1280;
	m_config.height	= 720;

	m_cnt			= 0;
	m_instance		= GetModuleHandle(nullptr);

	m_config.instacing_count	= 49;
	m_cam_pos.x = 0;
	m_cam_pos.y = 0;
	m_cam_pos.z = -40.f;
	m_cam_rot.x = m_cam_rot.y = m_cam_rot.z = 0.f;


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

	try{
		initGraphics();
		initResources();
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
		//printf("\r Cnt : %d", m_cnt++);

		auto now = Timer::now();
		std::chrono::duration<double> dur = now-m_time_prev;
		m_time_prev		= now;
		m_time_delta	= dur.count();
		m_time_counted	+= m_time_delta;


		auto xr = XInputGetState(0, &m_xinput_state);
		if(xr==ERROR_SUCCESS)
		{
			auto getNormalizedInput = [](float x, float y, float z)->auto
			{
				auto move = XMVectorSet(x,y,z,1.f);
				float	magnitude		= XMVectorGetX(XMVector3Length(move));
				auto	move_norm		= XMVectorScale(move, 1/magnitude);
				float	magnitude_norm	= 0.f;
				if(magnitude>(float)XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
				{
					//clip the magnitude at its expected maximum value 
					magnitude = (magnitude>32767.f) ? 32767.f : magnitude;

					// adjust magnitude relative to the end of the dead zone
					magnitude -= (float)XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

					//optionally normalize the magnitude with respect to its expected range 
					//giving a magnitude value of 0.0 to 1.0 
					magnitude_norm = magnitude/(32767.f-(float)XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
				}
				else{
					magnitude		= 0;
					magnitude_norm	= 0;
				}

				return XMVectorScale(move_norm, magnitude_norm);
			};
			auto move	= getNormalizedInput(m_xinput_state.Gamepad.sThumbLX, 0.f, m_xinput_state.Gamepad.sThumbLY);
			auto torque	= getNormalizedInput(-m_xinput_state.Gamepad.sThumbRY, m_xinput_state.Gamepad.sThumbRX, 0.f);
			
			auto rot	= XMLoadFloat3(&m_cam_rot);

			move = XMVectorAdd(
				XMVector3Rotate(move, XMQuaternionRotationRollPitchYawFromVector(rot)),
				XMVector3Rotate(move, XMQuaternionRotationRollPitchYawFromVector(rot))
			);

			XMStoreFloat3(
				&m_cam_pos,
				XMVectorAdd(XMLoadFloat3(&m_cam_pos), XMVectorScale(move, m_time_delta*5.f))
			);
			XMStoreFloat3(
				&m_cam_rot,
				XMVectorAdd(rot, XMVectorScale(torque, m_time_delta*2.f))
			);
			m_phase = (float)m_xinput_state.Gamepad.bRightTrigger/(float)MAXBYTE;
		}
		draw();
		std::this_thread::yield();
	}
	m_instacing_buffer->Unmap(0, nullptr);

	return 0;
}