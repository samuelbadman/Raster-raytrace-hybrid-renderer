#pragma once

#include "stdafx.h"
#include "InputReceiver.h"

class Gamepad : public InputReceiver
{
public:
	static const int maxGamepads = 4;

public:
	Gamepad();

	void Update();

private:
	void CreateButtonEvent(int port, const XINPUT_STATE& currentState, int button);
	void CalculateStickAxes(const short thumbX, const short thumbY, float& x, float& y);

private:
	XINPUT_STATE m_lastStates[maxGamepads];
	const float m_deadzone = 0.24f;
};

