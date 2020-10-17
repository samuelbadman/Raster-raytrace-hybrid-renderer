#pragma once

struct InputEvent
{
	int input;
	int port;
	float data;
	bool repeatKey;
	bool consumed;

	InputEvent() : input(-1), port(0), data(0), repeatKey(false), consumed(false) {}
	InputEvent(int i, int p, float d, bool r) : input(i), port(p), data(d), repeatKey(r), consumed(false) {}
	InputEvent(int i, int p, float d) : input(i), port(p), data(d), repeatKey(false), consumed(false) {}
};