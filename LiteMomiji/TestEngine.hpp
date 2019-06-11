#pragma once

#include "pre.hpp"
#include "Engine.hpp"
#include "Graphics.hpp"
#include "GraphicsContext.hpp"
#include "PipelineState.hpp"
#include "Mesh.hpp"
#include "RenderTarget.hpp"
#include "ImguiModule.hpp"
#include "DebugDrawer.hpp"
#include "Input.hpp"
#include "Swapchain.hpp"

#include <vector>
#include <chrono>
#include <memory>

struct Config{
	uint32_t	width;
	uint32_t	height;
	uint32_t	instancing_count;
};


class TestEngine{

	using Timer = std::chrono::high_resolution_clock;

	/* Instance */
		/* Fields */
			Timer::time_point	m_time_prev;
			double				m_time_delta;
			double				m_time_counted;

			Config		m_config;
			float		m_phase;
			bool		m_demo_window;
			uint32_t	m_cnt;

			// Graphics
			std::unique_ptr<Graphics>			m_gfx;
			std::unique_ptr<GraphicsContext>	m_gfx_context;

			// Swapchian
			HINSTANCE	m_instance;
			HWND		m_hwnd;
			std::unique_ptr<Swapchain>	m_swapchain;


			// Mesh State
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_instancing_buffer;
			std::vector<D3D12_VERTEX_BUFFER_VIEW>	m_vertex_views;
			UINT8*									m_instancing_ptr;

			// Meshes
			std::unique_ptr<Mesh> m_mesh_cube;
			std::unique_ptr<Mesh> m_mesh_plane;
			std::unique_ptr<Mesh> m_mesh_file;

			// Images
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_tex_ao;
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_tex_env;
			DXGI_FORMAT	m_tex_ao_fmt;
			DXGI_FORMAT	m_tex_env_fmt;

			// Forward States
			std::unique_ptr<PipelineState>	m_forward_pso;
			std::unique_ptr<PipelineState>	m_foward_shadow_caster;
			DirectX::XMFLOAT3	m_cam_pos;
			DirectX::XMFLOAT3	m_cam_rot;
			DirectX::XMFLOAT3	m_light_rot;
			float				m_cam_fov;
			float				m_ao;

			// Sky
			std::unique_ptr<PipelineState>	m_pso_skybox;
			std::unique_ptr<Mesh>			m_mesh_skysphere;

			// Shadow map
			std::unique_ptr<RenderTarget>	m_shadowmap;
			DXGI_FORMAT						m_shadowmap_fmt;
			DirectX::XMINT2					m_shadowmap_resolution;


			// Misc modules
			std::unique_ptr<DebugDrawer>	m_debug_drawer;
			std::unique_ptr<ImguiModule>	m_imgui;
			std::unique_ptr<Input>			m_input;

	public:
		/* Methods */
			int run(int argc, char** argv);
			void update();
			void render();
			void setRTVCurrent();
			void drawObjects(ID3D12GraphicsCommandList* cmd_list);

			void initWindow();
			void initGraphics();
			void initResources();
			void shutdown();

			LRESULT wndProc(HWND, UINT, WPARAM, LPARAM);
};