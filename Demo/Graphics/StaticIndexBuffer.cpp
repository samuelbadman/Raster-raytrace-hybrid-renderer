#include "stdafx.h"
#include "StaticIndexBuffer.h"

void StaticIndexBuffer::Initialize(ID3D12Device* const device, const uint32_t size)
{
	DefaultHeap::Initialize(device, CD3DX12_RESOURCE_DESC::Buffer(size), size, size, size);
	m_view.BufferLocation = GetHeapGPUVirtualAddress();
	m_view.Format = DXGI_FORMAT_R32_UINT;
	m_view.SizeInBytes = size;
}
