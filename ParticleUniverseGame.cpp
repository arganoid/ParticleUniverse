#include "ParticleUniverseGame.h"

#include "ARGCoreSrc\ARGUtils.h"

#include <allegro5/allegro.h>


ParticleUniverseGame::ParticleUniverseGame() :
	GameBase(true, 0)
{
	m_universe = std::make_unique<Universe>(300000);
}

void ParticleUniverseGame::Advance(float _deltaTime)
{
	int times = m_universe->GetFastForward() ? 100 : 1;
	for(int i = 0; i < times; ++i)
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

