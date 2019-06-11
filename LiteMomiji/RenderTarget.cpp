/* declaration */
#include "RenderTarget.hpp"

/* include */
#include "Graphics.hpp"

RenderTarget::RenderTarget()
{
	m_initialized			= false;
	m_buffer_current_index	= 0;
}

RenderTarget::~RenderTarget()
{
	shutdown();
}

void RenderTarget::init(Graphics* graph, RenderTargetDesc* desc)
{
	if(m_initialized)
	{
		return;
	}
	HRESULT hr;
	auto device = graph->m_device.Get();

	m_graph				= graph;
	m_backbuffer_count	= desc->backbuffer_count;
	m_target_count		= desc->target_count;

	if(m_target_count>0)
	{
		D3D12_DESCRIPTOR_HEAP_DESC hdesc={};
		hdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		hdesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		hdesc.NumDescriptors	= m_backbuffer_count * m_target_count;

		// Create RTV heap
		hr = device->CreateDescriptorHeap(&hdesc, IID_PPV_ARGS(m_rtv_heap.GetAddressOf()));
		THROW_IF_HFAILED(hr, "RTV heap creation fail.")

		// Allocate resource array
		m_buffers.reserve(m_backbuffer_count * m_target_count);
		m_buffers.resize(m_backbuffer_count * m_target_count);

		for(size_t ib = 0; ib<m_backbuffer_count; ib++)
		{
			for(size_t ir = 0; ir<m_target_count; ir++)
			{
				// Copy pointers to field
				auto i = ib*m_target_count + ir;
				m_buffers[i] = desc->targets_each_backbuffer_ptr[i];
				m_buffers[i]->AddRef();

				// Create RTV
				auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
				handle.ptr	+= graph->m_RTV_INC*i;
				device->CreateRenderTargetView(m_buffers[i], &desc->rendertarget_view_descs[ir], handle);
			}
		}
	}


	m_target_count	= desc->target_count;

	if(desc->depthstencil_buffer_ptr)
	{
		D3D12_DESCRIPTOR_HEAP_DESC hdesc={};
		hdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		hdesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		hdesc.NumDescriptors	= 1;

		// Create DSV heap
		hr = device->CreateDescriptorHeap(&hdesc, IID_PPV_ARGS(m_dsv_heap.GetAddressOf()));
		THROW_IF_HFAILED(hr, "RTV heap creation fail.")

		// Create DSV
		m_ds_buffer = desc->depthstencil_buffer_ptr;
		device->CreateDepthStencilView(m_ds_buffer.Get(), &desc->depthstencil_view_desc, m_dsv_heap->GetCPUDescriptorHandleForHeapStart());
	}

	m_initialized = true;
}

void RenderTarget::shutdown()
{
	if(!m_initialized)
	{
		return;
	}
	m_dsv_heap.Reset();

	m_rtv_heap.Reset();
	for(auto& it : m_buffers)
	{
		it->Release();
	}
	m_buffers.clear();

	m_initialized = false;
}

void RenderTarget::setBackBufferIndex(uint32_t index)
{
	m_buffer_current_index = index;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::getCurrentRTV() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE h={0};
	if(m_rtv_heap.Get()==nullptr)
	{
		return h;
	}

	h = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	h.ptr += m_graph->m_RTV_INC*m_buffer_current_index;
	return h;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::getDSV() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE h={0};
	h = m_dsv_heap.Get()!=nullptr ? m_dsv_heap->GetCPUDescriptorHandleForHeapStart() : h;

	return h;
}