#pragma once

#include "pre.hpp"
#include <vector>

struct IndexPair
{
	uint32_t	offset;
	uint32_t	count;
};

struct ModelDesc
{
	Vertex*		p_verts;
	size_t		verts_count;
	uint32_t*	p_indices;
	size_t		indices_count;
	IndexPair*	p_submesh_pairs;
	size_t		submesh_pairs_count;
};

struct Model{

	/* Instance */
		/* Fields */
			Microsoft::WRL::ComPtr<ID3D12Heap>		m_model_heap;
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_vertex_buffer;
			Microsoft::WRL::ComPtr<ID3D12Resource>	m_index_buffer;
			D3D12_VERTEX_BUFFER_VIEW	m_vb_view;
			D3D12_INDEX_BUFFER_VIEW		m_ib_view;
			std::vector<IndexPair>		m_submesh_pairs;

		/* Inits */
			HRESULT init(Graphics* graphics, ModelDesc* p_desc);
};


bool XM_CALLCONV vertexFillQuad(Vertex* p_first, size_t max_num, size_t offset, DirectX::XMVECTOR N, DirectX::XMVECTOR B);