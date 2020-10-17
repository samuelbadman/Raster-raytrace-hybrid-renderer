#pragma once

#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_impl_win32.h"
#include "ThirdParty/Imgui/imgui_impl_dx12.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <Xinput.h>

// included for hresult error handling with _com_error. Example usage commented below
#ifdef _DEBUG
#include <comdef.h>
//_com_error err(hr);
//LPCTSTR errMsg = err.ErrorMessage();
//std::wcout << errMsg << std::endl;
#endif

using namespace Microsoft::WRL;
using namespace DirectX;

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#ifdef CreateWindow
#undef CreateWindow
#endif

#include <assert.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <array>
#include <functional>

#include "ThirdParty/d3dx12.h"
