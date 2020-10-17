#include "stdafx.h"
#include "Fence.h"

void Fence::Initialize(ID3D12Device* const device, const uint64_t initialValue, const D3D12_FENCE_FLAGS flags)
{
	if (device)
	{
		HRESULT hr = device->CreateFence(initialValue, flags, IID_PPV_ARGS(&m_fence));
		assert(SUCCEEDED(hr));
		m_value = initialValue;
	}
}
