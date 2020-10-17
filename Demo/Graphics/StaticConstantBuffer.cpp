#include "stdafx.h"
#include "StaticConstantBuffer.h"

void StaticConstantBuffer::Initialize(ID3D12Device* const device, const uint32_t size)
{
	DefaultHeap::Initialize(device, CD3DX12_RESOURCE_DESC::Buffer(size), size, size, size);
}
