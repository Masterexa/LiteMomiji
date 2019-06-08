/* include */
#include "DebugDrawer.hpp"

/* include */
#include "Graphics.hpp"
#include "GraphicsContext.hpp"
#include "PipelineState.hpp"
namespace{
	using namespace DirectX;
	using namespace DirectX::PackedVector;
	using Microsoft::WRL::ComPtr;
}

const D3D12_INPUT_ELEMENT_DESC LINE_INPUT_ELEMENTS[] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};
const size_t LINE_INPUT_LENGTH = sizeof(LINE_INPUT_ELEMENTS)/sizeof(D3D12_INPUT_ELEMENT_DESC);


DebugDrawer::DebugDrawer()
{
	m_initialized = false;
	XMStoreFloat4x4(&m_matrix, XMMatrixIdentity());
	m_line_cnt = 0;
}
DebugDrawer::~DebugDrawer()
{
	shutdown();
}

void DebugDrawer::init(Graphics* graph)
{
	if(m_initialized)
	{
		return;
	}
	HRESULT hr;
	auto device = graph->m_device.Get();

	// Create the buffer
	{
		D3D12_HEAP_PROPERTIES	prop ={};
		prop.Type					= D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask		= 1;
		prop.VisibleNodeMask		= 1;

		D3D12_RESOURCE_DESC desc ={};
		desc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment			= 0;
		desc.Width				= 1024*64;
		desc.Height				= 1;
		desc.DepthOrArraySize	= 1;
		desc.MipLevels			= 1;
		desc.Format				= DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count	= 1;
		desc.SampleDesc.Quality	= 0;
		desc.Flags				= D3D12_RESOURCE_FLAG_NONE;
		desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_buffer.GetAddressOf())
		);
		THROW_IF_HFAILED(hr, "DebugDrawer : Buffer creation fail.")

		auto addr_gpu = m_buffer->GetGPUVirtualAddress();

		m_line_view[0].BufferLocation	= addr_gpu;
		m_line_view[0].StrideInBytes	= sizeof(DebugDrawerLineVertex);
		m_line_view[0].SizeInBytes		= desc.Width;

		m_line_buffer_capacity = m_line_view[0].SizeInBytes/sizeof(DebugDrawerLineVertex);
		m_line_list.reserve(m_line_buffer_capacity);
		m_line_list.resize(m_line_buffer_capacity);

		m_buffer->Map(0, nullptr, (void**)&m_buffer_mapped);
		m_buffer_line_start	= m_buffer_mapped;
	}


	// Create PSO
	{
		D3D12_ROOT_PARAMETER rs_param;
		rs_param.ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rs_param.ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;
		rs_param.Constants.Num32BitValues	= sizeof(DebugDrawerShaderUniform)/4;
		rs_param.Constants.RegisterSpace	= 0;
		rs_param.Constants.ShaderRegister	= 0;

		D3D12_ROOT_SIGNATURE_DESC	rs_desc={};
		rs_desc.NumParameters		= 1;
		rs_desc.pParameters			= &rs_param;
		rs_desc.NumStaticSamplers	= 0;
		rs_desc.pStaticSamplers		= nullptr;
		rs_desc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		;


		ComPtr<ID3DBlob>	vs_blob;
		ComPtr<ID3DBlob>	ps_blob;

		hr = D3DReadFileToBlob(L"GameResources/shaders/DebugDrawer_Line_vs.cso", vs_blob.GetAddressOf());
		THROW_IF_HFAILED(hr, "Can not open the vertex shader file.")

			hr = D3DReadFileToBlob(L"GameResources/shaders/DebugDrawer_Line_ps.cso", ps_blob.GetAddressOf());
		THROW_IF_HFAILED(hr, "Can not open the pixel shader file.")


		// rasterizer
		D3D12_RASTERIZER_DESC	raster;
		raster.FillMode					= D3D12_FILL_MODE_WIREFRAME;
		raster.CullMode					= D3D12_CULL_MODE_NONE;
		raster.FrontCounterClockwise	= TRUE;
		raster.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
		raster.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		raster.SlopeScaledDepthBias		= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		raster.DepthClipEnable			= TRUE;
		raster.MultisampleEnable		= FALSE;
		raster.AntialiasedLineEnable	= FALSE;
		raster.ForcedSampleCount		= 0;
		raster.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// blending each target
		D3D12_RENDER_TARGET_BLEND_DESC tag_blend={
			TRUE, FALSE,
			D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL
		};

		// blending
		D3D12_BLEND_DESC blend;
		blend.AlphaToCoverageEnable		= FALSE;
		blend.IndependentBlendEnable	= FALSE;
		for(auto& it : blend.RenderTarget)
		{
			it = tag_blend;
		}

		// PIPELINE STATE
		D3D12_GRAPHICS_PIPELINE_STATE_DESC	ps_desc ={};
		ZeroMemory(&ps_desc, sizeof(ps_desc));
		ps_desc.InputLayout						={LINE_INPUT_ELEMENTS, LINE_INPUT_LENGTH};
		ps_desc.pRootSignature					= nullptr;
		ps_desc.VS								={reinterpret_cast<UINT8*>(vs_blob->GetBufferPointer()), vs_blob->GetBufferSize()};
		ps_desc.PS								={reinterpret_cast<UINT8*>(ps_blob->GetBufferPointer()), ps_blob->GetBufferSize()};
		ps_desc.RasterizerState					= raster;
		ps_desc.BlendState						= blend;
		ps_desc.DepthStencilState.DepthEnable	= TRUE;
		ps_desc.DepthStencilState.StencilEnable	= FALSE;
		ps_desc.DepthStencilState.DepthFunc		= D3D12_COMPARISON_FUNC_LESS;
		ps_desc.DepthStencilState.DepthWriteMask= D3D12_DEPTH_WRITE_MASK_ALL;
		ps_desc.SampleMask						= UINT_MAX;
		ps_desc.PrimitiveTopologyType			= D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		ps_desc.NumRenderTargets				= 1;
		ps_desc.RTVFormats[0]					= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		ps_desc.DSVFormat						= DXGI_FORMAT_D24_UNORM_S8_UINT;
		ps_desc.SampleDesc.Count				= 1;

		m_pso_line.reset(new PipelineState());
		m_pso_line->init(graph, &rs_desc, &ps_desc);
	}
	m_initialized = true;
}

