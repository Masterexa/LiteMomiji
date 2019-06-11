/* declaration */
#include "Swapchain.hpp"

#include "Graphics.hpp"
#include <DirectXTex.h>
namespace{
	using Microsoft::WRL::ComPtr;
}




Swapchain::Swapchain()
{
	m_graph = nullptr;
}

Swapchain::~Swapchain()
{
	shutdown();
}

void Swapchain::init(Graphics* graph, SwapchainDesc* desc)
{
	if(m_graph)
	{
		return;
	}
	HRESULT hr;
	auto device		= graph->m_device.Get();
	auto factory	= graph->m_dxgi_factory.Get();


	// Create Swapchain
	std::vector<ID3D12Resource*> rtr;
	ComPtr<ID3D12Resource>	dsb;
	{
		ComPtr<IDXGISwapChain>	sc;
		hr = factory->CreateSwapChain(graph->m_cmd_queue.Get(), &desc->dxgi_swapchain_desc, sc.GetAddressOf());
		THROW_IF_HFAILED(hr, "CreateSwapChain() Fail.")

		// cast to 3
		hr = sc->QueryInterface(m_swapchain.GetAddressOf());
		THROW_IF_HFAILED(hr, "QueryInterface() Fail.")

		m_frame_index =  m_swapchain->GetCurrentBackBufferIndex();


		rtr.reserve(desc->dxgi_swapchain_desc.BufferCount);
		rtr.resize(rtr.capacity());

		for(size_t i=0; i<rtr.size(); i++)
		{
			m_swapchain->GetBuffer(i, IID_PPV_ARGS(&rtr[i]));
			THROW_IF_HFAILED(hr, "GetBuffer() Fail.")
		}
	}


	// Create DS Buffer
	if(desc->create_with_depthstencil)
	{
		D3D12_HEAP_PROPERTIES prop={};
		prop.Type					= D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask		= 1;
		prop.VisibleNodeMask		= 1;

		D3D12_RESOURCE_DESC ds_desc ={};
		ds_desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ds_desc.Alignment			= 0;
		ds_desc.Width				= desc->dxgi_swapchain_desc.BufferDesc.Width;
		ds_desc.Height				= desc->dxgi_swapchain_desc.BufferDesc.Height;
		ds_desc.DepthOrArraySize	= 1;
		ds_desc.MipLevels			= 0;
		ds_desc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
		ds_desc.SampleDesc.Count	= 1;
		ds_desc.SampleDesc.Quality	= 0;
		ds_desc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		ds_desc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_CLEAR_VALUE cval ={};
		cval.Format					= DXGI_FORMAT_D24_UNORM_S8_UINT;
		cval.DepthStencil.Depth		= 1.f;
		cval.DepthStencil.Stencil	= 0;

		// create buffer
		hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&ds_desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&cval,
			IID_PPV_ARGS(dsb.GetAddressOf())
		);
		THROW_IF_HFAILED(hr, "DS buffer creation fail.")
	}


	// Create the render target
	{
		D3D12_RENDER_TARGET_VIEW_DESC rdesc;
		rdesc.Format				= desc->use_srgb ? DirectX::MakeSRGB(desc->dxgi_swapchain_desc.BufferDesc.Format) : desc->dxgi_swapchain_desc.BufferDesc.Format;
		rdesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
		rdesc.Texture2D.MipSlice	= 0;
		rdesc.Texture2D.PlaneSlice	= 0;

		RenderTargetDesc rt_desc={};
		rt_desc.target_count					= 1;
		rt_desc.backbuffer_count				= 2;
		rt_desc.targets_each_backbuffer_ptr		= rtr.data();
		rt_desc.rendertarget_view_descs			= &rdesc;
		rt_desc.depthstencil_buffer_ptr			= dsb.Get();

		auto& dsd = rt_desc.depthstencil_view_desc;
		ZeroMemory(&dsd, sizeof(dsd));
		dsd.Format			= DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsd.ViewDimension	= D3D12_DSV_DIMENSION_TEXTURE2D;
		dsd.Flags			= D3D12_DSV_FLAG_NONE;

		m_window_rtv.reset(new RenderTarget());
		
		try{
			m_window_rtv->init(graph, &rt_desc);
		}
		catch(std::exception& e)
		{
			for(auto& it : rtr)
			{
				it->Release();
			}
			throw e;
		}


		for(auto& it : rtr)
		{
			it->Release();
		}
	}
	m_graph = graph;
}

void Swapchain::shutdown()
{
	if(!m_graph)
	{
		return;
	}
	m_window_rtv.reset();
	m_swapchain.Reset();

	m_graph = nullptr;
}

void Swapchain::present()
{
	m_swapchain->Present(1, 0);
	m_frame_index = m_swapchain->GetCurrentBackBufferIndex();
}