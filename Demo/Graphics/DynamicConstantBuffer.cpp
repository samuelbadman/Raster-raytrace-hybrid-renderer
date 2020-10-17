#include "stdafx.h"
#include "DynamicConstantBuffer.h"
#include "../Macros.h"

void DynamicConstantBuffer::Initialize(ID3D12Device* const device, const size_t size, const uint32_t backBufferCount, 
	const uint32_t numInstancesPerFrame)
{
	uint32_t bufferSize = static_cast<uint32_t>(size) * numInstancesPerFrame * backBufferCount;
	CD3DX12_RANGE readRange(0, 0);
	HRESULT hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_buffer));
	assert(SUCCEEDED(hr));
	hr = m_buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_gpuAddressForHeapStart));
	assert(SUCCEEDED(hr));
	m_instanceAlignedSize = ALIGN_TO(size, 256);
	m_numInstancesPerBackBuffer = numInstancesPerFrame;
}

void DynamicConstantBuffer::Update(const uint32_t backBufferIndex,
	const uint32_t instanceIndex, const void* const src, const size_t size)
{
	memcpy(m_gpuAddressForHeapStart +
		(m_instanceAlignedSize * backBufferIndex * m_numInstancesPerBackBuffer) +
		(m_instanceAlignedSize * instanceIndex),
		src, size);
}

D3D12_GPU_VIRTUAL_ADDRESS DynamicConstantBuffer::GetInstanceGPUVirtualAddress(const uint32_t backBufferIndex,
	const uint32_t instanceIndex)
{
	return m_buffer->GetGPUVirtualAddress() + (m_instanceAlignedSize * backBufferIndex * m_numInstancesPerBackBuffer) +
		(m_instanceAlignedSize * instanceIndex);
}