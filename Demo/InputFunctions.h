#pragma once
#include "stdafx.h"

namespace Input
{
	static bool IsKeyDown(int key)
	{
		SHORT state = GetAsyncKeyState(key);
		if ((1 << 15) & state) return true;
		return false;
	}

	static bool IsMouseButtonDown(int button)
	{
		if (GetKeyState(button) < 0) return true;
		return false;
	}

	static bool IsGamepadButtonDown(int port, int button)
	{
		XINPUT_STATE state = { 0 };
		DWORD found = XInputGetState(port, &state);
		assert(found == ERROR_SUCCESS);
		auto buttonState = (state.Gamepad.wButtons & button);
		if (buttonState != 0)
			return true;
		return false;
	}

	static void GetMouseCursorPosOnPlatform(int32_t& x, int32_t& y)
	{
		POINT point = {};
		if (GetCursorPos(&point))
		{
			x = point.x;
			y = point.y;
		}
	}
}