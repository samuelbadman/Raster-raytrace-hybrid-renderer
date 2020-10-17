#include "stdafx.h"
#include "Console.h"
#include "Window.h"
#include "Gamepad.h"
#include "Queue.h"
#include "InputDefinitions.h"
#include "InputFunctions.h"
#include "Macros.h"
#include "Graphics/Direct3DStatics.h"
#include "Graphics/Shader.h"
#include "Graphics/InputLayout.h"
#include "Graphics/RootSignature.h"
#include "Graphics/GraphicsPipelineState.h"
#include "Graphics/DynamicConstantBuffer.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/StaticVertexBuffer.h"
#include "Graphics/StaticIndexBuffer.h"
#include "Graphics/Fence.h"
#include "Graphics/StaticConstantBuffer.h"
#include "Graphics/Texture2D.h"
#include "Graphics/SamplerType.h"
#include "Graphics/Model.h"
#include "Graphics/TopLevelAccelerationStructure.h"

#include "ThirdParty/Assimp/importer.hpp"
#include "ThirdParty/Assimp/scene.h"
#include "ThirdParty/Assimp/postprocess.h"

// window
static bool vsync = true;
static std::unique_ptr<Window> window;
static const uint32_t bufferCount = 3;

// gamepad
static std::unique_ptr<Gamepad> gamepad;

// input
static std::unique_ptr<Queue<InputEvent>> inputEventQueue;
static const uint32_t maxPendingInputEvents = 8;
static bool rightMouseDown = false;

// camera
static XMFLOAT3 cameraPos = { 0.f, 0.f, -5.f };
static XMFLOAT3 cameraUp = { 0.f, 1.f, 0.f };
static float cameraYaw = 90.f;
static float cameraPitch = 0.f;
static float cameraLookSensitivity = 10.f;
static bool cameraInvertVerticalLook = false;
static float cameraSpeed = 5.f;

// model
static std::string textureFilepath = "Assets/checkerTexture.png";
static std::array<std::unique_ptr<Model>, 3> sphereModels;
static std::unique_ptr<Model> floorModel;

// directional light
struct DirectionalLight
{
	XMFLOAT3 Direction = { 0.6f, -1.0f, 1.0f };
	float padding = 0.f;
	XMFLOAT3 Diffuse = { 1.f, 1.f, 1.f };
	float padding2 = 0.f;
	XMFLOAT3 Ambient = { 0.f, 0.f, 0.f };
	float padding3 = 0.f;
};

static void AddCameraPitch(const float amount, const float min, const float max)
{
	cameraPitch += amount;
	
	if (cameraPitch > max)
		cameraPitch = max;
	if (cameraPitch < min)
		cameraPitch = min;
}

static void AddCameraYaw(const float amount)
{
	cameraYaw += amount;
}

// graphics
static ComPtr<IDXGIAdapter1> adapter;
static ComPtr<ID3D12Device5> device;
static ComPtr<ID3D12CommandQueue> graphicsQueue;
static std::array<ComPtr<ID3D12Resource>, bufferCount> renderTargets;
static std::unique_ptr<DescriptorHeap> rtvHeap;
static ComPtr<ID3D12Resource> dsv;
static std::unique_ptr<DescriptorHeap> dsvHeap;
static std::array<ComPtr<ID3D12CommandAllocator>, bufferCount> graphicsCommandAllocators;
static ComPtr<ID3D12GraphicsCommandList4> graphicsCommandList;
static std::array<std::unique_ptr<Fence>, bufferCount> backBufferFences;
static HANDLE fenceEvent = nullptr;
static D3D12_VIEWPORT viewport = {};
static D3D12_RECT scissorRect = {};
static const float clearColor[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
static std::unique_ptr<Fence> initializationFence;
static std::unique_ptr<DescriptorHeap> shaderDescriptorHeap;

static std::unique_ptr<Shader> vertexShader;
static std::unique_ptr<Shader> pixelShader;
static std::unique_ptr<InputLayout> inputLayout;
static std::unique_ptr<RootSignature> rootSignature;
static std::unique_ptr<GraphicsPipelineState> graphicsPipeline;

static ComPtr<ID3D12StateObject> rtPipelineState;
static ComPtr<ID3D12Resource> RTShadowMapOutput;

// GBuffer
static ComPtr<ID3D12Resource> sceneTextureGBuffer;
static ComPtr<ID3D12Resource> shadowMapTextureGBuffer;

struct PerFrameConstantBuffer
{
	XMFLOAT4X4 ViewProjection;
	DirectionalLight Light;
};
static std::unique_ptr<DynamicConstantBuffer> perFrameDynamicConstantBuffer;
static PerFrameConstantBuffer perFrameData = {};

struct PerObjectConstantBuffer
{
	XMFLOAT4X4 World;
	XMFLOAT4X4 WorldInvTranspose;
};
static std::unique_ptr<DynamicConstantBuffer> perObjectDynamicConstantBuffer;
static std::array<PerObjectConstantBuffer, 4> objectData;

struct RTPerFrameConstantBuffer
{
	XMFLOAT4X4 inverseView;
	XMFLOAT4X4 inverseProjection;
	XMFLOAT4 lightDirection = { 0.6f, -1.0f, 1.0f, 0.f };
};
static std::unique_ptr<DynamicConstantBuffer> rtPerFrameDynamicConstantBuffer;
static RTPerFrameConstantBuffer rtPerFrameData = {};

// Vertex structures
struct ScreenQuadVertex
{
	XMFLOAT3 Position;
	XMFLOAT2 UV;

	ScreenQuadVertex(const float x, const float y, const float z, const float u, const float v) :
		Position({ x,y,z }), UV({ u,v }) {}
};

static void InitializeGBuffer(const uint32_t width, const uint32_t height)
{
	// Whenever initializeGBuffer is called, the descriptors in the descriptor heap must be recreated to point to the new locations.

	// scene texture from raster pass.
	sceneTextureGBuffer.Reset();
	auto sceneTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	sceneTextureDesc.MipLevels = 1;
	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&sceneTextureDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&sceneTextureGBuffer));

	// shadow map from raytrace pass.
	RTShadowMapOutput.Reset();
	auto shadowMapDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	shadowMapDesc.MipLevels = 1;
	shadowMapDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&shadowMapDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&RTShadowMapOutput));

	// shadow map texture
	shadowMapTextureGBuffer.Reset();
	auto shadowMapTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	shadowMapTextureDesc.MipLevels = 1;
	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&shadowMapTextureDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&shadowMapTextureGBuffer));
}

