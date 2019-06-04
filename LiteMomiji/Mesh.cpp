#include "Mesh.hpp"
#include "Graphics.hpp"
namespace{
	using namespace DirectX;
}

bool XM_CALLCONV vertexFillQuad(Vertex* p_first, size_t max_num, size_t offset, XMVECTOR N, XMVECTOR B)
{
	if(offset+3>=max_num)
	{
		return false;
	}
	auto T = XMVector3Cross(B, N);
	p_first += offset;

	for(size_t i=0; i<4; i++)
	{
		XMStoreFloat3(&p_first[i].normal, N);
		XMStoreFloat3(&p_first[i].tangent, T);
		XMStoreFloat4(&p_first[i].color, XMVectorSet(1, 1, 1, 1));
	}
	XMStoreFloat3(&p_first[0].position, XMVectorAdd(N, XMVectorAdd(B, T)));
	XMStoreFloat3(&p_first[1].position, XMVectorAdd(N, XMVectorSubtract(T, B)));
	XMStoreFloat3(&p_first[2].position, XMVectorAdd(N, XMVectorSubtract(XMVectorNegate(B), T)));
	XMStoreFloat3(&p_first[3].position, XMVectorAdd(N, XMVectorSubtract(B, T)));

	return true;
}



HRESULT Mesh::init(Graphics* graphics, MeshInitDesc* p_desc)
{
	HRESULT	hr;

	auto	p_device		= graphics->m_device.Get();
	UINT64	vertex_size		= sizeof(Vertex)*p_desc->verts_count;
	auto	index_offset	= getAlignedSize<UINT64>(vertex_size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	UINT64	index_size		= sizeof(uint32_t)*p_desc->indices_count;
	auto	heap_size		= getAlignedSize<UINT64>(index_offset+index_size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);


	// create the heap for buffers
	{
		D3D12_HEAP_DESC	desc={};
		desc.SizeInBytes	= heap_size;
		desc.Alignment		= 0;
		desc.Flags			= D3D12_HEAP_FLAG_NONE;

		auto& prop = desc.Properties;
		prop.Type					= D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask		= 1;
		prop.VisibleNodeMask		= 1;

		hr = p_device->CreateHeap(&desc, IID_PPV_ARGS(m_mesh_heap.GetAddressOf()));
		if(FAILED(hr))
		{
			return hr;
		}
	}

	// create buffers
	{
		D3D12_RESOURCE_DESC	desc={};
		desc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment			= 0;
		desc.Width				= vertex_size;
		desc.Height				= 1;
		desc.DepthOrArraySize	= 1;
		desc.MipLevels			= 1;
		desc.Format				= DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count	= 1;
		desc.SampleDesc.Quality	= 0;
		desc.Flags				= D3D12_RESOURCE_FLAG_NONE;
		desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		// create vertex buffer
		hr = p_device->CreatePlacedResource(
			m_mesh_heap.Get(),
			0,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_vertex_buffer.GetAddressOf())
		);
		if(FAILED(hr))
		{
			return hr;
		}

		// create index buffer
		desc.Width		= index_size;
		hr = p_device->CreatePlacedResource(
			m_mesh_heap.Get(),
			index_offset,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_index_buffer.GetAddressOf())
		);
		if(FAILED(hr))
		{
			return hr;
		}
	}

	// copy resources
	{
		D3D12_SUBRESOURCE_DATA	subres={};
		subres.pData		= p_desc->p_verts;
		subres.RowPitch		= 0;
		subres.SlicePitch	= 0;

		// copy vertices to GPU
		auto rt = graphics->updateSubresources(m_vertex_buffer.Get(), 0, 1, &subres);
		if(!rt)
		{
			return E_FAIL;
		}

		subres.pData = p_desc->p_indices;
		rt = graphics->updateSubresources(m_index_buffer.Get(), 0, 1, &subres);
		if(!rt)
		{
			return E_FAIL;
		}
	}

	// create views
	{
		m_vb_view.BufferLocation	= m_vertex_buffer->GetGPUVirtualAddress();
		m_vb_view.StrideInBytes		= sizeof(Vertex);
		m_vb_view.SizeInBytes		= sizeof(Vertex)*p_desc->verts_count;

		m_ib_view.BufferLocation	= m_index_buffer->GetGPUVirtualAddress();
		m_ib_view.Format			= DXGI_FORMAT_R32_UINT;
		m_ib_view.SizeInBytes		= sizeof(uint32_t)*p_desc->indices_count;
	}

	// setup submeshes
	{
		auto cnt = 1 + ((p_desc->p_submesh_pairs && p_desc->submesh_pairs_count) ? (p_desc->submesh_pairs_count) : (0));

		m_submesh_pairs.reserve(cnt);
		m_submesh_pairs.resize(cnt);

		m_submesh_pairs[0].offset	= 0;
		m_submesh_pairs[0].count	= p_desc->indices_count;

		for(size_t i=1; i<cnt; i++)
		{
			m_submesh_pairs[i] = p_desc->p_submesh_pairs[i-1];
		}
	}
}