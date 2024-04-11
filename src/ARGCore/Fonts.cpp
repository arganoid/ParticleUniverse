#include <cstdarg>
#include <stdexcept>

#include "Fonts.h"
#include "Sprites.h"
#include "ARGUtils.h"

#include "allegro5/allegro_font.h"
#include "allegro5/allegro_ttf.h"

ALLEGRO_FONT* font = nullptr;
//ALLEGRO_FONT* menuFont = nullptr;
//ALLEGRO_FONT* menuFontSmall = nullptr;
//ALLEGRO_FONT* menuFontSmaller = nullptr;
//ALLEGRO_FONT* menuFontLarge = nullptr;

void Fonts::load()
{
	assert(al_is_font_addon_initialized());
	assert(al_is_ttf_addon_initialized());
	font = al_create_builtin_font(); // al_load_ttf_font("fonts\\SourceCodePro-Regular.ttf", 12, ALLEGRO_TTF_MONOCHROME);
	//menuFont = al_load_ttf_font("fonts\\OCRA.ttf", 30, ALLEGRO_TTF_MONOCHROME);
	//menuFontSmall = al_load_ttf_font("fonts\\OCRA.ttf", 24, ALLEGRO_TTF_MONOCHROME);
	//menuFontSmaller = al_load_ttf_font("fonts\\OCRA.ttf", 20, ALLEGRO_TTF_MONOCHROME);
	//menuFontLarge = al_load_ttf_font("fonts\\OCRA.ttf", 64, ALLEGRO_TTF_MONOCHROME);
	if (!font) // || !menuFont || !menuFontSmall || !menuFontSmaller || !menuFontLarge)
		throw std::runtime_error("Error loading fonts!");
}

void Fonts::drawText(ALLEGRO_BITMAP* dest, ALLEGRO_FONT* font, int x, int y, ALLEGRO_COLOR col, std::string_view text, int flags)
{
	if (dest != nullptr)
		Sprites::setTargetBitmap(dest);
	al_draw_text(font, col, x, y, flags, text.data());
}
