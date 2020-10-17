#pragma once

#include "DefaultHeap.h"

class StaticIndexBuffer : public DefaultHeap
{
public:
	void Initialize(ID3D12Device* const device, const uint32_t size);
	const D3D12_INDEX_BUFFER_VIEW* GetView() const { return &m_view; }

private:
	D3D12_INDEX_BUFFER_VIEW m_view = {};
};