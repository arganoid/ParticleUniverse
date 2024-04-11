#pragma once

#include "allegro5/allegro.h"

struct RGB
{
	uint8_t r = 0, g = 0, b = 0;

	RGB() = default;
	RGB(uint8_t r, uint8_t g, uint8_t b)
	{
		this->r = r;
		this->g = g;
		this->b = b;
	}
	RGB(ALLEGRO_COLOR c)
	{
		unsigned char r, g, b;
		al_unmap_rgb(c, &r, &g, &b);
		this->r = r;
		this->g = g;
		this->b = b;
	}

	operator ALLEGRO_COLOR() const
	{
		return al_map_rgb(r, g, b);
	}
};
