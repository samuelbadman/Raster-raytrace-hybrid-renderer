#pragma once

#include "DefaultHeap.h"

class StaticConstantBuffer : public DefaultHeap
{
public:
	void Initialize(ID3D12Device* const device, const uint32_t size);
};