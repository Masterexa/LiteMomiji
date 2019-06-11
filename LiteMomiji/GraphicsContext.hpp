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

			bool	m_recoding;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_cmd_alloc;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_cmd_list;

			std::vector<D3D12_VERTEX_BUFFER_VIEW>	m_vb_views;

			RenderTarget*				m_render_target;
			uint32_t					m_prev_rt_index;
			D3D12_CPU_DESCRIPTOR_HANDLE	m_rtv_handle;
			D3D12_CPU_DESCRIPTOR_HANDLE	m_dsv_handle;
			D3D12_RESOURCE_BARRIER		m_rtv_barriers[8];
			size_t						m_rtv_count;
			bool						m_ds_enabled;

			D3D12_VIEWPORT	m_viewports[D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX];
			size_t			m_viewports_count;
			D3D12_RECT		m_scissor_rects[D3D12_VIEWPORT_AND_SCISSORRECT_MAX_INDEX];
			size_t			m_scissor_rects_count;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_srv_heap;


		/* inits */
			GraphicsContext();
			void init(Graphics* graph);

		/* Methods */
			uint32_t calculateDescriptorHeapCapacity(uint32_t srv_each_draw, uint32_t samplers_each_draw);
			void setRenderTarget(RenderTarget* target);
			void setRenderTarget(ID3D12Resource** rt_res, uint32_t rtv_cnt, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv);

			void begin();
			void end();

			void setPipelineState(PipelineState* pso);

			void setViewports(uint32_t vp_cnt, D3D12_VIEWPORT* vp);
			void setScissorRects(uint32_t sc_cnt, D3D12_RECT* sc);

			void clearRenderTarget(uint32_t num, const FLOAT rgba[4], UINT rect_cnt, const D3D12_RECT* rects);
			void clearDepthStencil(D3D12_CLEAR_FLAGS flag, float depth, uint8_t stencil, UINT rect_cnt, const D3D12_RECT* rects);
};