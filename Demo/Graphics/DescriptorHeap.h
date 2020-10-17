#pragma once

#include "../stdafx.h"

class DescriptorHeap
{
public:
	void Initialize(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t numDescriptors,
		const bool shaderVisible);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const uint32_t descriptorOffset) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const uint32_t descriptorOffset) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const;
	ID3D12DescriptorHeap* GetInterfacePtr() const { return m_heap.Get(); }
	uint32_t GetNumDescriptors() const { return m_numDescriptors; }

private:
	ComPtr<ID3D12DescriptorHeap> m_heap;
	size_t m_cpuHandle = {};
	size_t m_gpuHandle = {};
	uint32_t m_descriptorIncrementSize = 0;
	uint32_t m_numDescriptors = 0;
};