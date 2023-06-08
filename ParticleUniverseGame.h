#pragma once

#include <memory>

#include "ARGCoreSrc\GameBase.h"

#include "Universe.h"

class ParticleUniverseGame : public GameBase
{
public:
	ParticleUniverseGame();
	~ParticleUniverseGame();

protected:
	void Advance(float _deltaTime);
	void Render();

private:
	std::unique_ptr<Universe> m_universe;
};
