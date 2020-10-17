#include "stdafx.h"
#include "Window.h"
#include "InputDefinitions.h"

LRESULT CALLBACK MsgRedirect(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Window* const pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	return pWindow->WndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MsgSetup(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_NCCREATE:
		{
			const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
			Window* pWindow = reinterpret_cast<Window*>(pCreate->lpCreateParams);
			assert(pWindow);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MsgRedirect));
			return pWindow->WndProc(hwnd, uMsg, wParam, lParam);
		}
	}
	return 0;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_MOUSEWHEEL:
	{
		const int wheelData = GET_WHEEL_DELTA_WPARAM(wParam);

		if (wheelData > 0)
			m_onInputEventCallback({ MOUSEWHEEL, 0, 1.0f });
		else if (wheelData < 0)
			m_onInputEventCallback({ MOUSEWHEEL, 0, 0.0f });
		return 0;
	}

	case WM_INPUT:
	{
		if (!m_moving)
		{
			UINT dataSize;
			GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));
			if (dataSize > 0)
			{
				std::unique_ptr<BYTE[]> rawData = std::make_unique<BYTE[]>(dataSize);
				if (GetRawInputData(
					reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawData.get(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
				{
					RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawData.get());
					if (raw->header.dwType == RIM_TYPEMOUSE)
					{
						m_onInputEventCallback({ MOUSE_X_AXIS, 0, (float)raw->data.mouse.lLastX });
						m_onInputEventCallback({ MOUSE_Y_AXIS, 0, (float)raw->data.mouse.lLastY });
					}
				}
			}
		}
		return 0;
	}

	case WM_LBUTTONDOWN:
		m_onInputEventCallback({ (int)MK_LBUTTON, 0, 1.0f });
		return 0;

	case WM_RBUTTONDOWN:
		m_onInputEventCallback({ (int)MK_RBUTTON, 0, 1.0f });
		return 0;

	case WM_MBUTTONDOWN:
		m_onInputEventCallback({ (int)MK_MBUTTON, 0, 1.0f });
		return 0;

	case WM_LBUTTONUP:
		m_onInputEventCallback({ (int)MK_LBUTTON, 0, 0.0f });
		return 0;

	case WM_RBUTTONUP:
		m_onInputEventCallback({ (int)MK_RBUTTON, 0, 0.0f });
		return 0;

	case WM_MBUTTONUP:
		m_onInputEventCallback({ (int)MK_MBUTTON, 0, 0.0f });
		return 0;

	case WM_KEYDOWN:
		if (lParam & 0x40000000)
		{
			// keydown message is a repeat key
			m_onInputEventCallback({ (int)wParam, 0, 1.0f, true });
			return 0;
		}
		else
		{
			m_onInputEventCallback({ (int)wParam, 0, 1.0f, false });
			return 0;
		}

	case WM_KEYUP:
		m_onInputEventCallback({ (int)wParam, 0, 0.0f });
		return 0;

	case WM_SIZE:
	{
		RECT clientRect = {};
		GetClientRect(hWnd, &clientRect);
		uint32_t newWidth = clientRect.right - clientRect.left;
		uint32_t newHeight = clientRect.bottom - clientRect.top;
		if ((newWidth > 0 && newHeight > 0) && (newWidth != m_clientWidth || newHeight != m_clientHeight))
		{
			if (m_swapChain)
			{
				m_clientWidth = newWidth;
				m_clientHeight = newHeight;
				POINT point = { clientRect.top, clientRect.left };
				ClientToScreen(m_hWnd, &point);
				m_viewportRect.top = point.y;
				m_viewportRect.left = point.x;
				m_viewportRect.right = m_viewportRect.left + m_clientWidth;
				m_viewportRect.bottom = m_viewportRect.top + m_clientHeight;
				GetWindowRect(hWnd, &m_windowRect);
				m_onResizedCallback(newWidth, newHeight);
			}
		}
		return 0;
	}

	case WM_ENTERSIZEMOVE:
		m_moving = true;
		return 0;

	case WM_EXITSIZEMOVE:
		m_moving = false;
		return 0;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Window::PresentFrame() const
{
	HRESULT hr = m_swapChain->Present(m_vsync ? 1 : 0, 0);
	assert(SUCCEEDED(hr));
}

void Window::SetVsyncEnabled(const bool vsync)
{
	m_vsync = vsync;
}

void Window::GetSwapChainSurface(const uint32_t index, ID3D12Resource** const ppRenderTarget) const
{
	HRESULT hr = m_swapChain->GetBuffer(index, IID_PPV_ARGS(ppRenderTarget));
	assert(SUCCEEDED(hr));
}

uint32_t Window::GetCurrentBackBufferIndex()
{
	return m_swapChain->GetCurrentBackBufferIndex();
}

