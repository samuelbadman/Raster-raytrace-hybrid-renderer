#pragma once

#include "../stdafx.h"
#include "StaticVertexBuffer.h"
#include "StaticIndexBuffer.h"
#include "BottomLevelAccelerationStructure.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Norm;
	XMFLOAT2 Uv;

	Vertex() : Pos({ 0.f, 0.f, 0.f }), Norm({ 0.f, 0.f, 0.f }), Uv({ 0.f, 0.f }) {}
	Vertex(const float posX, const float posY, const float posZ, const float normX, const float normY, const float normZ,
		const float u, const float v)
		: Pos({ posX, posY, posZ }), Norm({ normX, normY, normZ }), Uv({ u, v }) {}
};

class Model
{
public:
	Model();
	StaticVertexBuffer* GetVertexBuffer() const { return m_vertexBuffer.get(); }
	StaticIndexBuffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
	void PushBackVertex(const Vertex& vertex) { m_vertices.push_back(vertex); }
	void PushBackIndex(const DWORD& index) { m_indices.push_back(index); }
	void Initialize(ID3D12Device5* const device);
	void Stage(ID3D12Device5* const device);
	void Commit(ID3D12GraphicsCommandList4* const commandList);
	void StagingComplete();
	uint32_t GetNumIndices() const { return static_cast<uint32_t>(m_indices.size()); }
	const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const { return m_vertexBuffer->GetView(); }
	const D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const { return m_indexBuffer->GetView(); }
	const XMMATRIX& GetWorldMatrix();
	void SetPosition(const float x, const float y, const float z);
	void SetRotation(const float x, const float y, const float z);
	void SetScale(const float x, const float y, const float z);
	D3D12_GPU_VIRTUAL_ADDRESS GetVertexBufferGPUVirtualAddress() const { return m_vertexBuffer->GetHeapGPUVirtualAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetIndexBufferGPUVirtualAddress() const { return m_indexBuffer->GetHeapGPUVirtualAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetBottomLevelAccelerationStructureGPUVirtualAddress() const { return m_blas->GetGPUVirtualAddress(); }
	const XMFLOAT3& GetPosition() const { return m_position; }
	const XMFLOAT3& GetRotation() const { return m_rotation; }
	const XMFLOAT3& GetScale() const { return m_scale; }

private:
	std::unique_ptr<StaticVertexBuffer> m_vertexBuffer;
	std::unique_ptr<StaticIndexBuffer> m_indexBuffer;
	std::vector<Vertex> m_vertices;
	std::vector<DWORD> m_indices;
	XMFLOAT3 m_position = { 0.f, 0.f, 0.f };
	XMFLOAT3 m_rotation = { XMConvertToRadians(90.f), XMConvertToRadians(0.f), XMConvertToRadians(0.f) };
	XMFLOAT3 m_scale = { 1.f, 1.f, 1.f };
	XMMATRIX m_world = XMMatrixIdentity();
	std::unique_ptr<BottomLevelAccelerationStructure> m_blas;
};