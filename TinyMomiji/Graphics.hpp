#pragma once

#include "pre.hpp"
#include <wrl/client.h>
#include <d3d12.h>

struct Graphics{

	/* Instance */
		/* Fields */
			Microsoft::WRL::ComPtr<IDXGIFactory4>				m_dxgi_factory;
			Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
			Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_cmd_queue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_cmd_alloc;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_cmd_list;
			Microsoft::WRL::ComPtr<ID3D12Fence>					m_fence;
			UINT64	m_fence_value;
			HANDLE	m_fence_event;
			UINT	m_RTV_INC;


		/* Methods */
			void init();
			void cleanup();

			bool updateSubresources(ID3D12Resource* p_resource, UINT first, UINT count, D3D12_SUBRESOURCE_DATA* p_subresources);
			void waitForDone();
};