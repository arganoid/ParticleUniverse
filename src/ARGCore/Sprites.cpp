#include <cmath>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"

#include "Sprites.h"

#if 0
#define LOGBITMAPBPP(src,dest) Log("BPP: " + to_string(al_get_pixel_format_bits(al_get_bitmap_format((src)))) + " -> " + to_string(al_get_pixel_format_bits(al_get_bitmap_format((dest)))) + "\n")
#define LOGBITMAPFLAGS(src,dest) Log("Flags: " + to_string(al_get_bitmap_flags((src))) + " -> " + to_string(al_get_bitmap_flags((dest))) + "\n")
#else
#define LOGBITMAPBPP(src,dest)
#define LOGBITMAPFLAGS(src,dest)
#endif

// ALLEGRO_ALPHA_TEST is only for memory bitmaps

// For screen or temp bitmaps
const int newBitmapFlagsNoPreserveContents = ALLEGRO_NO_PRESERVE_TEXTURE | ALLEGRO_CONVERT_BITMAP;

const int newBitmapFlags = ALLEGRO_CONVERT_BITMAP;
const int newBitmapFormat = ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA;

void Sprites::setTargetBitmap(ALLEGRO_BITMAP* dest)
{
#if SET_TARGET_BITMAP_OPTIMISATION
	if (dest != targetBitmap)
#endif
	{
		targetBitmap = dest;
		al_set_target_bitmap(dest);
	}
}

// Bitmap -> Bitmap

inline void checkMemoryBitmap(ALLEGRO_BITMAP* bmp)
{
	assert((al_get_bitmap_flags(bmp) & ALLEGRO_MEMORY_BITMAP) == 0);
	if (al_get_bitmap_flags(bmp) & ALLEGRO_MEMORY_BITMAP)
		al_convert_bitmap(bmp);
}

void Sprites::draw(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int x, int y, int flags, int opacity)
{
	checkMemoryBitmap(dest);
	checkMemoryBitmap(src);

	int allegroFlags = 0;
	if (flags & (int)Flags::HFlip)
		allegroFlags |= ALLEGRO_FLIP_HORIZONTAL;

	//timingManager.BeginProfileSection("sprite");
	LOGBITMAPFLAGS(src, dest);
	LOGBITMAPBPP(src, dest);

	setTargetBitmap(dest);

	if (opacity == 255)
		al_draw_bitmap(src, x, y, allegroFlags);
	else
		al_draw_tinted_bitmap(src, al_map_rgba(opacity,opacity,opacity,opacity), x, y, allegroFlags);

	++s_numDraws;
	//auto time = timingManager.EndProfileSection("sprite");
	//Log("%#010x %#010x %d %d %.4f\n", src, dest, al_get_bitmap_width(src), al_get_bitmap_height(src), time * 1000);
}

//void Sprites::blit(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int sx, int sy, int dx, int dy)
//{
//	LOGBITMAPFLAGS(src, dest);
//#pragma message("al4 blit parameters don't map to al5")
//	
//	setTargetBitmap(dest);
//	al_draw_bitmap_region(src, sx, sy, dw, dh, dx, dy, 0);// todo use al_set_clipping_rectangle, al_reset_clipping_rectangle
//	++s_numDraws;
//}

void Sprites::stretchBlit(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh)
{
	checkMemoryBitmap(dest);
	checkMemoryBitmap(src);

	LOGBITMAPFLAGS(src, dest);
	setTargetBitmap(dest);
	al_draw_scaled_bitmap(src, sx, sy, sw, sh, dx, dy, dw, dh, 0);
	++s_numDraws;
}

void Sprites::rotateScaledSprite(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int cx, int cy, int dx, int dy, float angle, float scale, int opacity)
{
	checkMemoryBitmap(dest);
	checkMemoryBitmap(src);

	setTargetBitmap(dest);

	angle /= 40.58451049f;	// AL4 used angles 0-255!

	if (opacity == 255)
		al_draw_scaled_rotated_bitmap(src, cx, cy, dx, dy, scale, scale, angle, 0);
	else
		al_draw_tinted_scaled_rotated_bitmap(src, al_map_rgba(opacity, opacity, opacity, opacity), cx,cy, dx, dy, scale, scale, angle, 0);
}

void Sprites::drawLit(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int x, int y, int r, int g, int b, int a)
{
	checkMemoryBitmap(dest);
	checkMemoryBitmap(src);

	setTargetBitmap(dest);
	al_draw_tinted_bitmap(src, al_map_rgba(r, g, b, a), x, y, 0);
}


// Other

int Sprites::getr(ALLEGRO_COLOR c)
{
	unsigned char r, g, b;
	al_unmap_rgb(c, &r, &g, &b);
	return r;
}
int Sprites::getg(ALLEGRO_COLOR c)
{
	unsigned char r, g, b;
	al_unmap_rgb(c, &r, &g, &b);
	return g;
}
int Sprites::getb(ALLEGRO_COLOR c)
{
	unsigned char r, g, b;
	al_unmap_rgb(c, &r, &g, &b);
	return b;
}

// https://stackoverflow.com/questions/9018016/how-to-compare-two-colors-for-similarity-difference
float Sprites::ColourDistance(ALLEGRO_COLOR e1, ALLEGRO_COLOR e2)
{
	unsigned char r1, g1, b1, r2, g2, b2;
	al_unmap_rgb(e1, &r1, &g1, &b1);
	al_unmap_rgb(e2, &r2, &g2, &b2);
	long rmean = ((long)r1 + (long)r2) / 2;
	long r = (long)r1 - (long)r2;
	long g = (long)g1 - (long)g2;
	long b = (long)b1 - (long)b2;
	return std::sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8));
}

BitmapWrapper::BitmapWrapper(int w, int h, bool preserveContents)
{
	//Log("BitmapWrapper create %X\n", this);
	al_set_new_bitmap_flags(preserveContents ? newBitmapFlags : newBitmapFlagsNoPreserveContents);
	al_set_new_bitmap_format(newBitmapFormat);
	ptr = al_create_bitmap(w, h);
	//Log("Bitmap created %X\n", ptr);
}


BitmapWrapper::BitmapWrapper(BitmapWrapper&& other) noexcept
{
	ptr = other.ptr;
	other.ptr = nullptr;
}

BitmapWrapper::BitmapWrapper(ALLEGRO_BITMAP* bmp, bool takeOwnership) noexcept :
	ptr(bmp),
	owned(takeOwnership)
{
}

BitmapWrapper::~BitmapWrapper()
{
	if (owned)
	{
		//Log("BitmapWrapper destroy %X, %X\n", this, ptr);
		al_destroy_bitmap(ptr);
		//Log("Bitmap destroyed %X\n", ptr);
	}
}

int BitmapWrapper::w() const { return al_get_bitmap_width(ptr); }
int BitmapWrapper::h() const { return al_get_bitmap_height(ptr); }

void BitmapWrapper::clear()
{
	Sprites::setTargetBitmap(ptr);
	al_clear_to_color(al_map_rgb(0, 0, 0));
}

void BitmapWrapper::clear(ALLEGRO_COLOR col)
{
	Sprites::setTargetBitmap(ptr);
	al_clear_to_color(col);
}

void BitmapWrapper::draw(BitmapWrapper const& dest, int x, int y, int opacity) const
{
	Sprites::draw(dest.get(), ptr, x, y, 0, opacity);
}
