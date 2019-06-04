#pragma once

#include "pre.hpp"
#include <vector>

struct GraphicsContext{

	/* Instance */
		/* Fields */
			Graphics*	m_graphics;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_cmd_alloc;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_cmd_list;

			std::vector<D3D12_VERTEX_BUFFER_VIEW>	m_vb_views;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_rtv_heap;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_dsv_heap;
			D3D12_RESOURCE_BARRIER		m_rtv_barriers[8];
			D3D12_CLEAR_VALUE			m_clear_value;
			size_t						m_rtv_count;
			bool						m_ds_enabled;

		/* inits */
			void init(Graphics* graph);

		/* Methods */
			void setRenderTarget(ID3D12Resource** rt_res, D3D12_RENDER_TARGET_VIEW_DESC* rt_descs, uint32_t rt_res_cnt, ID3D12Resource* ds_res, D3D12_DEPTH_STENCIL_VIEW_DESC* ds_desc);

			void begin();
			void end();
};