#pragma once

#include "DefaultHeap.h"

class Texture2D : public DefaultHeap
{
public:
	void Initialize(ID3D12Device* const device, const std::string& filepath);
	void ReleaseData();
	unsigned char* GetData() const { return m_data; }
	DXGI_FORMAT GetFormat() const { return m_format; }

private:
	unsigned char* m_data = nullptr;
	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
};