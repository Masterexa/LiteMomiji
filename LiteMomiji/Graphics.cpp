#include "Graphics.hpp"
#include <d3dx12.h>
namespace{
	using Microsoft::WRL::ComPtr;
}


UINT64 getRequiredIntermediateSize(
	ID3D12Resource* p_dst_res, UINT subres_first, UINT subres_cnt, UINT64 baseoff,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* p_layouts, UINT* p_row_cnts, UINT64* p_rowsize_in_bytes)
{
	ID3D12Device* device;
	p_dst_res->GetDevice(IID_PPV_ARGS(&device));
	
	UINT64	ret;
	auto	desc = p_dst_res->GetDesc();
	device->GetCopyableFootprints(
		&desc,
		subres_first,
		subres_cnt,
		baseoff,
		p_layouts,
		p_row_cnts,
		p_rowsize_in_bytes,
		&ret
	);
	device->Release();

	return ret;
}


#pragma region Graphics
	void Graphics::init()
	{
		HRESULT hr;

		#ifdef _DEBUG
		{
			ID3D12Debug* debug_ctrl;
			if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_ctrl))))
			{
				debug_ctrl->EnableDebugLayer();
				debug_ctrl->Release();
			}
		}
		#endif


		// setup Direct3D 12
		{
			hr = D3D12CreateDevice(
				nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf())
			);
			THROW_IF_HFAIL(hr, "D3D12CreateDevice() Fail.")


			// Command Queue
			D3D12_COMMAND_QUEUE_DESC qdesc;
			qdesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
			qdesc.Type		= D3D12_COMMAND_LIST_TYPE_DIRECT;
			qdesc.NodeMask	= 0;
			qdesc.Priority	= 0;

			hr = m_device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(m_cmd_queue.GetAddressOf()));
			THROW_IF_HFAIL(hr, "CreateCommandQueue() Fail.")

			// Command Allocator
			hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmd_alloc.GetAddressOf()));
			THROW_IF_HFAIL(hr, "CreateCommandAllocator() Fail.")

			// Command List
			hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_alloc.Get(), nullptr, IID_PPV_ARGS(m_cmd_list.GetAddressOf()));
			THROW_IF_HFAIL(hr, "CreateCommandList() Fail.")
			m_cmd_list->Close();

			// Fence
			hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
			THROW_IF_HFAIL(hr, "CreateFence() Fail.")

			m_fence_value = 1;
			m_fence_event = CreateEventEx(nullptr, TEXT(""), 0, EVENT_ALL_ACCESS);
			if(!m_fence_event)
			{
				throw std::runtime_error("CreateEventEx() Fail.");
			}

			m_RTV_INC = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		{
			hr = CreateDXGIFactory(IID_PPV_ARGS(m_dxgi_factory.GetAddressOf()));
			THROW_IF_HFAIL(hr, "CreateDXGIFactory() Fail.")
		}
	}
	//-------------------------------------------------------------------------
	
	void Graphics::cleanup()
	{

	}
	//-------------------------------------------------------------------------

	bool Graphics::updateSubresources(ID3D12Resource* p_resource, UINT first, UINT count, D3D12_SUBRESOURCE_DATA* p_subresources)
	{
		bool ret = false;
		m_cmd_alloc->Reset();
		m_cmd_list->Reset(m_cmd_alloc.Get(), nullptr);

		ComPtr<ID3D12Resource>	intermediate;

		HRESULT hr;
		{
			auto required_size = getRequiredIntermediateSize(p_resource, first, count, 0, nullptr, nullptr, nullptr);
			D3D12_RESOURCE_DESC desc=
			{
				D3D12_RESOURCE_DIMENSION_BUFFER,
				0,
				required_size,
				1,
				1,
				1,
				DXGI_FORMAT_UNKNOWN,
				{1,0},
				D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				D3D12_RESOURCE_FLAG_NONE
			};
			D3D12_HEAP_PROPERTIES props=
			{
				D3D12_HEAP_TYPE_UPLOAD,
				D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				D3D12_MEMORY_POOL_UNKNOWN,
				1,1
			};

			hr = m_device->CreateCommittedResource(
				&props,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(intermediate.GetAddressOf())
			);
			if(FAILED(hr))
			{
				return false;
			}
			intermediate->SetName(L"INTERMEDIATE");
		}

		D3D12_RESOURCE_BARRIER barrier={};
		{
			barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource	= p_resource;
			barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_COMMON;
			barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_COPY_DEST;
			m_cmd_list->ResourceBarrier(1, &barrier);
		}
		{
			UINT64	required_size;
			UINT64	buf_size = count*(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT)+sizeof(UINT)+sizeof(UINT64));
			if(buf_size>SIZE_MAX)
			{
				goto GOTO_FINISHED;
			}

			auto buffer = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)buf_size);
			if(!buffer)
			{
				goto GOTO_FINISHED;
			}

			auto p_layouts		= reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(buffer);
			auto p_row_sizes	= reinterpret_cast<UINT64*>(p_layouts + count);
			auto p_row_cnt		= reinterpret_cast<UINT*>(p_row_sizes + count);
			required_size		= getRequiredIntermediateSize(p_resource, first, count, 0, p_layouts, p_row_cnt, p_row_sizes);

			hr = UpdateSubresources(m_cmd_list.Get(), p_resource, intermediate.Get(), first, count, required_size, p_subresources);
			if(FAILED(hr))
			{
				goto GOTO_FINISHED;
			}
			ret = true;
		}
		GOTO_FINISHED:{
			barrier.Transition.StateBefore	= D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter	= D3D12_RESOURCE_STATE_GENERIC_READ;
			m_cmd_list->ResourceBarrier(1, &barrier);

			ID3D12CommandList* lists[] ={static_cast<ID3D12CommandList*>(m_cmd_list.Get())};
			m_cmd_list->Close();
			m_cmd_queue->ExecuteCommandLists(1, lists);

			waitForDone();
		}

		return ret;
	}
	//-------------------------------------------------------------------------

	void Graphics::waitForDone()
	{
		const auto f = m_fence_value;
		auto hr = m_cmd_queue->Signal(m_fence.Get(), f);
		if(FAILED(hr))
		{
			return;
		}
		m_fence_value++;

		if(m_fence->GetCompletedValue()<f)
		{
			hr = m_fence->SetEventOnCompletion(f, m_fence_event);
			if(FAILED(hr))
			{
				return;
			}
			WaitForSingleObject(m_fence_event, INFINITE);
		}
	}
	//-------------------------------------------------------------------------
#pragma endregion
