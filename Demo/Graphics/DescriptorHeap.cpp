#include "stdafx.h"
#include "DescriptorHeap.h"

void DescriptorHeap::Initialize(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint32_t numDescriptors, const bool shaderVisible)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = type;
	heapDesc.NumDescriptors = numDescriptors;
	heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heap));
	assert(SUCCEEDED(hr));
	m_cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart().ptr;
	m_gpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart().ptr;
	m_descriptorIncrementSize = device->GetDescriptorHandleIncrementSize(type);
	m_numDescriptors = numDescriptors;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUDescriptorHandleForHeapStart() const
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_gpuHandle;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUDescriptorHandle(const uint32_t descriptorOffset) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_cpuHandle + (m_descriptorIncrementSize * descriptorOffset);
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUDescriptorHandleForHeapStart() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_cpuHandle;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUDescriptorHandle(const uint32_t descriptorOffset) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
	handle.ptr = m_gpuHandle + (m_descriptorIncrementSize * descriptorOffset);
	return handle;
}