static void InputEventCallback(const InputEvent& event)
{
	inputEventQueue->Push(event);
}

static void WindowResizedCallback(const uint32_t newWidth, const uint32_t newHeight)
{
	for (uint32_t i = 0; i < bufferCount; i++)
	{
		Direct3D::SignalFenceOnGPU(backBufferFences[i]->GetInterfacePtr(), graphicsQueue.Get(), backBufferFences[i]->Value());
		Direct3D::WaitForFenceValueOnCPU(backBufferFences[i]->GetInterfacePtr(), backBufferFences[i]->Value(), fenceEvent);
		renderTargets[i].Reset();
	}

	window->ResizeSwapChain(bufferCount, newWidth, newHeight);
	Direct3D::CreateRenderTargetsForWindow(device.Get(), window.get(), bufferCount, rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		renderTargets.data());
	dsv.Reset();
	Direct3D::CreateDepthStencilView(device.Get(), newWidth, newHeight, dsvHeap->GetCPUDescriptorHandleForHeapStart(), &dsv);

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(newWidth);
	viewport.Height = static_cast<FLOAT>(newHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(newWidth);
	scissorRect.bottom = static_cast<LONG>(newHeight);

	InitializeGBuffer(newWidth, newHeight);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	device->CreateShaderResourceView(sceneTextureGBuffer.Get(), &srvDesc, shaderDescriptorHeap->GetCPUDescriptorHandle(1));

	device->CreateShaderResourceView(shadowMapTextureGBuffer.Get(), &srvDesc, shaderDescriptorHeap->GetCPUDescriptorHandle(2));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(RTShadowMapOutput.Get(), nullptr, &uavDesc, shaderDescriptorHeap->GetCPUDescriptorHandle(4));
}

static void ProcessInputEventQueue(const float deltaSeconds)
{
	const int maxPending = inputEventQueue->GetMaxPending();
	const int tail = inputEventQueue->GetTail();
	for (int i = inputEventQueue->GetHead(); i != tail; i = (i + 1) % maxPending)
	{
		const auto& event = inputEventQueue->PopNextItem();
		if (!event.consumed)
		{
			if (event.input == KEY_ESCAPE && event.data == 1.0f && !event.repeatKey)
				window->Close();

			if (event.input == KEY_F && event.data == 1.0f && !event.repeatKey)
				window->ToggleFullscreen();

			if (event.input == KEY_C && event.data == 1.0f && !event.repeatKey)
			{
				for (int i = 0; i < 5000; i++)
				{
					std::cout << std::endl;
				}
			}

			if (event.input == MOUSEBUTTON_RIGHTMOUSEBUTTON && event.data == 1.f)
			{
				rightMouseDown = true;
			}

			if (event.input == MOUSEBUTTON_RIGHTMOUSEBUTTON && event.data == 0.f)
			{
				rightMouseDown = false;
			}

			if (rightMouseDown)
			{
				if (event.input == MOUSE_X_AXIS && !event.consumed)
				{
					float delta = cameraInvertVerticalLook ? event.data : -event.data;
					AddCameraYaw(delta *cameraLookSensitivity * deltaSeconds);
				}

				if (event.input == MOUSE_Y_AXIS && !event.consumed)
				{
					float delta = cameraInvertVerticalLook ? event.data : -event.data;
					AddCameraPitch(delta * cameraLookSensitivity * deltaSeconds, -89.9f, 89.9f);
				}
			}
		}
	}
}

static void ProcessMesh(aiMesh* mesh, const aiScene* scene, Model* const model)
{
	for (uint32_t i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vert = {};
		vert.Pos.x = mesh->mVertices[i].x;
		vert.Pos.y = mesh->mVertices[i].y;
		vert.Pos.z = mesh->mVertices[i].z;

		vert.Norm.x = mesh->mNormals[i].x;
		vert.Norm.y = mesh->mNormals[i].y;
		vert.Norm.z = mesh->mNormals[i].z;

		if (mesh->mTextureCoords[0])
		{
			vert.Uv.x = mesh->mTextureCoords[0][i].x;
			vert.Uv.y = mesh->mTextureCoords[0][i].y;
		}
		else
		{
			vert.Uv.x = 0.f;
			vert.Uv.y = 0.f;
		}

		model->PushBackVertex(vert);
	}

	for (uint32_t i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (uint32_t j = 0; j < face.mNumIndices; j++)
		{
			model->PushBackIndex(face.mIndices[j]);
		}
	}
}

static void ProcessNode(aiNode* node, const aiScene* scene, Model* const model)
{
	for (uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, model);
	}
	for (uint32_t i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, model);
	}
}

static void LoadModel(Model* const model, const std::string& filepath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filepath, aiProcess_Triangulate);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
		assert(false);
	}
	ProcessNode(scene->mRootNode, scene, model);
}

