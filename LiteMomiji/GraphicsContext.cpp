/* declatation */
#include "GraphicsContext.hpp"

/* includes */
#include "Graphics.hpp"
#include "PipelineState.hpp"

void GraphicsContext::init(Graphics* graph)
{
	HRESULT hr;
	auto device = graph->m_device.Get();
	m_graphics	= graph;

	// Command Allocator
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmd_alloc.GetAddressOf()));
	THROW_IF_HFAILED(hr, "CreateCommandAllocator() Fail.")

	// Command List
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_alloc.Get(), nullptr, IID_PPV_ARGS(m_cmd_list.GetAddressOf()));
	THROW_IF_HFAILED(hr, "CreateCommandList() Fail.")
	m_cmd_list->Close();

	{
		D3D12_DESCRIPTOR_HEAP_DESC hdesc;
		ZeroMemory(&hdesc, sizeof(hdesc));
		hdesc.NumDescriptors	= 8;
		hdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		hdesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		// Create RTV
		hr = device->CreateDescriptorHeap(&hdesc, IID_PPV_ARGS(m_rtv_heap.GetAddressOf()));
		THROW_IF_HFAILED(hr, "RTV heap creation fail.")

		// Create DSV
		hdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		hr = device->CreateDescriptorHeap(&hdesc, IID_PPV_ARGS(m_dsv_heap.GetAddressOf()));
		THROW_IF_HFAILED(hr, "RTV heap creation fail.")

		for(auto& it : m_rtv_barriers)
		{
			ZeroMemory(&it, sizeof(it));
			it.Type						= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			it.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
			it.Transition.StateBefore	= D3D12_RESOURCE_STATE_PRESENT;
			it.Transition.StateAfter	= D3D12_RESOURCE_STATE_RENDER_TARGET;
			it.Transition.Subresource	= D3D12_RESOURCE_BARRIER_FLAG_NONE;
		}
		m_rtv_count		= 0;
		m_ds_enabled	= false;
	}
}

void GraphicsContext::setRenderTarget(ID3D12Resource** rt_res, D3D12_RENDER_TARGET_VIEW_DESC* rt_descs, uint32_t rt_res_cnt, ID3D12Resource* ds_res, D3D12_DEPTH_STENCIL_VIEW_DESC* ds_desc)
{
	auto device = m_graphics->m_device.Get();
	

	m_rtv_count = rt_res_cnt;

	// Create RTV
	auto rtv_handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	for(size_t i = 0; i <m_rtv_count; i++)
	{
		m_rtv_barriers[i].Transition.pResource = rt_res[i];
		device->CreateRenderTargetView(rt_res[i], &rt_descs[i], rtv_handle);
		
		rtv_handle.ptr += m_graphics->m_RTV_INC;
	}

	// create DSV
	m_ds_enabled = ds_res!=nullptr;
	if(m_ds_enabled)
	{
		device->CreateDepthStencilView(
			ds_res,
			ds_desc,
			m_dsv_heap->GetCPUDescriptorHandleForHeapStart()
		);
	}
}

void GraphicsContext::begin()
{
	m_cmd_alloc->Reset();
	m_cmd_list->Reset(m_cmd_alloc.Get(), nullptr);

	for(size_t i = 0; i <m_rtv_count; i++)
	{
		m_rtv_barriers[i].Transition.StateBefore	= D3D12_RESOURCE_STATE_PRESENT;
		m_rtv_barriers[i].Transition.StateAfter		= D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	m_cmd_list->ResourceBarrier(m_rtv_count, m_rtv_barriers);

	auto rt_handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	auto ds_handle = m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
	m_cmd_list->OMSetRenderTargets(m_rtv_count, &rt_handle, TRUE, m_ds_enabled ? &ds_handle : nullptr);
}

void GraphicsContext::end()
{
	for(size_t i = 0; i <m_rtv_count; i++)
	{
		m_rtv_barriers[i].Transition.StateBefore	= m_rtv_barriers[i].Transition.StateAfter;
		m_rtv_barriers[i].Transition.StateAfter		= D3D12_RESOURCE_STATE_PRESENT;
	}
	m_cmd_list->ResourceBarrier(m_rtv_count, m_rtv_barriers);

	m_cmd_list->Close();
}

void GraphicsContext::setPipelineState(PipelineState* pso)
{
	m_cmd_list->SetPipelineState(pso->m_pipeline_state.Get());
	m_cmd_list->SetGraphicsRootSignature(pso->m_root_signature.Get());
}

void GraphicsContext::clearRenderTarget(uint32_t num, const FLOAT rgba[4], UINT rect_cnt, const D3D12_RECT* rects)
{
	auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr	+= m_graphics->m_RTV_INC*num;

	m_cmd_list->ClearRenderTargetView(handle, rgba, rect_cnt, rects);
}

void GraphicsContext::clearDepthStencil(D3D12_CLEAR_FLAGS flags, float depth, uint8_t stencil, UINT rect_cnt, const D3D12_RECT* rects)
{
	if(!m_ds_enabled)
	{
		return;
	}
	m_cmd_list->ClearDepthStencilView(m_dsv_heap->GetCPUDescriptorHandleForHeapStart(), flags, depth, stencil, rect_cnt, rects);
}