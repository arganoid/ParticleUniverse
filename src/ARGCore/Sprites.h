#pragma once

// for ALLEGRO_COLOR, not ideal having this here
#include "allegro5/allegro.h"

class BitmapWrapper;
struct ALLEGRO_BITMAP;
struct ALLEGRO_COLOR;

class Sprites
{
	friend class BitmapWrapper;
	static inline ALLEGRO_BITMAP* targetBitmap = nullptr;

public:
	static inline int s_numDraws = 0;

	enum class Flags
	{
		HFlip = 1,
	};

public:
	static void setTargetBitmap(ALLEGRO_BITMAP* dest);

	// Bitmap -> Bitmap
	static void draw(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int x, int y, int flags = 0, int opacity = 255);
	static void drawLit(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int x, int y, int r, int g, int b, int a);
	static void rotateScaledSprite(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int cx, int cy, int dx, int dy, float angle, float scale, int opacity = 255);
	//static void blit(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int sx, int sy, int dx, int dy);
	static void stretchBlit(ALLEGRO_BITMAP* dest, ALLEGRO_BITMAP* src, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);

	// Other
	static int getr(ALLEGRO_COLOR c);
	static int getg(ALLEGRO_COLOR c);
	static int getb(ALLEGRO_COLOR c);
	static float ColourDistance(ALLEGRO_COLOR e1, ALLEGRO_COLOR e2);

	// alternatively...
	//template<typename ...Args>
	//static void rotateScaledSprite(BitmapWrapper const& dest, BitmapWrapper const& src, Args&& ... args)
	//{
	//	rotateScaledSprite(dest.get(), src.get(), std::forward<Args>(args) ...);
	//}
};

class BitmapWrapper
{
	ALLEGRO_BITMAP* ptr;
	bool owned = true;

public:
	BitmapWrapper(int w, int h, bool preserveContents = true);
	BitmapWrapper(BitmapWrapper const& other) = delete;
	BitmapWrapper(BitmapWrapper&& other) noexcept;
	BitmapWrapper(ALLEGRO_BITMAP* bmp, bool takeOwnership) noexcept;
	~BitmapWrapper();

	ALLEGRO_BITMAP* get() const { return ptr; };
	operator ALLEGRO_BITMAP*() const { return ptr;  }

	int w() const;
	int h() const;

	void clear();
	void clear(ALLEGRO_COLOR col);

	void draw(BitmapWrapper const& dest, int x, int y, int opacity = 255) const;
};
