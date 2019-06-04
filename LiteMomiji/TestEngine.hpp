#pragma once

#include "pre.hpp"
#include "Engine.hpp"
#include "Mesh.hpp"
#include "Graphics.hpp"
#include "GraphicsContext.hpp"
#include <Xinput.h>
#include <vector>
#include <chrono>
#include <memory>

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

			Config		m_config;
			Graphics	m_graphics;
			std::unique_ptr<GraphicsContext>	m_gfx_context;

			Microsoft::WRL::ComPtr<IDXGISwapChain3>				m_swapchain;
			Microsoft::WRL::ComPtr<ID3D12Resource>				m_ds_buffer;
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>	m_sc_buffers;
			UINT m_frame_index;

			// Mesh State
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_instacing_buffer;
			std::vector<D3D12_VERTEX_BUFFER_VIEW>	m_vertex_views;
			UINT8*									m_instacing_ptr;

			Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_root_signature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState>	m_pipeline_state;
			DirectX::XMFLOAT3	m_cam_pos;
			DirectX::XMFLOAT3	m_cam_rot;

			HINSTANCE	m_instance;
			HWND		m_hwnd;
			uint32_t	m_cnt;

			float			m_phase;
			XINPUT_STATE	m_xinput_state;

			std::unique_ptr<Mesh> m_mesh_cube;
			std::unique_ptr<Mesh> m_mesh_plane;

	public:
		/* Methods */

			int run(int argc, char** argv);
			void draw();
			void setRTVCurrent();

			void initGraphics();
			void initResources();
};