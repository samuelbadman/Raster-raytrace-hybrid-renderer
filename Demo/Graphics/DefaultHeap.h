#pragma once

#include "../stdafx.h"

class DefaultHeap
{
public:
	void StageData(const void* pData);
	void CommitStagedData(ID3D12GraphicsCommandList* const commandList, const D3D12_RESOURCE_STATES finalResourceState);
	void StagingComplete();
	D3D12_GPU_VIRTUAL_ADDRESS GetHeapGPUVirtualAddress() const { return m_heap->GetGPUVirtualAddress(); }
	uint32_t GetSize() const { return m_size; }
	ID3D12Resource* GetResource() const { return m_heap.Get(); }

protected:
	void Initialize(ID3D12Device* const device, const CD3DX12_RESOURCE_DESC& resourceDesc, const uint32_t size,
		const uint32_t rowPitch, const uint32_t slicePitch);

private:
	ComPtr<ID3D12Resource> m_heap;
	ComPtr<ID3D12Resource> m_intermediate;
	uint32_t m_size = 0;
	D3D12_SUBRESOURCE_DATA m_stagedData = {};
};