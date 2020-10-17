#pragma once

#include "../stdafx.h"

class Fence
{
public:
	void Initialize(ID3D12Device* const device, const uint64_t initialValue, const D3D12_FENCE_FLAGS flags);
	uint64_t& Value() { return m_value; }
	ID3D12Fence* GetInterfacePtr() const { return m_fence.Get(); }

private:
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_value = 0;
};