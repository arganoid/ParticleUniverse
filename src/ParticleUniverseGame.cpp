#include "ParticleUniverseGame.h"

#include "ARGCore\ARGUtils.h"

#include <allegro5/allegro.h>


ParticleUniverseGame::ParticleUniverseGame() :
	GameBase(true, 0)
{
	m_universe = std::make_unique<Universe>();
}

void ParticleUniverseGame::Advance(float _deltaTime)
{
	m_universe->Advance(_deltaTime);
}

void ParticleUniverseGame::Render()
{
	static ALLEGRO_COLOR black = al_map_rgb(0, 0, 0);
	al_clear_to_color(black);
	m_universe->Render();
}

void ParticleUniverseGame::OnClose()
{
	m_universe->OnClose();
}

