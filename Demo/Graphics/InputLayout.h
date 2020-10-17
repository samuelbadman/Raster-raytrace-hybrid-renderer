#pragma once

#include "../stdafx.h"

class InputLayout
{
public:
	void AddInputElement(const D3D12_INPUT_ELEMENT_DESC& element);
	void AddInputElement(const LPCSTR semanticName, const UINT semanticIndex, const DXGI_FORMAT format,
		const UINT inputSlot, const UINT alignedByteOffset, const D3D12_INPUT_CLASSIFICATION inputSlotClass,
		const UINT instanceDataStepRate);
	void Create();
	const D3D12_INPUT_LAYOUT_DESC& GetInterfacePtr() const { return m_description; }

private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayoutElements;
	D3D12_INPUT_LAYOUT_DESC m_description = {};
};
