#pragma once

#include "pre.hpp"
#include <vector>


struct GraphicsBatch{
	/* Instance */
		/* Fields */
			std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>	m_srv_handle;
			std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>	m_sampler_handle;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_srv_heap;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_sampler_heap;
};


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
			size_t						m_rtv_count;
			bool						m_ds_enabled;

		/* inits */
			void init(Graphics* graph);

		/* Methods */
			uint32_t calculateDescriptorHeapCapacity(uint32_t srv_each_draw, uint32_t samplers_each_draw);
			void setRenderTarget(ID3D12Resource** rt_res, D3D12_RENDER_TARGET_VIEW_DESC* rt_descs, uint32_t rt_res_cnt, ID3D12Resource* ds_res, D3D12_DEPTH_STENCIL_VIEW_DESC* ds_desc);

			void begin();
			void end();

			void setPipelineState(PipelineState* pso);

			void clearRenderTarget(uint32_t num, const FLOAT rgba[4], UINT rect_cnt, const D3D12_RECT* rects);
			void clearDepthStencil(D3D12_CLEAR_FLAGS flag, float depth, uint8_t stencil, UINT rect_cnt, const D3D12_RECT* rects);
};