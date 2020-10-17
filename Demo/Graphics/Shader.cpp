#include "stdafx.h"
#include "Shader.h"
#include "ShaderBlob.h"

Shader::~Shader()
{
	delete m_blob;
	m_blob = nullptr;
}

// TODO: Fix this function to support loading seperate exports from a compiled hlsl library.
void Shader::InitializeFromBinary(const std::string& filepath)
{
	CompiledShaderBlob* blob = new CompiledShaderBlob;

	std::vector<uint8_t> data = ReadData(filepath);
	blob->MoveDataIntoBlob(data);

	m_blob = blob;
	m_bufferLength = m_blob->BufferLength();
	m_bufferPointer = m_blob->BufferPointer();
}

void Shader::FXCCompile(LPCWSTR filepath, LPCSTR entryPoint, LPCSTR target)
{
	FXCShaderBlob* blob = new FXCShaderBlob;

	ComPtr<ID3DBlob> errBuff;
	UINT flags1 = 0;
#ifdef _DEBUG
	flags1 = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = D3DCompileFromFile(filepath, nullptr, nullptr, entryPoint, target,
		flags1, 0, blob->GetAddressOf(), &errBuff);
	if (FAILED(hr))
		std::cout << (char*)errBuff->GetBufferPointer();
	else
		assert(SUCCEEDED(hr));

	m_blob = blob;
	m_bufferLength = m_blob->BufferLength();
	m_bufferPointer = m_blob->BufferPointer();
}

void Shader::DXCCompile(LPCWSTR filepath, LPCWSTR entryPoint, LPCWSTR target)
{
	DXCShaderBlob* blob = new DXCShaderBlob;

	ComPtr<IDxcLibrary> library;
	HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
	assert(SUCCEEDED(hr));

	ComPtr<IDxcCompiler> compiler;
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	assert(SUCCEEDED(hr));

	uint32_t codePage = CP_UTF8;
	ComPtr<IDxcBlobEncoding> sourceBlob;
	hr = library->CreateBlobFromFile(filepath, &codePage, &sourceBlob);
	assert(SUCCEEDED(hr));

	ComPtr<IDxcOperationResult> result;
	hr = compiler->Compile(
		sourceBlob.Get(),
		filepath,
		entryPoint,
		target,
		NULL, 0,
		NULL, 0,
		NULL,
		&result
	);
	result->GetStatus(&hr);
	if (FAILED(hr))
	{
		if (result)
		{
			ComPtr<IDxcBlobEncoding> errorsBlob;
			HRESULT hr = result->GetErrorBuffer(&errorsBlob);
			if (SUCCEEDED(hr) && errorsBlob)
			{
				std::wcout << (const char*)errorsBlob->GetBufferPointer() << std::endl;
			}
			assert(false);
		}
	}
	else
	{
		assert(SUCCEEDED(hr));
		result->GetResult(blob->GetAddressOf());
	}

	m_blob = blob;
	m_bufferLength = m_blob->BufferLength();
	m_bufferPointer = m_blob->BufferPointer();
}

uintmax_t Shader::FileSize(const std::string& filepath)
{
	std::filesystem::path p(filepath);
	if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p))
		return std::filesystem::file_size(p);
	return 0;
}

std::vector<uint8_t> Shader::ReadData(const std::string& filepath)
{
	std::vector<uint8_t> data;

	std::ifstream fs;
	fs.open(filepath.c_str(), std::ifstream::in | std::ifstream::binary);
	if (fs.good())
	{
		auto size = FileSize(filepath);
		data.resize(static_cast<size_t>(size));
		fs.seekg(0, std::ios::beg);
		fs.read(reinterpret_cast<char*>(&data[0]), size);
		fs.close();
	}
	return data;
}