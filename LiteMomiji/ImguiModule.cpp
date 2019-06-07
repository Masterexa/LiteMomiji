/* declarations */
#include "ImguiModule.hpp"

/* includes */
#include "Graphics.hpp"
#include "GraphicsContext.hpp"

/* imgui */
#include <imgui.h>
#include <examples/imgui_impl_win32.h>
#include <examples/imgui_impl_dx12.h>

ImguiModule::ImguiModule()
{
	m_initialized = false;
}

ImguiModule::~ImguiModule()
{
	shutdown();
}

void ImguiModule::init(Graphics* graph, HWND hwnd, uint32_t backbuffer_cnt, DXGI_FORMAT backbuffer_format)
{
	auto device = graph->m_device.Get();

	// setup imgui
	if(m_initialized)return;
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& io = ImGui::GetIO(); (void)io;
	ImFontConfig	config; config.MergeMode = true;
	io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\meiryo.ttc", 18.f, &config, io.Fonts->GetGlyphRangesJapanese());

	// select style
	ImGui::StyleColorsClassic();

	// setup platform bindings
	{
		// create the Descriptor heap for imgui
		D3D12_DESCRIPTOR_HEAP_DESC		dheap_desc={};
		dheap_desc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		dheap_desc.NumDescriptors	= 1;
		dheap_desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		dheap_desc.NodeMask			= 1;
		
		HRESULT hr = device->CreateDescriptorHeap(&dheap_desc, IID_PPV_ARGS(m_dheap.GetAddressOf()));
		THROW_IF_HFAILED(hr, "ImguiModule : DescriptorHeap creation fail.")

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX12_Init(device, backbuffer_cnt, backbuffer_format, m_dheap->GetCPUDescriptorHandleForHeapStart(), m_dheap->GetGPUDescriptorHandleForHeapStart());
	}
	m_initialized = true;
}

void ImguiModule::shutdown()
{
	if(!m_initialized)
	{
		return;
	}
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	m_dheap.Reset();

	m_initialized = false;
}

void ImguiModule::newFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImguiModule::render(GraphicsContext* context)
{
	context->m_cmd_list->SetDescriptorHeaps(1, m_dheap.GetAddressOf());
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context->m_cmd_list.Get());
}

void ImguiModule::resizeBegin()
{
	ImGui_ImplDX12_InvalidateDeviceObjects();
}

void ImguiModule::resizeEnd()
{
	ImGui_ImplDX12_CreateDeviceObjects();
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT ImguiModule::sendWindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
}