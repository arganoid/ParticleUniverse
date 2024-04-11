#pragma once

#include <string_view>
#include <type_traits>

#include "allegro5/allegro_font.h"

struct ALLEGRO_BITMAP;
struct ALLEGRO_FONT;
struct ALLEGRO_COLOR;

extern ALLEGRO_FONT* font;
extern ALLEGRO_FONT* menuFont;
extern ALLEGRO_FONT* menuFontSmall;
extern ALLEGRO_FONT* menuFontSmaller;
extern ALLEGRO_FONT* menuFontLarge;

class Fonts
{
public:
	static void load();

	static void drawText(ALLEGRO_BITMAP* dest, ALLEGRO_FONT* font, int x, int y, ALLEGRO_COLOR col, std::string_view text, int flags = 0);
	
	template<typename T,
		typename std::enable_if<!std::is_same_v<T, std::string>
							 && !std::is_same_v<T, const char*>>::type* = nullptr>
	static void drawText(ALLEGRO_BITMAP* dest, ALLEGRO_FONT* font, int x, int y, ALLEGRO_COLOR col, T data, int flags = 0)
	{
		std::ostringstream ss;
		ss << data;
		drawText(dest, font, x, y, col, std::string_view(ss.str()), flags);
	}
};
