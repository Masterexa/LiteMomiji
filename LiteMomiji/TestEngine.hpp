#pragma once

#include "pre.hpp"
#include "Engine.hpp"
#include "Graphics.hpp"
#include "GraphicsContext.hpp"
#include "PipelineState.hpp"
#include "Mesh.hpp"
#include "ImguiModule.hpp"
#include "DebugDrawer.hpp"
#include "Input.hpp"

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
			std::unique_ptr<Graphics>			m_gfx;
			std::unique_ptr<GraphicsContext>	m_gfx_context;

			Microsoft::WRL::ComPtr<IDXGISwapChain3>				m_swapchain;
			Microsoft::WRL::ComPtr<ID3D12Resource>				m_ds_buffer;
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>	m_sc_buffers;
			UINT m_frame_index;

			// Mesh State
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_instacing_buffer;
			std::vector<D3D12_VERTEX_BUFFER_VIEW>	m_vertex_views;
			UINT8*									m_instacing_ptr;

			// Images
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_tex_ao;
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_tex_env;
			DXGI_FORMAT	m_tex_ao_fmt;
			DXGI_FORMAT	m_tex_env_fmt;

			std::unique_ptr<PipelineState>	m_pso;
			DirectX::XMFLOAT3	m_cam_pos;
			DirectX::XMFLOAT3	m_cam_rot;
			float				m_cam_fov;
			float				m_ao;

			HINSTANCE	m_instance;
			HWND		m_hwnd;
			uint32_t	m_cnt;

			float			m_phase;
			XINPUT_STATE	m_xinput_state;

			std::unique_ptr<Mesh> m_mesh_cube;
			std::unique_ptr<Mesh> m_mesh_plane;
			std::unique_ptr<Mesh> m_mesh_file;

			// Sky
			std::unique_ptr<PipelineState>	m_pso_skybox;
			std::unique_ptr<Mesh>			m_mesh_skysphere;

			// Debug
			std::unique_ptr<DebugDrawer>	m_debug_drawer;

			// Misc modules
			std::unique_ptr<ImguiModule>	m_imgui;
			bool	m_demo_window;
			std::unique_ptr<Input>	m_input;

	public:
		/* Methods */
			int run(int argc, char** argv);
			void update();
			void render();
			void setRTVCurrent();

			void initWindow();
			void initGraphics();
			void initResources();
			void shutdown();

			LRESULT wndProc(HWND, UINT, WPARAM, LPARAM);
};