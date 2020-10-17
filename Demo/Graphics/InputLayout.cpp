#include "stdafx.h"
#include "InputLayout.h"

void InputLayout::AddInputElement(const D3D12_INPUT_ELEMENT_DESC& element)
{
	m_inputLayoutElements.push_back(element);
}

void InputLayout::AddInputElement(const LPCSTR semanticName, const UINT semanticIndex, const DXGI_FORMAT format,
	const UINT inputSlot, const UINT alignedByteOffset, const D3D12_INPUT_CLASSIFICATION inputSlotClass,
	const UINT instanceDataStepRate)
{
	m_inputLayoutElements.push_back(
		{ semanticName, semanticIndex, format, inputSlot, alignedByteOffset, inputSlotClass, instanceDataStepRate }
	);
}

void InputLayout::Create()
{
	m_description.NumElements = static_cast<uint32_t>(m_inputLayoutElements.size());
	m_description.pInputElementDescs = m_inputLayoutElements.data();
}