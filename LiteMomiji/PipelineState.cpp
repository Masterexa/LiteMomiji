/* declaration */
#include "PipelineState.hpp"
#include "Graphics.hpp"
namespace{
	using Microsoft::WRL::ComPtr;
}


void PipelineState::init(Graphics* graph, D3D12_ROOT_SIGNATURE_DESC* rs_desc, D3D12_GRAPHICS_PIPELINE_STATE_DESC* ps_desc)
{
	HRESULT	hr;
	m_graphics	= graph;
	auto device	= m_graphics->m_device.Get();

	// Create the root signature
	{
		ComPtr<ID3DBlob>	signature;
		ComPtr<ID3DBlob>	error;
		
		hr = D3D12SerializeRootSignature(
			rs_desc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			signature.GetAddressOf(),
			error.GetAddressOf()
		);
		//auto e = (char*)error->GetBufferPointer();
		THROW_IF_HFAILED(hr, "root signature serialization fail.")


		hr = device->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(m_root_signature.GetAddressOf())
		);
		THROW_IF_HFAILED(hr, "root signature creation fail.")
	}


	// Create the pipeline state object
	{
		ps_desc->pRootSignature = m_root_signature.Get();

		hr = device->CreateGraphicsPipelineState(ps_desc, IID_PPV_ARGS(m_pipeline_state.GetAddressOf()));
		THROW_IF_HFAILED(hr, "PSO creation fail.")
	}
}