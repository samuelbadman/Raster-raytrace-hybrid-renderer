#include "stdafx.h"
#include "GraphicsPipelineState.h"

GraphicsPipelineState::GraphicsPipelineState()
{
	// Blend states
	m_opaqueBlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	m_nonPremultipliedAlphaBlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	m_nonPremultipliedAlphaBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	m_nonPremultipliedAlphaBlendDesc.RenderTarget[0].SrcBlend =
		m_nonPremultipliedAlphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	m_nonPremultipliedAlphaBlendDesc.RenderTarget[0].DestBlend =
		m_nonPremultipliedAlphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

	// Depth stencil states
	m_depthNoneDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	m_depthNoneDepthStencilDesc.DepthEnable = FALSE;
	m_depthNoneDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	m_depthNoneDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	m_depthDefaultDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	m_depthDefaultDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// Rasterizer states
	m_noCullingRasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE,
		FALSE,
		D3D12_DEFAULT_DEPTH_BIAS,
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		TRUE,
		TRUE,
		FALSE,
		0,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);

	m_backCullingRasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK,
		FALSE,
		D3D12_DEFAULT_DEPTH_BIAS,
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		TRUE,
		TRUE,
		FALSE,
		0,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);

	m_wireframeRasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_FILL_MODE_WIREFRAME, D3D12_CULL_MODE_NONE,
		FALSE,
		D3D12_DEFAULT_DEPTH_BIAS,
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		TRUE,
		TRUE,
		FALSE,
		0,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
}

void GraphicsPipelineState::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& inputLayout)
{
	m_pipelineDesc.InputLayout = inputLayout;
}

void GraphicsPipelineState::SetRootSignature(ID3D12RootSignature* const rootSignature)
{
	m_pipelineDesc.pRootSignature = rootSignature;
}

void GraphicsPipelineState::SetVertexShader(const size_t bytecodeLength, const void* const pShaderBytecode)
{
	m_pipelineDesc.VS.BytecodeLength = bytecodeLength;
	m_pipelineDesc.VS.pShaderBytecode = pShaderBytecode;
}

void GraphicsPipelineState::SetPixelShader(const size_t bytecodeLength, const void* pShaderBytecode)
{
	m_pipelineDesc.PS.BytecodeLength = bytecodeLength;
	m_pipelineDesc.PS.pShaderBytecode = pShaderBytecode;
}

void GraphicsPipelineState::SetPrimitiveTopologyType(const D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType)
{
	m_pipelineDesc.PrimitiveTopologyType = primitiveTopologyType;
}

void GraphicsPipelineState::SetRTVFormats(const uint32_t numRenderTargets, const DXGI_FORMAT format)
{
	for (uint32_t i = 0; i < numRenderTargets; i++)
	{
		m_pipelineDesc.RTVFormats[i] = format;
	}
}

void GraphicsPipelineState::SetSampleDesc(const DXGI_SAMPLE_DESC& sampleDesc)
{
	m_pipelineDesc.SampleDesc = sampleDesc;
}

void GraphicsPipelineState::SetSampleMask(const uint32_t mask)
{
	m_pipelineDesc.SampleMask = mask;
}

void GraphicsPipelineState::SetRasterizerState(const RasterizerState state)
{
	switch (state)
	{
	case RasterizerState::NoCulling: m_pipelineDesc.RasterizerState = m_noCullingRasterizerDesc; return;
	case RasterizerState::BackCulling: m_pipelineDesc.RasterizerState = m_backCullingRasterizerDesc; return;
	case RasterizerState::Wireframe: m_pipelineDesc.RasterizerState = m_wireframeRasterizerDesc; return;
	}
}

void GraphicsPipelineState::SetBlendState(const BlendState state)
{
	switch (state)
	{
	case BlendState::Opaque: m_pipelineDesc.BlendState = m_opaqueBlendDesc; return;
	case BlendState::NonPremultiplied: m_pipelineDesc.BlendState = m_nonPremultipliedAlphaBlendDesc; return;
	}
}

void GraphicsPipelineState::SetDepthStencilState(const DepthStencilState state)
{
	switch (state)
	{
	case DepthStencilState::None: m_pipelineDesc.DepthStencilState = m_depthNoneDepthStencilDesc; return;
	case DepthStencilState::Default: m_pipelineDesc.DepthStencilState = m_depthDefaultDepthStencilDesc; return;
	}
}

void GraphicsPipelineState::SetDSVFormat(const DXGI_FORMAT format)
{
	m_pipelineDesc.DSVFormat = format;
}

void GraphicsPipelineState::SetNumRenderTargets(const uint32_t numRenderTargets)
{
	m_pipelineDesc.NumRenderTargets = numRenderTargets;
}

void GraphicsPipelineState::Create(ID3D12Device* const device)
{
	HRESULT hr = device->CreateGraphicsPipelineState(&m_pipelineDesc, IID_PPV_ARGS(&m_pipelineState));
	assert(SUCCEEDED(hr));
}