#pragma once

class TopLevelAccelerationStructure
{
public:
	void Initialize(ID3D12Device* const device, const uint32_t numInstances, const bool supportsUpdate);
	void SetInstance(const uint32_t instanceID, const uint32_t instanceContributionToHitGroupIndex, const XMMATRIX& transform,
		const uint32_t instanceMask, const uint32_t instanceFlags,
		const D3D12_GPU_VIRTUAL_ADDRESS bottomLevelAccelerationStructureAddress);
	void Stage(ID3D12Device5* const device);
	void Commit(ID3D12GraphicsCommandList4* const commandList);
	void Update(ID3D12Device5* const device, ID3D12GraphicsCommandList4* const commandList);
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_tlas->GetGPUVirtualAddress(); }
	ID3D12Resource* GetResource() const { return m_tlas.Get(); }

private:
	ComPtr<ID3D12Resource> m_tlas;
	ComPtr<ID3D12Resource> m_scratch;
	ComPtr<ID3D12Resource> m_instancesBuffer;
	uint32_t m_maxInstances = 0;
	D3D12_RAYTRACING_INSTANCE_DESC* m_pInstanceDescs;
	bool m_supportsUpdate = false;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_desc = {};
};
