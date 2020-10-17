#include "stdafx.h"
#include "Model.h"

Model::Model()
{
	m_vertexBuffer = std::make_unique<StaticVertexBuffer>();
	m_indexBuffer = std::make_unique<StaticIndexBuffer>();
	m_blas = std::make_unique<BottomLevelAccelerationStructure>();
}

void Model::Initialize(ID3D12Device5* const device)
{
	m_vertexBuffer->Initialize(device, static_cast<uint32_t>(m_vertices.size() * sizeof(Vertex)),
		static_cast<uint32_t>(sizeof(Vertex)));
	m_indexBuffer->Initialize(device, static_cast<uint32_t>(m_indices.size() * sizeof(DWORD)));
	m_blas->Initialize(device, 1);
}

void Model::Stage(ID3D12Device5* const device)
{
	m_vertexBuffer->StageData(m_vertices.data());
	m_indexBuffer->StageData(m_indices.data());
	m_blas->AddStagedGeometry(m_vertexBuffer->GetHeapGPUVirtualAddress(), DXGI_FORMAT_R32G32B32_FLOAT, sizeof(Vertex), m_vertices.size(),
		m_indexBuffer->GetHeapGPUVirtualAddress(), m_indices.size());
	m_blas->BuildStaged(device);
}

void Model::Commit(ID3D12GraphicsCommandList4* const commandList)
{
	m_vertexBuffer->CommitStagedData(commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_indexBuffer->CommitStagedData(commandList, D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	m_blas->CommitStaged(commandList);
}

void Model::StagingComplete()
{
	m_vertexBuffer->StagingComplete();
	m_indexBuffer->StagingComplete();
}

const XMMATRIX& Model::GetWorldMatrix()
{
	m_world = XMMatrixScalingFromVector(XMLoadFloat3(&m_scale)) *
		XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&m_rotation))) *
		XMMatrixTranslationFromVector(XMLoadFloat3(&m_position));
	return m_world;
}

void Model::SetPosition(const float x, const float y, const float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}

void Model::SetRotation(const float x, const float y, const float z)
{
	m_rotation.x = x;
	m_rotation.y = y;
	m_rotation.z = z;
}

void Model::SetScale(const float x, const float y, const float z)
{
	m_scale.x = x;
	m_scale.y = y;
	m_scale.z = z;
}
