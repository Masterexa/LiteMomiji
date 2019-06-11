#pragma once

/* include */
#include "pre.hpp"
#include <vector>

struct RenderTargetDesc
{
	uint32_t	target_count;
	uint32_t	backbuffer_count;
	
	ID3D12Resource**				targets_each_backbuffer_ptr;
	D3D12_RENDER_TARGET_VIEW_DESC*	rendertarget_view_descs;

	ID3D12Resource*					depthstencil_buffer_ptr;
	D3D12_DEPTH_STENCIL_VIEW_DESC	depthstencil_view_desc;
};

struct RenderTarget{

	/* Instance */
		/* Fields */
			bool		m_initialized;
			uint32_t	m_buffer_current_index;
			uint32_t	m_target_count;
			uint32_t	m_backbuffer_count;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_rtv_heap;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_dsv_heap;
			std::vector<ID3D12Resource*>					m_buffers;
			Microsoft::WRL::ComPtr<ID3D12Resource>			m_ds_buffer;
			Graphics*	m_graph;


		/* Methods */
			RenderTarget();
			~RenderTarget();

			void init(Graphics* graph, RenderTargetDesc* desc);
			void shutdown();
			void setBackBufferIndex(uint32_t index);
			
			D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV() const;
			D3D12_CPU_DESCRIPTOR_HANDLE getDSV() const;
};