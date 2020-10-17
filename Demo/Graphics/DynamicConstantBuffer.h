#pragma once

class DynamicConstantBuffer
{
public:
	void Initialize(ID3D12Device* const device, const size_t size, const uint32_t backBufferCount,
		const uint32_t numInstancesPerFrame);
	void Update(const uint32_t backBufferIndex,
		const uint32_t instanceIndex, const void* const src, const size_t size);
	D3D12_GPU_VIRTUAL_ADDRESS GetInstanceGPUVirtualAddress(const uint32_t backBufferIndex, const uint32_t instanceIndex);

private:
	ComPtr<ID3D12Resource> m_buffer;
	UINT8* m_gpuAddressForHeapStart = nullptr;
	uint32_t m_instanceAlignedSize = 0;
	uint32_t m_numInstancesPerBackBuffer = 0;
};
