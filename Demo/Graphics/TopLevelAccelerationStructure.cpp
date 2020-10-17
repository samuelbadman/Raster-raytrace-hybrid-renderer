#include "stdafx.h"
#include "TopLevelAccelerationStructure.h"

void TopLevelAccelerationStructure::Initialize(ID3D12Device* const device, const uint32_t numInstances, const bool supportsUpdate)
{
	m_maxInstances = numInstances;
	m_supportsUpdate = supportsUpdate;
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances);
	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_instancesBuffer));
	HRESULT hr = m_instancesBuffer->Map(0, nullptr, (void**)&m_pInstanceDescs);
	assert(SUCCEEDED(hr));
}

void TopLevelAccelerationStructure::SetInstance(const uint32_t instanceID,
	const uint32_t instanceContributionToHitGroupIndex, const XMMATRIX& transform,
	const uint32_t instanceMask, const uint32_t instanceFlags, const D3D12_GPU_VIRTUAL_ADDRESS bottomLevelAccelerationStructureAddress)
{
	assert(instanceID < m_maxInstances);
	m_pInstanceDescs[instanceID].InstanceID = instanceID;
	m_pInstanceDescs[instanceID].InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex;
	m_pInstanceDescs[instanceID].Flags = instanceFlags;
	memcpy(m_pInstanceDescs[instanceID].Transform, &XMMatrixTranspose(transform), sizeof(m_pInstanceDescs[instanceID].Transform));
	m_pInstanceDescs[instanceID].AccelerationStructure = bottomLevelAccelerationStructureAddress;
	m_pInstanceDescs[instanceID].InstanceMask = instanceMask;
}

void TopLevelAccelerationStructure::Stage(ID3D12Device5* const device)
{
	m_instancesBuffer->Unmap(0, nullptr);
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = m_supportsUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
		: D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = m_maxInstances;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	auto scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&scratchDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&m_scratch)
	);

	auto resultDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resultDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(&m_tlas));

	m_desc.Inputs = inputs;
	m_desc.Inputs.InstanceDescs = m_instancesBuffer->GetGPUVirtualAddress();
	m_desc.DestAccelerationStructureData = m_tlas->GetGPUVirtualAddress();
	m_desc.ScratchAccelerationStructureData = m_scratch->GetGPUVirtualAddress();
}

void TopLevelAccelerationStructure::Commit(ID3D12GraphicsCommandList4* const commandList)
{
	commandList->BuildRaytracingAccelerationStructure(&m_desc, 0, nullptr);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_tlas.Get()));
}

void TopLevelAccelerationStructure::Update(ID3D12Device5* const device, ID3D12GraphicsCommandList4* const commandList)
{
	assert(m_supportsUpdate);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.NumDescs = m_maxInstances;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	m_desc.Inputs = inputs;
	m_desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	m_desc.SourceAccelerationStructureData = m_tlas->GetGPUVirtualAddress();
	m_desc.Inputs.InstanceDescs = m_instancesBuffer->GetGPUVirtualAddress();
	m_desc.DestAccelerationStructureData = m_tlas->GetGPUVirtualAddress();
	m_desc.ScratchAccelerationStructureData = m_scratch->GetGPUVirtualAddress();

	commandList->BuildRaytracingAccelerationStructure(&m_desc, 0, nullptr);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_tlas.Get()));
}