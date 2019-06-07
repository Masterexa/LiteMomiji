#pragma once

#include "pre.hpp"


struct ImguiModule{

	/* Instance */
		/* Fields */
			bool	m_initialized;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	m_dheap;

		/* Methods */
			ImguiModule();
			~ImguiModule();
			void init(Graphics* graph, HWND hwnd, uint32_t backbuffer_cnt, DXGI_FORMAT backbuffer_format);
			void shutdown();

			void newFrame();
			void render(GraphicsContext* context);

			void resizeBegin();
			void resizeEnd();

			LRESULT sendWindowMessage(HWND, UINT, WPARAM, LPARAM);
};