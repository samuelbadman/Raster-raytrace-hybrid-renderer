#include "stdafx.h"
#include "Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/stb_image.h"

void Texture2D::Initialize(ID3D12Device* const device, const std::string& filepath)
{
	int width = 0;
	int height = 0;
	int channels = 0;

	stbi_set_flip_vertically_on_load(true);
	m_data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	if (channels == 3)
	{
		// TODO: Investigate block compression formats to support 24 bit source textures. 
		//       For now only 32 bit textures will be supported, expanding 24 bits into 32 bits.
	}

	assert((width > 0) && (height > 0) && (channels > 0) && m_data);

	m_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	uint32_t bitsPerPixel = 32;
	uint32_t bitsPerChannel = 8;

	uint32_t rowPitch = (bitsPerPixel * width) / bitsPerChannel;
	uint32_t slicePitch = rowPitch * height;

	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(m_format, width, height);
	uint64_t size;
	device->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &size);

	DefaultHeap::Initialize(device, desc, static_cast<uint32_t>(size), rowPitch, slicePitch);
}

void Texture2D::ReleaseData()
{
	stbi_image_free(m_data);
	m_data = nullptr;
}