void Window::Close()
{
	PostMessage(m_hWnd, WM_CLOSE, 0, 0);
}

void Window::ToggleFullscreen()
{
	if (!m_fullscreen)
	{
		// go to fullscreen
		GetWindowRect(m_hWnd, &m_lastWindowedRect);
		HMONITOR hmon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hmon, &mi);
		m_clientWidth = mi.rcMonitor.right - mi.rcMonitor.left;
		m_clientHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
		SetWindowPos(m_hWnd,
			HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, m_clientWidth, m_clientHeight,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
		SetWindowLong(m_hWnd, GWL_STYLE, 0);
		// using SW_MAXIMIZE sends an extra WM_SIZE message calling on resized callback
		ShowWindow(m_hWnd, SW_MAXIMIZE);
	}
	else
	{
		// return to last windowed rect
		m_clientWidth = m_lastWindowedRect.right - m_lastWindowedRect.left;
		m_clientHeight = m_lastWindowedRect.bottom - m_lastWindowedRect.top;
		SetWindowPos(m_hWnd,
			HWND_TOP, m_lastWindowedRect.left, m_lastWindowedRect.top, m_clientWidth, m_clientHeight,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
		SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		ShowWindow(m_hWnd, SW_NORMAL);
	}
	m_fullscreen = !m_fullscreen;
}

bool Window::Update()
{
	static MSG msg = {};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
		{
			return false;
			break;
		}
	}
	return true;
}

void Window::GetMouseCursorPosInViewport(int32_t& x, int32_t& y)
{
	POINT point = {};
	if (GetCursorPos(&point))
	{
		if (ScreenToClient(m_hWnd, &point))
		{
			x = point.x;
			y = point.y;
		}
	}
}

void Window::Initialize(const WCHAR* title, const bool fullscreen)
{
	static LPCTSTR windowName = L"window";

	m_clientWidth = 1280;
	m_clientHeight = 720;

	// Note : Use hinstance parameter from winmain if creating the window in a DLL.
	auto hInstance = GetModuleHandle(NULL); 

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MsgSetup;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = windowName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		std::cout << "Failed to register window class" << std::endl;
	}
	else
	{
		m_hWnd = CreateWindowEx(
			NULL,
			windowName,
			title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			m_clientWidth, m_clientHeight,
			NULL,
			NULL,
			hInstance,
			this);

		assert(m_hWnd != NULL);
		// remove window style if fullscreen is true
		if (m_fullscreen)
			SetWindowLong(m_hWnd, GWL_STYLE, 0);

		ShowWindow(m_hWnd, SW_SHOW);
		UpdateWindow(m_hWnd);
	}

	RECT clientRect = {};
	GetClientRect(m_hWnd, &clientRect);
	m_clientWidth = clientRect.right - clientRect.left;
	m_clientHeight = clientRect.bottom - clientRect.top;
	POINT point = { clientRect.top, clientRect.left };
	ClientToScreen(m_hWnd, &point);
	m_viewportRect.top = point.y;
	m_viewportRect.left = point.x;
	m_viewportRect.right = m_viewportRect.left + m_clientWidth;
	m_viewportRect.bottom = m_viewportRect.top + m_clientHeight;
	GetWindowRect(m_hWnd, &m_windowRect);

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01; // Mouse.
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = NULL;
	BOOL registeredRawDevices = RegisterRawInputDevices(&rid, 1, sizeof(rid));
	assert(registeredRawDevices);

	if (fullscreen)
		ToggleFullscreen();
}

void Window::CreateSwapChain(const uint32_t frameBufferCount, ID3D12CommandQueue* const commandQueue)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#ifdef _DEBUG
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = m_clientWidth;
	scDesc.Height = m_clientHeight;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.Stereo = FALSE;
	scDesc.SampleDesc = m_sampleDesc;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = frameBufferCount;
	scDesc.Scaling = DXGI_SCALING_STRETCH;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	scDesc.Flags = 0;
	ComPtr<IDXGISwapChain1> tempSwapChain;
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, m_hWnd, &scDesc, nullptr, nullptr, &tempSwapChain);
	assert(SUCCEEDED(hr));
	hr = dxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_WINDOW_CHANGES);
	assert(SUCCEEDED(hr));
	hr = tempSwapChain.As(&m_swapChain);
	assert(SUCCEEDED(hr));
}

void Window::ResizeSwapChain(const uint32_t backBufferCount, const uint32_t width, const uint32_t height)
{
	DXGI_SWAP_CHAIN_DESC scDesc = {};
	HRESULT hr = m_swapChain->GetDesc(&scDesc);
	assert(SUCCEEDED(hr));
	hr = m_swapChain->ResizeBuffers(backBufferCount, width, height, scDesc.BufferDesc.Format, scDesc.Flags);
	assert(SUCCEEDED(hr));
}