std::unique_ptr<TopLevelAccelerationStructure> sceneAccelerationStructure;
void BuildSceneAccelerationStructure()
{
	sceneAccelerationStructure->SetInstance(0, 0, sphereModels[0]->GetWorldMatrix(), 0xFF, D3D12_RAYTRACING_INSTANCE_FLAG_NONE,
		sphereModels[0]->GetBottomLevelAccelerationStructureGPUVirtualAddress());
	sceneAccelerationStructure->SetInstance(1, 0, sphereModels[1]->GetWorldMatrix(), 0xFF, D3D12_RAYTRACING_INSTANCE_FLAG_NONE,
		sphereModels[0]->GetBottomLevelAccelerationStructureGPUVirtualAddress());
	sceneAccelerationStructure->SetInstance(2, 0, sphereModels[2]->GetWorldMatrix(), 0xFF, D3D12_RAYTRACING_INSTANCE_FLAG_NONE,
		sphereModels[0]->GetBottomLevelAccelerationStructureGPUVirtualAddress());
	sceneAccelerationStructure->SetInstance(3, 0, floorModel->GetWorldMatrix(), 0xFF, D3D12_RAYTRACING_INSTANCE_FLAG_NONE,
		floorModel->GetBottomLevelAccelerationStructureGPUVirtualAddress());
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	Console::RedirectIOToConsole();
#endif

	inputEventQueue = std::make_unique<Queue<InputEvent>>(maxPendingInputEvents);

	window = std::make_unique<Window>();
	window->Initialize(L"Game window", false);
	window->SetVsyncEnabled(vsync);
	window->RegisterOnResizedCallback(&WindowResizedCallback);

	gamepad = std::make_unique<Gamepad>();

#ifdef _DEBUG
	Direct3D::EnableDebugLayer();
#endif

	adapter = Direct3D::GetHardwareAdapter(false);
	device = Direct3D::CreateDevice(adapter.Get());
	graphicsQueue = Direct3D::CreateCommandQueue(device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	window->CreateSwapChain(bufferCount, graphicsQueue.Get());
	
	rtvHeap = std::make_unique<DescriptorHeap>();
	rtvHeap->Initialize(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, bufferCount, false);
	Direct3D::CreateRenderTargetsForWindow(device.Get(), window.get(), bufferCount, rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		renderTargets.data());
	
	dsvHeap = std::make_unique<DescriptorHeap>();
	dsvHeap->Initialize(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	Direct3D::CreateDepthStencilView(device.Get(), window->GetClientWidth(), window->GetClientHeight(), 
		dsvHeap->GetCPUDescriptorHandleForHeapStart(), &dsv);
	
	for (uint32_t i = 0; i < bufferCount; i++)
	{
		graphicsCommandAllocators[i] = Direct3D::CreateCommandAllocator(device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	graphicsCommandList = Direct3D::CreateCommandList(device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT,
		graphicsCommandAllocators[0].Get(), nullptr);

	for (uint32_t i = 0; i < bufferCount; i++)
	{
		backBufferFences[i] = std::make_unique<Fence>();
		backBufferFences[i]->Initialize(device.Get(), 0, D3D12_FENCE_FLAG_NONE);
	}
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(fenceEvent != nullptr);
	initializationFence = std::make_unique<Fence>();
	initializationFence->Initialize(device.Get(), 0, D3D12_FENCE_FLAG_NONE);

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(window->GetClientWidth());
	viewport.Height = static_cast<FLOAT>(window->GetClientHeight());
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(window->GetClientWidth());
	scissorRect.bottom = static_cast<LONG>(window->GetClientHeight());

	// build rasterization pipelines
	rootSignature = std::make_unique<RootSignature>();
	rootSignature->AddStaticSampler(SamplerType::LinearWrap, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootSignature->AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootSignature->AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootSignature->AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootSignature->AddRootDescriptorTableParameter({
		{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND}
		}, D3D12_SHADER_VISIBILITY_PIXEL);
	rootSignature->SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
	rootSignature->Create(device.Get());
	vertexShader = std::make_unique<Shader>();
	vertexShader->FXCCompile(L"ShaderSource/Shaders.hlsl", "vertex", "vs_5_1");
	pixelShader = std::make_unique<Shader>();
	pixelShader->FXCCompile(L"ShaderSource/Shaders.hlsl", "pixel", "ps_5_1");
	inputLayout = std::make_unique<InputLayout>();
	inputLayout->AddInputElement("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0);
	inputLayout->AddInputElement("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Norm),
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0);
	inputLayout->AddInputElement("TEXTURECOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, Uv),
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0);
	inputLayout->Create();
	graphicsPipeline = std::make_unique<GraphicsPipelineState>();
	graphicsPipeline->SetInputLayout(inputLayout->GetInterfacePtr());
	graphicsPipeline->SetRootSignature(rootSignature->GetInterfacePtr());
	graphicsPipeline->SetVertexShader(vertexShader->BufferLength(), vertexShader->BufferPointer());
	graphicsPipeline->SetPixelShader(pixelShader->BufferLength(), pixelShader->BufferPointer());
	graphicsPipeline->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	graphicsPipeline->SetRTVFormats(1, DXGI_FORMAT_R8G8B8A8_UNORM);
	graphicsPipeline->SetDSVFormat(DXGI_FORMAT_D32_FLOAT);
	graphicsPipeline->SetSampleDesc(window->GetSwapChainSampleDesc());
	graphicsPipeline->SetSampleMask(0xffffffff);
	graphicsPipeline->SetRasterizerState(RasterizerState::BackCulling);
	graphicsPipeline->SetDepthStencilState(DepthStencilState::Default);
	graphicsPipeline->SetBlendState(BlendState::Opaque);
	graphicsPipeline->SetNumRenderTargets(1);
	graphicsPipeline->Create(device.Get());

	std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
	texture->Initialize(device.Get(), textureFilepath);
	texture->StageData(texture->GetData());

	// Sphere model data is only being loaded for sphereModel[0]. Other sphere model's share the data from this instance.
	sphereModels[0] = std::make_unique<Model>();
	LoadModel(sphereModels[0].get(), "Assets/Sphere.fbx");
	sphereModels[0]->Initialize(device.Get());
	sphereModels[0]->Stage(device.Get());
	sphereModels[1] = std::make_unique<Model>();
	sphereModels[1]->SetPosition(-2.2f, 0.f, 0.f);
	sphereModels[2] = std::make_unique<Model>();
	sphereModels[2]->SetPosition(2.2f, 0.f, 0.f);

	floorModel = std::make_unique<Model>();
	LoadModel(floorModel.get(), "Assets/floor.fbx");
	floorModel->Initialize(device.Get());
	floorModel->Stage(device.Get());

	sceneAccelerationStructure = std::make_unique<TopLevelAccelerationStructure>();
	sceneAccelerationStructure->Initialize(device.Get(), 4, true);
	BuildSceneAccelerationStructure();
	sceneAccelerationStructure->Stage(device.Get());

	std::vector<ScreenQuadVertex> screenQuadVertices;
	std::vector<DWORD> screenQuadIndices;
	screenQuadVertices.push_back({ -1.f, 1.f, 0.f, 0.f, 1.f });
	screenQuadVertices.push_back({ 1.f, 1.f, 0.f, 1.f, 1.f });
	screenQuadVertices.push_back({ -1.f, -1.f, 0.f, 0.f, 0.f });
	screenQuadVertices.push_back({ 1.f, -1.f, 0.f, 1.f, 0.f });
	screenQuadIndices.push_back(0);
	screenQuadIndices.push_back(1);
	screenQuadIndices.push_back(2);
	screenQuadIndices.push_back(2);
	screenQuadIndices.push_back(1);
	screenQuadIndices.push_back(3);

	std::unique_ptr<RootSignature> FinalPassRootSignature = std::make_unique<RootSignature>();
	FinalPassRootSignature->AddStaticSampler(SamplerType::LinearWrap, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	FinalPassRootSignature->AddRootDescriptorTableParameter({
		{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND}
		}, D3D12_SHADER_VISIBILITY_PIXEL);
	FinalPassRootSignature->SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
	FinalPassRootSignature->Create(device.Get());
	std::unique_ptr<Shader> FinalPassVertexShader = std::make_unique<Shader>();
	FinalPassVertexShader->FXCCompile(L"ShaderSource/FinalPassShaders.hlsl", "vertex", "vs_5_1");
	std::unique_ptr<Shader> FinalPassPixelShader = std::make_unique<Shader>();
	FinalPassPixelShader->FXCCompile(L"ShaderSource/FinalPassShaders.hlsl", "pixel", "ps_5_1");
	std::unique_ptr<InputLayout> FinalPassInputLayout = std::make_unique<InputLayout>();
	FinalPassInputLayout->AddInputElement("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0);
	FinalPassInputLayout->AddInputElement("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(ScreenQuadVertex, UV),
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0);
	FinalPassInputLayout->Create();
	std::unique_ptr<GraphicsPipelineState> FinalPassGraphicsPipeline = std::make_unique<GraphicsPipelineState>();
	FinalPassGraphicsPipeline->SetInputLayout(FinalPassInputLayout->GetInterfacePtr());
	FinalPassGraphicsPipeline->SetRootSignature(FinalPassRootSignature->GetInterfacePtr());
	FinalPassGraphicsPipeline->SetVertexShader(FinalPassVertexShader->BufferLength(), FinalPassVertexShader->BufferPointer());
	FinalPassGraphicsPipeline->SetPixelShader(FinalPassPixelShader->BufferLength(), FinalPassPixelShader->BufferPointer());
	FinalPassGraphicsPipeline->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	FinalPassGraphicsPipeline->SetRTVFormats(1, DXGI_FORMAT_R8G8B8A8_UNORM);
	FinalPassGraphicsPipeline->SetDSVFormat(DXGI_FORMAT_D32_FLOAT);
	FinalPassGraphicsPipeline->SetSampleDesc(window->GetSwapChainSampleDesc());
	FinalPassGraphicsPipeline->SetSampleMask(0xffffffff);
	FinalPassGraphicsPipeline->SetRasterizerState(RasterizerState::BackCulling);
	FinalPassGraphicsPipeline->SetDepthStencilState(DepthStencilState::None);
	FinalPassGraphicsPipeline->SetBlendState(BlendState::Opaque);
	FinalPassGraphicsPipeline->SetNumRenderTargets(1);
	FinalPassGraphicsPipeline->Create(device.Get());

	std::unique_ptr<StaticVertexBuffer> screenQuadVertexBuffer = std::make_unique<StaticVertexBuffer>();
	screenQuadVertexBuffer->Initialize(device.Get(), static_cast<uint32_t>(sizeof(ScreenQuadVertex) * screenQuadVertices.size()),
		static_cast<uint32_t>(sizeof(ScreenQuadVertex)));
	screenQuadVertexBuffer->StageData(screenQuadVertices.data());
	std::unique_ptr<StaticIndexBuffer> screenQuadIndexBuffer = std::make_unique<StaticIndexBuffer>();
	screenQuadIndexBuffer->Initialize(device.Get(), static_cast<uint32_t>(sizeof(DWORD) * screenQuadIndices.size()));
	screenQuadIndexBuffer->StageData(screenQuadIndices.data());

	perFrameDynamicConstantBuffer = std::make_unique<DynamicConstantBuffer>();
	perFrameDynamicConstantBuffer->Initialize(device.Get(), ALIGN_TO(sizeof(PerFrameConstantBuffer), _64KB), bufferCount, 1);

	rtPerFrameDynamicConstantBuffer = std::make_unique<DynamicConstantBuffer>();
	rtPerFrameDynamicConstantBuffer->Initialize(device.Get(), ALIGN_TO(sizeof(RTPerFrameConstantBuffer), _64KB), bufferCount, 1);

	perObjectDynamicConstantBuffer = std::make_unique<DynamicConstantBuffer>();
	perObjectDynamicConstantBuffer->Initialize(device.Get(), ALIGN_TO(sizeof(PerObjectConstantBuffer), _64KB), bufferCount, 4);

	InitializeGBuffer(window->GetClientWidth(), window->GetClientHeight());

	// build descriptor heaps
	const uint32_t numDescriptors = 6;
	shaderDescriptorHeap = std::make_unique<DescriptorHeap>();
	shaderDescriptorHeap->Initialize(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, numDescriptors, true);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetFormat();
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(texture->GetResource(), &srvDesc, shaderDescriptorHeap->GetCPUDescriptorHandle(0));

	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	device->CreateShaderResourceView(sceneTextureGBuffer.Get(), &srvDesc, shaderDescriptorHeap->GetCPUDescriptorHandle(1));

	device->CreateShaderResourceView(shadowMapTextureGBuffer.Get(), &srvDesc, shaderDescriptorHeap->GetCPUDescriptorHandle(2));

	D3D12_SHADER_RESOURCE_VIEW_DESC asSrvDesc = {};
	asSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	asSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	asSrvDesc.RaytracingAccelerationStructure.Location = sceneAccelerationStructure->GetGPUVirtualAddress();
	device->CreateShaderResourceView(nullptr, &asSrvDesc, shaderDescriptorHeap->GetCPUDescriptorHandle(3));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(RTShadowMapOutput.Get(), nullptr, &uavDesc, shaderDescriptorHeap->GetCPUDescriptorHandle(4));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window->GetHWND());
	ImGui_ImplDX12_Init(device.Get(), bufferCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, shaderDescriptorHeap->GetInterfacePtr(),
		shaderDescriptorHeap->GetCPUDescriptorHandle(shaderDescriptorHeap->GetNumDescriptors() - 1),
		shaderDescriptorHeap->GetGPUDescriptorHandle(shaderDescriptorHeap->GetNumDescriptors() - 1));

	// build raytracing pipeline
	std::unique_ptr<Shader> rtShaderLib = std::make_unique<Shader>();
	rtShaderLib->DXCCompile(L"ShaderSource/RaytracingShaders.hlsl", L"", L"lib_6_3");

	CD3DX12_STATE_OBJECT_DESC rtpsoDesc = {};
	rtpsoDesc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	const WCHAR* hitGroupExportName = L"HitGroup";
	const WCHAR* shadowHitGroupExportName = L"ShadowHitGroup";

	const WCHAR* rayGenExportName = L"RayGen";
	const WCHAR* missExportName = L"Miss";
	const WCHAR* closestHitExportName = L"ClosestHit";
	const WCHAR* shadowMissExportName = L"ShadowMiss";
	const WCHAR* shadowClosestHitExportName = L"ShadowClosestHit";
	const WCHAR* exports[] = { rayGenExportName, missExportName, closestHitExportName, shadowMissExportName, 
		shadowClosestHitExportName };
	CD3DX12_DXIL_LIBRARY_SUBOBJECT dxilLibSubobject;
	dxilLibSubobject.SetDXILLibrary(&CD3DX12_SHADER_BYTECODE(rtShaderLib->BufferPointer(), rtShaderLib->BufferLength()));
	dxilLibSubobject.DefineExports<_countof(exports)>(exports);
	dxilLibSubobject.AddToStateObject(rtpsoDesc);

	CD3DX12_HIT_GROUP_SUBOBJECT hitGroupSubobject;
	hitGroupSubobject.SetIntersectionShaderImport(nullptr);
	hitGroupSubobject.SetAnyHitShaderImport(nullptr);
	hitGroupSubobject.SetClosestHitShaderImport(closestHitExportName);
	hitGroupSubobject.SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	hitGroupSubobject.SetHitGroupExport(hitGroupExportName);
	hitGroupSubobject.AddToStateObject(rtpsoDesc);

	RootSignature hitGroupRootSig;
	hitGroupRootSig.AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	hitGroupRootSig.AddRootDescriptorTableParameter({
		{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND}
		}, D3D12_SHADER_VISIBILITY_ALL);
	hitGroupRootSig.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	hitGroupRootSig.Create(device.Get());
	CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT hitGroupLocalRootSigSubobject;
	hitGroupLocalRootSigSubobject.SetRootSignature(hitGroupRootSig.GetInterfacePtr());
	hitGroupLocalRootSigSubobject.AddToStateObject(rtpsoDesc);

	CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT hitGroupAssociationSubobject;
	hitGroupAssociationSubobject.AddExport(hitGroupExportName);
	hitGroupAssociationSubobject.SetSubobjectToAssociate(hitGroupLocalRootSigSubobject);
	hitGroupAssociationSubobject.AddToStateObject(rtpsoDesc);

	CD3DX12_HIT_GROUP_SUBOBJECT shadowHitGroupSubobject;
	shadowHitGroupSubobject.SetIntersectionShaderImport(nullptr);
	shadowHitGroupSubobject.SetAnyHitShaderImport(nullptr);
	shadowHitGroupSubobject.SetClosestHitShaderImport(shadowClosestHitExportName);
	shadowHitGroupSubobject.SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	shadowHitGroupSubobject.SetHitGroupExport(shadowHitGroupExportName);
	shadowHitGroupSubobject.AddToStateObject(rtpsoDesc);

	UINT payloadSize = sizeof(float) * 3;
	UINT attributeSize = sizeof(float) * 2;
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT rtShaderConfigSubobject;
	rtShaderConfigSubobject.Config(payloadSize, attributeSize);
	rtShaderConfigSubobject.AddToStateObject(rtpsoDesc);

	RootSignature rayGenRootSig;
	rayGenRootSig.AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rayGenRootSig.AddRootDescriptorTableParameter({
		{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND},
		{D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND}
		}, D3D12_SHADER_VISIBILITY_ALL);
	rayGenRootSig.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	rayGenRootSig.Create(device.Get());
	CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT rayGenLocalRootSigSubobject;
	rayGenLocalRootSigSubobject.SetRootSignature(rayGenRootSig.GetInterfacePtr());
	rayGenLocalRootSigSubobject.AddToStateObject(rtpsoDesc);

	CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT rayGenAssociationSubobject;
	rayGenAssociationSubobject.AddExport(rayGenExportName);
	rayGenAssociationSubobject.SetSubobjectToAssociate(rayGenLocalRootSigSubobject);
	rayGenAssociationSubobject.AddToStateObject(rtpsoDesc);

	RootSignature globalRootSig;
	globalRootSig.Create(device.Get());
	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT globalRootSigSubobject;
	globalRootSigSubobject.SetRootSignature(globalRootSig.GetInterfacePtr());
	globalRootSigSubobject.AddToStateObject(rtpsoDesc);

	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT rtPipelineConfigSubobject;
	UINT traceRecursionDepth = 2;
	rtPipelineConfigSubobject.Config(traceRecursionDepth);
	rtPipelineConfigSubobject.AddToStateObject(rtpsoDesc);

	device->CreateStateObject(rtpsoDesc, IID_PPV_ARGS(&rtPipelineState));

	// -- shader table
	ComPtr<ID3D12StateObjectProperties> rtPipelineStateProps;
	HRESULT hr = rtPipelineState->QueryInterface(IID_PPV_ARGS(&rtPipelineStateProps));
	assert(SUCCEEDED(hr));

	uint32_t numShaderRecords = 5;
	uint32_t shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	uint32_t shaderRecordSize = shaderIDSize + 8 + 8; // shader id size + root descriptor size + descriptor table size
	shaderRecordSize = ALIGN_TO(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	uint32_t shaderTableSize = shaderRecordSize * numShaderRecords;

	ComPtr<ID3D12Resource> shaderTableBuffer;
	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(shaderTableSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&shaderTableBuffer));
	uint8_t* tableData;
	shaderTableBuffer->Map(0, nullptr, (void**)&tableData);

	// 0
	memcpy(tableData, rtPipelineStateProps->GetShaderIdentifier(rayGenExportName), shaderIDSize);
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(tableData + shaderIDSize) = rtPerFrameDynamicConstantBuffer->GetInstanceGPUVirtualAddress(0, 0);
	*(uint64_t*)(tableData + shaderIDSize + 8) = shaderDescriptorHeap->GetGPUDescriptorHandle(3).ptr;

	// 1
	memcpy(tableData + shaderRecordSize, rtPipelineStateProps->GetShaderIdentifier(missExportName), shaderIDSize);

	// 2
	memcpy(tableData + shaderRecordSize * 2, rtPipelineStateProps->GetShaderIdentifier(shadowMissExportName), shaderIDSize);

	// 3
	memcpy(tableData + shaderRecordSize * 3, rtPipelineStateProps->GetShaderIdentifier(hitGroupExportName), shaderIDSize);
	*(D3D12_GPU_VIRTUAL_ADDRESS*)((tableData + shaderRecordSize * 3) + shaderIDSize) = 
		rtPerFrameDynamicConstantBuffer->GetInstanceGPUVirtualAddress(0, 0);
	*(uint64_t*)((tableData + shaderRecordSize * 3) + shaderIDSize + 8) = shaderDescriptorHeap->GetGPUDescriptorHandle(3).ptr;

	// 4
	memcpy(tableData + shaderRecordSize * 4, rtPipelineStateProps->GetShaderIdentifier(shadowHitGroupExportName), shaderIDSize);

	hr = graphicsCommandAllocators[0]->Reset();
	assert(SUCCEEDED(hr));
	hr = graphicsCommandList->Reset(graphicsCommandAllocators[0].Get(), nullptr);
	assert(SUCCEEDED(hr));

	sphereModels[0]->Commit(graphicsCommandList.Get());
	floorModel->Commit(graphicsCommandList.Get());
	texture->CommitStagedData(graphicsCommandList.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	screenQuadVertexBuffer->CommitStagedData(graphicsCommandList.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	screenQuadIndexBuffer->CommitStagedData(graphicsCommandList.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
	sceneAccelerationStructure->Commit(graphicsCommandList.Get());

	hr = graphicsCommandList->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* onLoadCommandLists[] = { graphicsCommandList.Get() };
	graphicsQueue->ExecuteCommandLists(1, onLoadCommandLists);
	Direct3D::SignalFenceOnGPU(initializationFence->GetInterfacePtr(), graphicsQueue.Get(), initializationFence->Value());

	Direct3D::WaitForFenceValueOnCPU(initializationFence->GetInterfacePtr(), initializationFence->Value(), fenceEvent);
	sphereModels[0]->StagingComplete();
	floorModel->StagingComplete();
	texture->StagingComplete();
	texture->ReleaseData();
	screenQuadVertexBuffer->StagingComplete();
	screenQuadIndexBuffer->StagingComplete();
	window->RegisterOnInputEventCallback(&InputEventCallback);
	gamepad->RegisterOnInputEventCallback(&InputEventCallback);
	bool running = true;
	auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	ID3D12DescriptorHeap* descriptorHeaps[] = { shaderDescriptorHeap->GetInterfacePtr() };
	bool leftSphereTranslatePlus = false;
	bool rightSphereTranslatePlus = true;
	while (running)
	{
		static std::chrono::high_resolution_clock clock;
		static auto startTime = clock.now();
		auto currentTime = clock.now();
		auto frameTime = currentTime - startTime;
		startTime = currentTime;
		float frameTimeDeltaSeconds = frameTime.count() * 1e-9f;

		// update
		running = window->Update();
		if (running == false)
			continue;
		gamepad->Update();

		ProcessInputEventQueue(frameTimeDeltaSeconds);

		const auto& leftPos = sphereModels[1]->GetPosition();
		if (leftPos.z >= 5.f) leftSphereTranslatePlus = false;
		if (leftPos.z <= -5.f) leftSphereTranslatePlus = true;
		float newLeftZ;
		if(leftSphereTranslatePlus)
			newLeftZ = leftPos.z + 2.f * frameTimeDeltaSeconds; 
		else
			newLeftZ = leftPos.z - 2.f * frameTimeDeltaSeconds;

		sphereModels[1]->SetPosition(leftPos.x, leftPos.y, newLeftZ);

		const auto& rightPos = sphereModels[2]->GetPosition();
		if (rightPos.z >= 5.f) rightSphereTranslatePlus = false;
		if (rightPos.z <= -5.f) rightSphereTranslatePlus = true;
		float newRightZ;
		if (rightSphereTranslatePlus)
			newRightZ = rightPos.z + 2.f * frameTimeDeltaSeconds;
		else
			newRightZ = rightPos.z - 2.f * frameTimeDeltaSeconds;

		sphereModels[2]->SetPosition(rightPos.x, rightPos.y, newRightZ);

		sphereModels[0]->SetRotation(sphereModels[0]->GetRotation().x + XMConvertToRadians(0.5f),
			sphereModels[0]->GetRotation().y,
			sphereModels[0]->GetRotation().z + XMConvertToRadians(0.5f));

		XMStoreFloat4x4(&objectData[0].World, sphereModels[0]->GetWorldMatrix());
		XMStoreFloat4x4(&objectData[0].WorldInvTranspose,
			XMMatrixInverse(nullptr, XMMatrixTranspose(sphereModels[0]->GetWorldMatrix())));
		XMStoreFloat4x4(&objectData[1].World, sphereModels[1]->GetWorldMatrix());
		XMStoreFloat4x4(&objectData[1].WorldInvTranspose,
			XMMatrixInverse(nullptr, XMMatrixTranspose(sphereModels[1]->GetWorldMatrix())));
		XMStoreFloat4x4(&objectData[2].World, sphereModels[2]->GetWorldMatrix());
		XMStoreFloat4x4(&objectData[2].WorldInvTranspose,
			XMMatrixInverse(nullptr, XMMatrixTranspose(sphereModels[2]->GetWorldMatrix())));
		XMStoreFloat4x4(&objectData[3].World, floorModel->GetWorldMatrix());
		XMStoreFloat4x4(&objectData[3].WorldInvTranspose,
			XMMatrixInverse(nullptr, XMMatrixTranspose(floorModel->GetWorldMatrix())));

		BuildSceneAccelerationStructure();

		if (window->GetClientWidth() > 0.0f && window->GetClientHeight() > 0.0f)
		{
			XMFLOAT3 cameraForward;
			cameraForward.x = XMScalarCos(XMConvertToRadians(cameraYaw)) * XMScalarCos(XMConvertToRadians(cameraPitch));
			cameraForward.y = XMScalarSin(XMConvertToRadians(cameraPitch));
			cameraForward.z = XMScalarSin(XMConvertToRadians(cameraYaw)) * XMScalarCos(XMConvertToRadians(cameraPitch));
			XMVECTOR cameraForwardVector = XMLoadFloat3(&cameraForward);
			cameraForwardVector = XMVector3Normalize(cameraForwardVector);
			XMStoreFloat3(&cameraForward, cameraForwardVector);

			XMVECTOR cameraRight = XMVector3Cross(XMLoadFloat3(&cameraForward), XMLoadFloat3(&cameraUp));
			XMFLOAT3 right;
			XMStoreFloat3(&right, cameraRight);

			float step = frameTimeDeltaSeconds * cameraSpeed;
			XMVECTOR vStep = XMVectorSet(step, step, step, 0.f);

			if (Input::IsKeyDown(KEY_W))
			{
				XMStoreFloat3(&cameraPos,
					XMVectorAdd(XMLoadFloat3(&cameraPos), XMVectorMultiply(XMLoadFloat3(&cameraForward), vStep)));
			}

			if (Input::IsKeyDown(KEY_S))
			{
				XMStoreFloat3(&cameraPos,
					XMVectorSubtract(XMLoadFloat3(&cameraPos), XMVectorMultiply(XMLoadFloat3(&cameraForward), vStep)));
			}

			if (Input::IsKeyDown(KEY_D))
			{
				XMStoreFloat3(&cameraPos,
					XMVectorSubtract(XMLoadFloat3(&cameraPos), XMVectorMultiply(cameraRight, vStep)));
			}

			if (Input::IsKeyDown(KEY_A))
			{
				XMStoreFloat3(&cameraPos,
					XMVectorAdd(XMLoadFloat3(&cameraPos), XMVectorMultiply(cameraRight, vStep)));
			}

			if (Input::IsKeyDown(KEY_E))
			{
				XMStoreFloat3(&cameraPos,
					XMVectorAdd(XMLoadFloat3(&cameraPos), XMVectorMultiply(XMLoadFloat3(&cameraUp), vStep)));
			}

			if (Input::IsKeyDown(KEY_Q))
			{
				XMStoreFloat3(&cameraPos,
					XMVectorSubtract(XMLoadFloat3(&cameraPos), XMVectorMultiply(XMLoadFloat3(&cameraUp), vStep)));
			}

			XMMATRIX view = XMMatrixLookAtLH(XMLoadFloat3(&cameraPos),
				XMVectorAdd(XMLoadFloat3(&cameraPos), XMLoadFloat3(&cameraForward)),
				XMLoadFloat3(&cameraUp));

			XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.f),
				static_cast<float>(window->GetClientWidth()) / static_cast<float>(window->GetClientHeight()),
				0.1f, 1000.f);

			XMStoreFloat4x4(&perFrameData.ViewProjection, view * proj);

			XMStoreFloat4x4(&rtPerFrameData.inverseView, XMMatrixInverse(nullptr, view));
			XMStoreFloat4x4(&rtPerFrameData.inverseProjection, XMMatrixInverse(nullptr, proj));
		}

		// render
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		auto backBufferIndex = window->GetCurrentBackBufferIndex();
		perFrameDynamicConstantBuffer->Update(backBufferIndex, 0, &perFrameData, sizeof(PerFrameConstantBuffer));
		rtPerFrameDynamicConstantBuffer->Update(0, 0, &rtPerFrameData, sizeof(RTPerFrameConstantBuffer));
		HRESULT hr = graphicsCommandAllocators[backBufferIndex]->Reset();
		assert(SUCCEEDED(hr));
		hr = graphicsCommandList->Reset(graphicsCommandAllocators[backBufferIndex].Get(), nullptr);
		assert(SUCCEEDED(hr));
		
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[backBufferIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// raster scene onto backbuffer render target.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), backBufferIndex, rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
		graphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		graphicsCommandList->RSSetViewports(1, &viewport);
		graphicsCommandList->RSSetScissorRects(1, &scissorRect);
		graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphicsCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		graphicsCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
		graphicsCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		graphicsCommandList->SetPipelineState(graphicsPipeline->Get());
		graphicsCommandList->SetGraphicsRootSignature(rootSignature->GetInterfacePtr());
		graphicsCommandList->SetGraphicsRootConstantBufferView(0, 
			perFrameDynamicConstantBuffer->GetInstanceGPUVirtualAddress(backBufferIndex, 0));
		graphicsCommandList->SetGraphicsRootConstantBufferView(1,
			perFrameDynamicConstantBuffer->GetInstanceGPUVirtualAddress(backBufferIndex, 0));
		graphicsCommandList->SetGraphicsRootDescriptorTable(3, shaderDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		graphicsCommandList->IASetVertexBuffers(0, 1, sphereModels[0]->GetVertexBufferView());
		graphicsCommandList->IASetIndexBuffer(sphereModels[0]->GetIndexBufferView());
		for (uint32_t i = 0; i < 3; i++)
		{
			perObjectDynamicConstantBuffer->Update(backBufferIndex, i, &objectData[i], sizeof(PerObjectConstantBuffer));
			graphicsCommandList->SetGraphicsRootConstantBufferView(2,
				perObjectDynamicConstantBuffer->GetInstanceGPUVirtualAddress(backBufferIndex, i));
			graphicsCommandList->DrawIndexedInstanced(sphereModels[0]->GetNumIndices(), 1, 0, 0, 0);
		}

		graphicsCommandList->IASetVertexBuffers(0, 1, floorModel->GetVertexBufferView());
		graphicsCommandList->IASetIndexBuffer(floorModel->GetIndexBufferView());
		perObjectDynamicConstantBuffer->Update(backBufferIndex, 3, &objectData[3], sizeof(PerObjectConstantBuffer));
		graphicsCommandList->SetGraphicsRootConstantBufferView(2,
			perObjectDynamicConstantBuffer->GetInstanceGPUVirtualAddress(backBufferIndex, 3));
		graphicsCommandList->DrawIndexedInstanced(floorModel->GetNumIndices(), 1, 0, 0, 0);

		// store rastered scene in the GBuffer.
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sceneTextureGBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[backBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

		graphicsCommandList->CopyResource(sceneTextureGBuffer.Get(), renderTargets[backBufferIndex].Get());
		
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(sceneTextureGBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[backBufferIndex].Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// raytrace scene to build shadow map.
		sceneAccelerationStructure->Update(device.Get(), graphicsCommandList.Get());

		D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
		dispatchRaysDesc.Width = window->GetClientWidth();
		dispatchRaysDesc.Height = window->GetClientHeight();
		dispatchRaysDesc.Depth = 1;

		dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = shaderTableBuffer->GetGPUVirtualAddress();
		dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = shaderRecordSize;

		dispatchRaysDesc.MissShaderTable.StartAddress = shaderTableBuffer->GetGPUVirtualAddress() + shaderRecordSize;
		dispatchRaysDesc.MissShaderTable.StrideInBytes = shaderRecordSize;
		dispatchRaysDesc.MissShaderTable.SizeInBytes = shaderRecordSize * 2;

		dispatchRaysDesc.HitGroupTable.StartAddress = shaderTableBuffer->GetGPUVirtualAddress() + shaderRecordSize * 3;
		dispatchRaysDesc.HitGroupTable.StrideInBytes = shaderRecordSize;
		dispatchRaysDesc.HitGroupTable.SizeInBytes = shaderRecordSize * 2;

		graphicsCommandList->SetPipelineState1(rtPipelineState.Get());
		graphicsCommandList->DispatchRays(&dispatchRaysDesc);
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(RTShadowMapOutput.Get()));

		// store raytraced shadow map into GBuffer.
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RTShadowMapOutput.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowMapTextureGBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

		graphicsCommandList->CopyResource(shadowMapTextureGBuffer.Get(), RTShadowMapOutput.Get());

		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(RTShadowMapOutput.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowMapTextureGBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(RTShadowMapOutput.Get()));

		// draw screen quad onto backbuffer rendertarget
		graphicsCommandList->SetPipelineState(FinalPassGraphicsPipeline->Get());
		graphicsCommandList->SetGraphicsRootSignature(FinalPassRootSignature->GetInterfacePtr());
		graphicsCommandList->SetGraphicsRootDescriptorTable(0, shaderDescriptorHeap->GetGPUDescriptorHandle(1));
		graphicsCommandList->IASetVertexBuffers(0, 1, screenQuadVertexBuffer->GetView());
		graphicsCommandList->IASetIndexBuffer(screenQuadIndexBuffer->GetView());
		graphicsCommandList->DrawIndexedInstanced(screenQuadIndices.size(), 1, 0, 0, 0);

		// draw imgui on top of everything
		ImGui::Begin("Settings");
		{
			ImGui::Text("Use 'WASD' to fly camera.\nHold 'Q' to move down.\nHold 'E' to move up.");
			ImGui::InputFloat("Camera Speed", &cameraSpeed, 1.f, 2.f, 3);
			if (cameraSpeed < 1.f) cameraSpeed = 1.f;
			if (cameraSpeed > 200.f) cameraSpeed = 200.f;
			ImGui::Spacing();
			ImGui::Text("Hold down right mouse button\nand move the mouse to look around.");
			ImGui::InputFloat("Look Sensitivity", &cameraLookSensitivity, 1.f, 2.f, 3);
			if (cameraLookSensitivity < 1.f) cameraLookSensitivity = 1.f;
			if (cameraLookSensitivity > 200.f) cameraLookSensitivity = 200.f;
			ImGui::Checkbox("Invert Vertical Look", &cameraInvertVerticalLook);
			ImGui::Spacing();
			ImGui::Text("Press 'F' to toggle fullscreen/windowed.");
			ImGui::Spacing();
			ImGui::Text("Press 'Esc' to exit the application.");
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("Directional light");
			ImGui::DragFloat3("Direction", &perFrameData.Light.Direction.x, 0.01f, -1.f, 1.f);
			ImGui::DragFloat3("Diffuse", &perFrameData.Light.Diffuse.x, 0.01f, 0.f, 1.f);
			ImGui::DragFloat3("Ambient", &perFrameData.Light.Ambient.x, 0.01f, 0.f, 1.f);
			rtPerFrameData.lightDirection = XMFLOAT4(perFrameData.Light.Direction.x, 
				perFrameData.Light.Direction.y, perFrameData.Light.Direction.z, 0.f);
		}
		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsCommandList.Get());

		graphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[backBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		hr = graphicsCommandList->Close();
		assert(SUCCEEDED(hr));
		ID3D12CommandList* commandLists[] = { graphicsCommandList.Get() };
		graphicsQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		Direct3D::SignalFenceOnGPU(backBufferFences[backBufferIndex]->GetInterfacePtr(), graphicsQueue.Get(), 
			backBufferFences[backBufferIndex]->Value());
		Direct3D::WaitForFenceValueOnCPU(backBufferFences[backBufferIndex]->GetInterfacePtr(),
			backBufferFences[backBufferIndex]->Value(), fenceEvent);

		window->PresentFrame();
	}

	for (uint32_t i = 0; i < bufferCount; i++)
	{
		Direct3D::SignalFenceOnGPU(backBufferFences[i]->GetInterfacePtr(), graphicsQueue.Get(), backBufferFences[i]->Value());
		Direct3D::WaitForFenceValueOnCPU(backBufferFences[i]->GetInterfacePtr(), backBufferFences[i]->Value(), fenceEvent);
	}
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#ifdef _DEBUG
	Console::ReleaseConsole();
#endif
	return 0;
}