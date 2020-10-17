#include "stdafx.h"
#include "BottomLevelAccelerationStructure.h"

void BottomLevelAccelerationStructure::Initialize(ID3D12Device5* const device, const uint32_t numGeometries)
{
	m_geometryDescs.resize(numGeometries, {});
}

void BottomLevelAccelerationStructure::AddStagedGeometry(const D3D12_GPU_VIRTUAL_ADDRESS vbStartAddress, const DXGI_FORMAT positionAttributeFormat,
	const uint32_t vertexStride, const uint32_t vertexCount, 
	const D3D12_GPU_VIRTUAL_ADDRESS indexBufferStartAddress, const uint32_t indexCount)
{
	assert(m_geometryID < m_geometryDescs.size());
	m_geometryDescs[m_geometryID].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	m_geometryDescs[m_geometryID].Triangles.VertexBuffer.StartAddress = vbStartAddress;
	m_geometryDescs[m_geometryID].Triangles.VertexBuffer.StrideInBytes = vertexStride;
	m_geometryDescs[m_geometryID].Triangles.VertexFormat = positionAttributeFormat;
	m_geometryDescs[m_geometryID].Triangles.VertexCount = vertexCount;
	m_geometryDescs[m_geometryID].Triangles.IndexBuffer = indexBufferStartAddress;
	m_geometryDescs[m_geometryID].Triangles.IndexCount = indexCount;
	m_geometryDescs[m_geometryID].Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	m_geometryDescs[m_geometryID].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	m_geometryID++;
}

void BottomLevelAccelerationStructure::CommitStaged(ID3D12GraphicsCommandList4* const commandList)
{
	commandList->BuildRaytracingAccelerationStructure(&m_buildDesc, 0, nullptr);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_blas.Get()));
}

void BottomLevelAccelerationStructure::BuildStaged(ID3D12Device5* const device)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = m_geometryDescs.size();
	inputs.pGeometryDescs = m_geometryDescs.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&m_scratch));

	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(&m_blas));

	m_buildDesc.Inputs = inputs;
	m_buildDesc.DestAccelerationStructureData = m_blas->GetGPUVirtualAddress();
	m_buildDesc.ScratchAccelerationStructureData = m_scratch->GetGPUVirtualAddress();
}
