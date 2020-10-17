#pragma once

#include "InputEvent.h"

class InputReceiver
{
	typedef std::function<void(const InputEvent&)> inputEventCallback;

public:
	void RegisterOnInputEventCallback(const inputEventCallback& callback) { m_onInputEventCallback = callback; }

protected:
	inputEventCallback m_onInputEventCallback;
};

