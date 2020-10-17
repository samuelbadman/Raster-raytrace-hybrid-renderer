#pragma once

#include "../stdafx.h"

enum class RasterizerState : uint8_t
{
	NoCulling = 0,
	BackCulling,
	Wireframe
};

enum class BlendState : uint8_t
{
	Opaque = 0,
	NonPremultiplied
};

enum class DepthStencilState : uint8_t
{
	None = 0,
	Default
};

class InputLayout;
class RootSignature;

class GraphicsPipelineState
{
public:
	GraphicsPipelineState();

	void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& inputLayout);
	void SetRootSignature(ID3D12RootSignature* const rootSignature);
	void SetVertexShader(const size_t bytecodeLength, const void* const pShaderBytecode);
	void SetPixelShader(const size_t bytecodeLength, const void* const pShaderBytecode);
	void SetPrimitiveTopologyType(const D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType);
	void SetRTVFormats(const uint32_t numRenderTargets, const DXGI_FORMAT format);
	void SetSampleDesc(const DXGI_SAMPLE_DESC& sampleDesc);
	void SetSampleMask(const uint32_t mask);
	void SetRasterizerState(const RasterizerState state);
	void SetBlendState(const BlendState state);
	void SetDepthStencilState(const DepthStencilState state);
	void SetDSVFormat(const DXGI_FORMAT format);
	void SetNumRenderTargets(const uint32_t numRenderTargets);
	void Create(ID3D12Device* const device);
	ID3D12PipelineState* Get() const { return m_pipelineState.Get(); }

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipelineDesc = {};
	ComPtr<ID3D12PipelineState> m_pipelineState;

	CD3DX12_BLEND_DESC m_opaqueBlendDesc;
	CD3DX12_BLEND_DESC m_nonPremultipliedAlphaBlendDesc;

	CD3DX12_DEPTH_STENCIL_DESC m_depthNoneDepthStencilDesc;
	CD3DX12_DEPTH_STENCIL_DESC m_depthDefaultDepthStencilDesc;

	CD3DX12_RASTERIZER_DESC m_noCullingRasterizerDesc;
	CD3DX12_RASTERIZER_DESC m_backCullingRasterizerDesc;
	CD3DX12_RASTERIZER_DESC m_wireframeRasterizerDesc;
};