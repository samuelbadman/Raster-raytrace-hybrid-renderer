#pragma once

#include "../stdafx.h"

class ShaderBlob
{
public:
	virtual ~ShaderBlob() = default;
	virtual size_t BufferLength() = 0;
	virtual const void* BufferPointer() = 0;
};

class CompiledShaderBlob : public ShaderBlob
{
public:
	size_t BufferLength() override { return m_blob.size(); }
	const void* BufferPointer() override { return  m_blob.data(); }

	void MoveDataIntoBlob(std::vector<uint8_t>& data);

private:
	std::vector<uint8_t> m_blob;
};

class FXCShaderBlob : public ShaderBlob
{
public:
	size_t BufferLength() override { return m_blob->GetBufferSize(); }
	const void* BufferPointer() override { return m_blob->GetBufferPointer(); }

	ID3DBlob** GetAddressOf() { return &m_blob; }

private:
	ComPtr<ID3DBlob> m_blob;
};

class DXCShaderBlob : public ShaderBlob
{
public:
	size_t BufferLength() override { return m_blob->GetBufferSize(); }
	const void* BufferPointer() override { return m_blob->GetBufferPointer(); }

	IDxcBlob** GetAddressOf() { return &m_blob; }

private:
	ComPtr<IDxcBlob> m_blob;
};
