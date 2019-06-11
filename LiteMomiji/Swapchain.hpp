#pragma once

/* include */
#include "pre.hpp"
#include "RenderTarget.hpp"

struct SwapchainDesc
{
	DXGI_SWAP_CHAIN_DESC	dxgi_swapchain_desc;
	bool					use_srgb;
	bool					create_with_depthstencil;
};

struct Swapchain{

	/* Instance */
		/* Fields */
			Graphics*	m_graph;
			Microsoft::WRL::ComPtr<IDXGISwapChain3>		m_swapchain;
			UINT							m_frame_index;
			std::unique_ptr<RenderTarget>	m_window_rtv;

		/* Methods */
			Swapchain();
			~Swapchain();
			void init(Graphics* graph, SwapchainDesc* desc);
			void shutdown();

			void present();
};