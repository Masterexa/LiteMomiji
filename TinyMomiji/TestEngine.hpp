#pragma once

#include "Engine.hpp"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <stdint.h>
#include <vector>
#include <DirectXMath.h>
#include <chrono>

struct Config{
	uint32_t	width;
	uint32_t	height;
	uint32_t	instacing_count;
};


class TestEngine{

	using Timer = std::chrono::high_resolution_clock;

	/* Instance */
		/* Fields */
			Timer::time_point	m_time_prev;
			double				m_time_delta;
			double				m_time_counted;

			Microsoft::WRL::ComPtr<ID3D12Device>				m_device;
			Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_cmd_queue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_cmd_alloc;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_cmd_list;
			Microsoft::WRL::ComPtr<ID3D12Fence>					m_fence;
			UINT64	m_fence_value;
			HANDLE	m_fence_event;
			UINT m_RTV_INC;

			Microsoft::WRL::ComPtr<IDXGISwapChain3>				m_swapchain;
			Microsoft::WRL::ComPtr<ID3D12Resource>				m_ds_buffer;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_ds_heap;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_rt_heap;
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>	m_sc_buffers;
			UINT m_frame_index;

			Microsoft::WRL::ComPtr<ID3D12Resource>	m_vertex_buffer;
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_index_buffer;
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_instacing_buffer;
			D3D12_VERTEX_BUFFER_VIEW	m_vertex_views[2];
			D3D12_INDEX_BUFFER_VIEW		m_index_view;
			uint32_t					m_index_count;
			UINT8*						m_instacing_ptr;

			Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_root_signature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState>	m_pipeline_state;
			DirectX::XMFLOAT3	m_cam_pos;
			DirectX::XMFLOAT3	m_cam_rot;


			Config		m_config;
			HINSTANCE	m_instance;
			HWND		m_hwnd;
			uint32_t	m_cnt;

	public:
		/* Methods */

			int run(int argc, char** argv);
			void draw();
			void waitForGpu();
			void setRTVCurrent();

			void initGraphics();
			void initResources();
};