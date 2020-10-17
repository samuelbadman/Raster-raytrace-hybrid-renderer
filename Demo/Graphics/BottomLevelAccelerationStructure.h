#pragma once

#include "../stdafx.h"

class BottomLevelAccelerationStructure
{
public:
	void Initialize(ID3D12Device5* const device, const uint32_t numGeometries);
	void AddStagedGeometry(const D3D12_GPU_VIRTUAL_ADDRESS vbStartAddress, const DXGI_FORMAT positionAttributeFormat,
		const uint32_t vertexStride, const uint32_t vertexCount,
		const D3D12_GPU_VIRTUAL_ADDRESS indexBufferStartAddress, const uint32_t indexCount);
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_blas->GetGPUVirtualAddress(); }
	void CommitStaged(ID3D12GraphicsCommandList4* const commandList);
	void BuildStaged(ID3D12Device5* const device);

private:
	ComPtr<ID3D12Resource> m_blas;
	ComPtr<ID3D12Resource> m_scratch;
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescs;
	uint32_t m_geometryID = 0;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_buildDesc = {};
};
