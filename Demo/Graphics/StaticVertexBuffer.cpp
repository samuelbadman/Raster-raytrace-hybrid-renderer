#include "stdafx.h"
#include "StaticVertexBuffer.h"

void StaticVertexBuffer::Initialize(ID3D12Device* const device, const uint32_t size, const uint32_t stride)
{
	DefaultHeap::Initialize(device, CD3DX12_RESOURCE_DESC::Buffer(size), size, size, size);
	m_view.BufferLocation = GetHeapGPUVirtualAddress();
	m_view.SizeInBytes = size;
	m_view.StrideInBytes = stride;
}
