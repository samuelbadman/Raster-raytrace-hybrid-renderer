#pragma once

#include "../stdafx.h"
#include "../Window.h"

namespace Direct3D
{
	static void EnableDebugLayer(const bool enableGPUBasedValidation = false)
	{
		ComPtr<ID3D12Debug> debugController;
		ComPtr<ID3D12Debug1> debugController1;
		HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		assert(SUCCEEDED(hr));
		hr = debugController->QueryInterface(IID_PPV_ARGS(&debugController1));
		assert(SUCCEEDED(hr));
		debugController->EnableDebugLayer();
		if (enableGPUBasedValidation) debugController1->SetEnableGPUBasedValidation(TRUE);
	}

	static bool SupportsDirectXRaytracing(IDXGIAdapter1* const adapter)
	{
		ComPtr<ID3D12Device> testDevice;
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
		return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
			&& SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
			&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
	}

	static ComPtr<IDXGIAdapter1> GetHardwareAdapter(const bool requireDirectXRaytracing)
	{
		ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;
#ifdef _DEBUG
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
		HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
		assert(SUCCEEDED(hr));

		ComPtr<IDXGIAdapter1> adapter;
		int adapterIndex = 0;
		int bestAdapterIndex = -1;
		SIZE_T maxDedicatedVideoMemory = 0;
		while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
				SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))
				&& desc.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				if (requireDirectXRaytracing)
				{
					if (SupportsDirectXRaytracing(adapter.Get()))
					{
						maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
						bestAdapterIndex = adapterIndex;
					}
				}
				else
				{
					maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
					bestAdapterIndex = adapterIndex;
				}
			}
			adapterIndex++;
		}

		if (bestAdapterIndex < 0)
		{
			std::cout << "No available supported adapter" << std::endl;
			assert(false);
		}

		dxgiFactory->EnumAdapters1(bestAdapterIndex, &adapter);
		assert(adapter);
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		return adapter;
	}

	static ComPtr<ID3D12Device5> CreateDevice(IDXGIAdapter1* const adapter)
	{
		ComPtr<ID3D12Device5> device;
		if (adapter)
		{
			HRESULT hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
			assert(SUCCEEDED(hr));
		}
		return device;
	}

	static ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* const device, const D3D12_COMMAND_LIST_TYPE type)
	{
		ComPtr<ID3D12CommandQueue> commandQueue;
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = type;
		if (device)
		{
			HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue));
			assert(SUCCEEDED(hr));
		}
		return commandQueue;
	}

	static ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE type,
		const uint32_t numDescriptors, const D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	{
		ComPtr<ID3D12DescriptorHeap> heap;
		if (device)
		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = type;
			desc.NumDescriptors = numDescriptors;
			desc.Flags = flags;
			HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
			assert(SUCCEEDED(hr));
		}
		return heap;
	}

	static ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device* const device, const D3D12_COMMAND_LIST_TYPE type)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		if (device)
		{
			HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
			assert(SUCCEEDED(hr));
		}
		return commandAllocator;
	}

	static ComPtr<ID3D12GraphicsCommandList4> CreateCommandList(ID3D12Device* const device, const D3D12_COMMAND_LIST_TYPE type,
		ID3D12CommandAllocator* const commandAllocator, ID3D12PipelineState* initialState)
	{
		ComPtr<ID3D12GraphicsCommandList4> commandList;
		if (device)
		{
			HRESULT hr = device->CreateCommandList(0, type, commandAllocator, initialState, IID_PPV_ARGS(&commandList));
			assert(SUCCEEDED(hr));
			hr = commandList->Close();
			assert(SUCCEEDED(hr));
		}
		return commandList;
	}

	static void WaitForFenceValueOnCPU(ID3D12Fence* const fence, const uint64_t value, const HANDLE event, 
		std::chrono::milliseconds duration = std::chrono::milliseconds::max())
	{
		if (fence)
		{
			if (fence->GetCompletedValue() < value)
			{
				fence->SetEventOnCompletion(value, event);
				WaitForSingleObject(event, static_cast<DWORD>(duration.count()));
			}
		}
		else
		{
			std::cout << "Warning: a null fence was passed to WaitForFenceValueOnCPU." << std::endl;
		}
	}

	static uint64_t SignalFenceOnCPU(ID3D12Fence* const fence, uint64_t& value)
	{
		if (fence)
		{
			HRESULT hr = fence->Signal(++value);
			assert(SUCCEEDED(hr));
		}
		return value;
	}

	static uint64_t SignalFenceOnGPU(ID3D12Fence* const fence, ID3D12CommandQueue* const commandQueue, uint64_t& value)
	{
		if (fence && commandQueue)
		{
			HRESULT hr = commandQueue->Signal(fence, ++value);
			assert(SUCCEEDED(hr));
		}
		return value;
	}

	static void WaitForFenceValueOnGPU(ID3D12Fence* const fence, ID3D12CommandQueue* const commandQueue, const uint64_t value)
	{
		if (fence && commandQueue)
		{
			HRESULT hr = commandQueue->Wait(fence, value);
			assert(SUCCEEDED(hr));
		}
	}

	static void CreateDepthStencilView(ID3D12Device* const device, const uint32_t width, const uint32_t height, 
		const D3D12_CPU_DESCRIPTOR_HANDLE hHeapStart, ID3D12Resource** const ppResource)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;
		HRESULT hr = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(ppResource));
		assert(SUCCEEDED(hr));
		device->CreateDepthStencilView(*ppResource,
			&depthStencilDesc,
			hHeapStart);
	}

	static void CreateRenderTargetsForWindow(ID3D12Device* const device, const Window* const window, const uint32_t backBufferCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE hHeapStart, ComPtr<ID3D12Resource>* const pResources)
	{
		auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(hHeapStart);
		for (uint32_t i = 0; i < backBufferCount; i++)
		{
			window->GetSwapChainSurface(i, &pResources[i]);
			device->CreateRenderTargetView(pResources[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(rtvDescriptorSize);
		}
	}
}