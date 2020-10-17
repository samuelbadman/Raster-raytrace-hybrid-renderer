#include "stdafx.h"
#include "DefaultHeap.h"

void DefaultHeap::Initialize(ID3D12Device* const device, const CD3DX12_RESOURCE_DESC& resourceDesc, const uint32_t size,
	const uint32_t rowPitch, const uint32_t slicePitch)
{
	HRESULT hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_heap));
	assert(SUCCEEDED(hr));

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_intermediate));
	assert(SUCCEEDED(hr));

	m_size = size;
	m_stagedData.RowPitch = rowPitch;
	m_stagedData.SlicePitch = slicePitch;
}

void DefaultHeap::StageData(const void* pData)
{
	m_stagedData.pData = pData;
}

void DefaultHeap::CommitStagedData(ID3D12GraphicsCommandList* const commandList, const D3D12_RESOURCE_STATES finalResourceState)
{
	UpdateSubresources(commandList, m_heap.Get(), m_intermediate.Get(), 0, 0, 1, &m_stagedData);
	// TODO: Nvidia says not to do this. Resource transitions should be grouped and executed in one call to take advantage of 
	//		 driver optimisations. https://developer.nvidia.com/dx12-dos-and-donts
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_heap.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
		finalResourceState));
}

void DefaultHeap::StagingComplete()
{
	m_intermediate.Reset();
	m_stagedData = {};
}
