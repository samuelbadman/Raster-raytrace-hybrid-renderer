#pragma once

#include "DefaultHeap.h"

class StaticVertexBuffer : public DefaultHeap
{
public:
	void Initialize(ID3D12Device* const device, const uint32_t size, const uint32_t stride);
	const D3D12_VERTEX_BUFFER_VIEW* GetView() const { return &m_view; }

private:
	D3D12_VERTEX_BUFFER_VIEW m_view = {};
};