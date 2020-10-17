#include "stdafx.h"
#include "ShaderBlob.h"

void CompiledShaderBlob::MoveDataIntoBlob(std::vector<uint8_t>& data)
{
	m_blob = std::move(data);
	data.clear();
}