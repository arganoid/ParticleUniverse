#pragma once

#include "TimingManager.h"

#include <string>
#include <list>
#include <cmath>

struct ALLEGRO_EVENT_QUEUE;

class GameBase
{
public:
	GameBase(bool _requireMouse, int _advanceLimit);
	void MainLoop();

protected:
	virtual void Advance(float _deltaTime) = 0;
	virtual void Render() = 0;

	TimingManager* m_timingManager;

private:
	void ScreenSwap();

	bool m_showFPS = false;
	int m_advanceLimit = 0;
	ALLEGRO_EVENT_QUEUE* m_eventQueue = nullptr;
};
