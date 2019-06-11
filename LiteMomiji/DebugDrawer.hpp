#pragma once

/* include */
#include "pre.hpp"
#include <vector>

struct DebugDrawerLineVertex
{
	DirectX::XMFLOAT3				position;
	DirectX::PackedVector::XMCOLOR	color;
};

struct DebugDrawerTextureVertex
{
	DirectX::XMFLOAT2	position;
	DirectX::XMFLOAT2	uv;
};

struct DebugDrawerTextureInstacing
{
	DirectX::XMFLOAT4	rect;
	float				z_order;
	ID3D12Resource*		resource;
	DXGI_FORMAT			format;
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
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_buffer;
			std::unique_ptr<PipelineState>			m_pso_line;
			std::unique_ptr<PipelineState>			m_pso_texture;
			DebugDrawerShaderUniform				m_shader_uniform;
			DebugDrawerShaderUniform				m_su_screen;

			D3D12_VERTEX_BUFFER_VIEW	m_line_view[1];
			D3D12_VERTEX_BUFFER_VIEW	m_tex_vertex_view[2];
			D3D12_INDEX_BUFFER_VIEW		m_tex_index_view;
			uint8_t*	m_buffer_mapped;
			uint8_t*	m_buffer_line_start;
			uint8_t*	m_buffer_texture_start;

			DirectX::XMFLOAT4X4					m_matrix;
			std::vector<DebugDrawerLineVertex>	m_line_list;
			size_t								m_line_cnt;
			size_t								m_line_buffer_capacity;

			std::vector<DebugDrawerTextureInstacing>	m_texture_list;
			size_t		m_texture_capacity;
			size_t		m_texture_cnt;

		/* Methods */
			DebugDrawer();
			~DebugDrawer();

			void init(Graphics* graph);
			void shutdown();

			void render(Graphics* graph, GraphicsContext* context);
			void flush();

			void XM_CALLCONV setVPMatrix(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj);
			void setResolution(uint32_t w, uint32_t h);

			void XM_CALLCONV drawLines(DirectX::XMFLOAT3* in_lines, size_t lines_length, DirectX::XMVECTOR color_start, DirectX::XMVECTOR color_end);
			void XM_CALLCONV drawWireCube(DirectX::FXMVECTOR origin, DirectX::FXMVECTOR scale, DirectX::FXMVECTOR pivot, DirectX::FXMVECTOR color);
			void XM_CALLCONV drawTexture(ID3D12Resource* texture, DXGI_FORMAT format, DirectX::FXMVECTOR rect, float z_order);
};