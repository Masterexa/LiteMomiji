#pragma once

/* include */
#include "pre.hpp"
#include <vector>

struct DebugDrawerLineVertex
{
	DirectX::XMFLOAT3				position;
	DirectX::PackedVector::XMCOLOR	color;
};

struct DebugDrawerShaderUniform
{
	DirectX::XMFLOAT4X4	view_matrix;
	DirectX::XMFLOAT4X4	projection_matrix;
};

struct DebugDrawer{

	/* Instance */
		/* Fields */
			bool m_initialized;
			Microsoft::WRL::ComPtr<ID3D12Resource>		m_buffer;
			std::unique_ptr<PipelineState>				m_pso_line;

			D3D12_VERTEX_BUFFER_VIEW	m_line_view[1];
			size_t						m_line_buffer_capacity;
			uint8_t*	m_buffer_mapped;
			uint8_t*	m_buffer_line_start;

			DirectX::XMFLOAT4X4					m_matrix;
			DebugDrawerShaderUniform			m_shader_uniform;
			std::vector<DebugDrawerLineVertex>	m_line_list;
			size_t								m_line_cnt;

		/* Methods */
			DebugDrawer();
			~DebugDrawer();

			void init(Graphics* graph);
			void shutdown();

			void render(Graphics* graph, GraphicsContext* context);
			void flush();

			void XM_CALLCONV setVPMatrix(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj);
			void XM_CALLCONV drawLines(DirectX::XMFLOAT3* in_lines, size_t lines_length, DirectX::XMVECTOR color_start, DirectX::XMVECTOR color_end);
			void XM_CALLCONV drawWireCube(DirectX::FXMVECTOR origin, DirectX::FXMVECTOR scale, DirectX::FXMVECTOR pivot, DirectX::FXMVECTOR color);
};