void DebugDrawer::shutdown()
{
	if(!m_initialized)
	{
		return;
	}
	m_pso_line.reset();

	m_buffer->Unmap(0,nullptr);
	m_buffer.Reset();

	m_initialized = false;
}

void DebugDrawer::render(Graphics* graph, GraphicsContext* context)
{
	auto cmd_queue	= graph->m_cmd_queue.Get();
	auto cmd_list	= context->m_cmd_list.Get();

	UINT inst_cnt = m_line_cnt;
	if(inst_cnt<1)
	{
		return;
	}
	memcpy(m_buffer_line_start, m_line_list.data(), sizeof(DebugDrawerLineVertex)*inst_cnt);

	context->begin();
	{
		context->setPipelineState(m_pso_line.get());
		
		cmd_list->SetGraphicsRoot32BitConstants(0, sizeof(DebugDrawerShaderUniform)/4, &m_shader_uniform, 0);
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		cmd_list->IASetVertexBuffers(0, 1, m_line_view);
		cmd_list->DrawInstanced(inst_cnt, 1, 0, 0);
	}
	context->end();

	ID3D12CommandList* cmd_lists[] ={cmd_list};
	cmd_queue->ExecuteCommandLists(1, cmd_lists);

	graph->waitForDone();
}

void DebugDrawer::flush()
{
	m_line_cnt=0;
}

void XM_CALLCONV DebugDrawer::setVPMatrix(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj)
{
	XMStoreFloat4x4(&m_shader_uniform.view_matrix, view);
	XMStoreFloat4x4(&m_shader_uniform.projection_matrix, proj);
}

