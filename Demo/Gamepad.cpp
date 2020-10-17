#include "stdafx.h"
#include "Gamepad.h"
#include "InputDefinitions.h"

Gamepad::Gamepad()
{
	for (int i = 0; i < maxGamepads; i++)
		m_lastStates[i] = {};
}

void Gamepad::Update()
{
	for (DWORD port = 0; port < maxGamepads; port++)
	{
		XINPUT_STATE state = {};
		if (XInputGetState(port, &state) == ERROR_SUCCESS)
		{
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_A);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_B);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_X);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_Y);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_DPAD_UP);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_DPAD_DOWN);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_DPAD_LEFT);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_DPAD_RIGHT);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_LEFT_THUMB);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_RIGHT_THUMB);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_BACK);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_START);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_LEFT_SHOULDER);
			CreateButtonEvent((int)port, state, XINPUT_GAMEPAD_RIGHT_SHOULDER);

			float x, y;
			CalculateStickAxes(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY, x, y);
			m_onInputEventCallback({ GAMEPAD_THUMBSTICK_LEFT_X_AXIS, (int)port, x });
			m_onInputEventCallback({ GAMEPAD_THUMBSTICK_LEFT_Y_AXIS, (int)port, y });

			CalculateStickAxes(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY, x, y);
			m_onInputEventCallback({ GAMEPAD_THUMBSTICK_RIGHT_X_AXIS, (int)port, x });
			m_onInputEventCallback({ GAMEPAD_THUMBSTICK_RIGHT_Y_AXIS, (int)port, y });

			m_onInputEventCallback({ GAMEPAD_TRIGGER_LEFT, (int)port, (float)state.Gamepad.bLeftTrigger / 255 });
			m_onInputEventCallback({ GAMEPAD_TRIGGER_RIGHT, (int)port, (float)state.Gamepad.bRightTrigger / 255 });

			m_lastStates[port] = state;
		}
	}
}

void Gamepad::CreateButtonEvent(int port, const XINPUT_STATE& currentState, int button)
{
	auto buttonStateCurrent = currentState.Gamepad.wButtons & button;
	auto buttonStateLast = m_lastStates[port].Gamepad.wButtons & button;
	if (buttonStateCurrent != buttonStateLast)
	{
		if (buttonStateCurrent != 0)
			m_onInputEventCallback({ button, port, 1.0f });
		else
			m_onInputEventCallback({ button, port, 0.0f });
	}
}

void Gamepad::CalculateStickAxes(const short thumbX, const short thumbY, float& x, float& y)
{
	float normLX = fmaxf(-1.0f, (float)thumbX / 32767);
	float normLY = fmaxf(-1.0f, (float)thumbY / 32767);
	x = (abs(normLX) < m_deadzone ? 0.0f : (abs(normLX) - m_deadzone) * (normLX / abs(normLX)));
	y = (abs(normLY) < m_deadzone ? 0.0f : (abs(normLY) - m_deadzone) * (normLY / abs(normLY)));
	if (m_deadzone > 0) x /= 1 - m_deadzone;
	if (m_deadzone > 0) y /= 1 - m_deadzone;
}
