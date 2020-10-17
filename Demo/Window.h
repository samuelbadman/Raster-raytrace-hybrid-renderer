#pragma once

#include "InputReceiver.h"

class Window : public InputReceiver
{
	typedef std::function<void(const uint32_t newWidth, const uint32_t newHeight)> ResizedCallbackType;

public:
	LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void Initialize(const WCHAR* title, const bool fullscreen);
	void CreateSwapChain(const uint32_t frameBufferCount, ID3D12CommandQueue* const commandQueue);
	uint32_t GetClientWidth() const { return m_clientWidth; }
	uint32_t GetClientHeight() const { return m_clientHeight; }
	void PresentFrame() const;
	void SetVsyncEnabled(const bool vsync);
	void GetSwapChainSurface(const uint32_t index, ID3D12Resource** const ppRenderTarget) const;
	uint32_t GetCurrentBackBufferIndex();
	void ResizeSwapChain(const uint32_t backBufferCount, const uint32_t width, const uint32_t height);
	void RegisterOnResizedCallback(const ResizedCallbackType& callback) { m_onResizedCallback = callback; }
	const DXGI_SAMPLE_DESC& GetSwapChainSampleDesc() const { return m_sampleDesc; }
	const RECT* WindowRect() const { return &m_windowRect; }
	const RECT* ViewportRect() const { return &m_viewportRect; }
	const HWND& GetHWND() const { return m_hWnd; }

	void Close();
	void ToggleFullscreen();
	bool Update();
	void GetMouseCursorPosInViewport(int32_t& x, int32_t& y);

private:
	uint32_t m_clientWidth = 0;
	uint32_t m_clientHeight = 0;
	bool m_fullscreen = false;
	HWND m_hWnd = NULL;
	RECT m_windowRect = {};
	RECT m_lastWindowedRect = {};
	RECT m_viewportRect = {};
	ComPtr<IDXGISwapChain3> m_swapChain;
	DXGI_SAMPLE_DESC m_sampleDesc = { 1,0 };
	bool m_vsync = false;
	bool m_moving = false;
	ResizedCallbackType m_onResizedCallback;
};

