#pragma once

#include "../stdafx.h"

class Shader
{
public:
	~Shader();

	void InitializeFromBinary(const std::string& filepath);
	void FXCCompile(LPCWSTR filepath, LPCSTR entryPoint, LPCSTR target);
	void DXCCompile(LPCWSTR filepath, LPCWSTR entryPoint, LPCWSTR target);

	size_t BufferLength() const { return m_bufferLength; }
	const void* BufferPointer() const { return m_bufferPointer; }

private:
	uintmax_t FileSize(const std::string& filepath);
	std::vector<uint8_t> ReadData(const std::string& filepath);

private:
	size_t m_bufferLength = 0;
	const void* m_bufferPointer = nullptr;
	class ShaderBlob* m_blob = nullptr;
};
