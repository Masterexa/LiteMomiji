#pragma once

/* includes */
#include "pre.hpp"


struct PipelineState{

	/* Instance */
		/* Fields */
			Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_root_signature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState>	m_pipeline_state;
			Graphics*	m_graphics;

		/* Inits */
			void init(Graphics* graph, D3D12_ROOT_SIGNATURE_DESC* rs_desc, D3D12_GRAPHICS_PIPELINE_STATE_DESC* ps_desc);
};