void XM_CALLCONV DebugDrawer::drawLines(DirectX::XMFLOAT3* in_lines, size_t lines_length, DirectX::XMVECTOR color_start, DirectX::XMVECTOR color_end)
{
	if(m_line_cnt+lines_length*2 >= m_line_buffer_capacity)
	{
		return;
	}

	auto mat = XMLoadFloat4x4(&m_matrix);

	for(size_t i=0,end=lines_length-1; i<end; i++)
	{
		DebugDrawerLineVertex v0,v1;

		XMStoreFloat3(&v0.position, XMVector3Transform(XMLoadFloat3(&in_lines[i]),mat));
		PackedVector::XMStoreColor(&v0.color, XMVectorLerp(color_start, color_end, (float)i/(float)lines_length));

		XMStoreFloat3(&v1.position, XMVector3Transform(XMLoadFloat3(&in_lines[i+1]),mat));
		PackedVector::XMStoreColor(&v1.color, XMVectorLerp(color_start, color_end, (float)(i+1)/(float)lines_length));

		m_line_list[m_line_cnt++] = v0;
		m_line_list[m_line_cnt++] = v1;
	}
}

void XM_CALLCONV DebugDrawer::drawWireCube(DirectX::FXMVECTOR origin, DirectX::FXMVECTOR scale, DirectX::FXMVECTOR pivot, DirectX::FXMVECTOR color)
{
	if(m_line_cnt+24>=m_line_buffer_capacity)
	{
		return;
	}
	// 0,1,2,3 4,5,6,7
	XMVECTOR zero	= g_XMZero;
	XMVECTOR one	= g_XMOne;

	zero	= XMVectorMultiplyAdd(XMVectorSubtract(zero,pivot), scale, origin);
	one		= XMVectorMultiplyAdd(XMVectorSubtract(one,pivot), scale, origin);
	
	{
		auto mat	= XMLoadFloat4x4(&m_matrix);
		zero		= XMVector3Transform(zero, mat);
		one			= XMVector3Transform(one, mat);
	}
	DebugDrawerLineVertex v;
	XMStoreColor(&v.color, color);
	

	// bottom-back
	XMStoreFloat3(&v.position, zero); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<4,1,2,3>(zero,one) ); m_line_list[m_line_cnt++] = v;

	// bottom-front
	XMStoreFloat3(&v.position, XMVectorPermute<0, 1, 6, 3>(zero,one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<4, 1, 6, 3>(zero,one)); m_line_list[m_line_cnt++] = v;

	// bottom-left
	XMStoreFloat3(&v.position, zero); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<0, 1, 6, 3>(zero, one)); m_line_list[m_line_cnt++] = v;

	// bottom-right
	XMStoreFloat3(&v.position, XMVectorPermute<4, 1, 2, 3>(zero, one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<4, 1, 6, 3>(zero, one)); m_line_list[m_line_cnt++] = v;


	// top-back
	XMStoreFloat3(&v.position, XMVectorPermute<0, 5, 2, 3>(zero, one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<4, 5, 2, 3>(zero, one)); m_line_list[m_line_cnt++] = v;

	// top-front
	XMStoreFloat3(&v.position, XMVectorPermute<0,5,6,3>(zero, one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, one); m_line_list[m_line_cnt++] = v;

	// top-left
	XMStoreFloat3(&v.position, XMVectorPermute<0,5,2,3>(zero, one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<0,5,6,3>(zero, one)); m_line_list[m_line_cnt++] = v;

	// top-right
	XMStoreFloat3(&v.position, XMVectorPermute<4, 5, 2, 3>(zero, one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, one); m_line_list[m_line_cnt++] = v;


	// middle-BL
	XMStoreFloat3(&v.position, zero); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<0,5,2,3>(zero, one)); m_line_list[m_line_cnt++] = v;

	// middle-BR
	XMStoreFloat3(&v.position, XMVectorPermute<4,1,2,3>(zero, one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<4,5,2,3>(zero, one)); m_line_list[m_line_cnt++] = v;

	// middle-TL
	XMStoreFloat3(&v.position, XMVectorPermute<0,1,6,3>(zero, one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<0,5,6,3>(zero, one)); m_line_list[m_line_cnt++] = v;

	// middle-TR
	XMStoreFloat3(&v.position, XMVectorPermute<4,1,6,3>(zero, one)); m_line_list[m_line_cnt++] = v;
	XMStoreFloat3(&v.position, XMVectorPermute<4,5,6,3>(zero, one)); m_line_list[m_line_cnt++] = v;
}