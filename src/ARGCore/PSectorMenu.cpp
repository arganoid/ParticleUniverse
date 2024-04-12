#include "PSectorMenu.h"
#include "Fonts.h"
#include "Keyboard.h"
#include "Sprites.h"
#include "rgb.h"
#include "ARGUtils.h"

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

#include <assert.h>

#include <memory>

//extern int sfxVolume255;

using namespace std;

std::unordered_set<int> PSectorMenu::held;

PSectorMenu::PSectorMenu(PSectorMenuLayoutOptions const& options, std::function<void()> onExit) :
	layoutOptions(options),
	font(options.font),
	startY(options.startY),
	nextY(options.startY),
	yInc(options.yInc),
	buttonHeight(options.buttonHeight),
	scale(options.scale),
	onExit(onExit)
{
}

PSectorMenu::~PSectorMenu()
{
	onExit();
}

void PSectorMenu::update(int maxY)
{
	if (cmdMenu.focusButton()->isHeading())
		cmdMenu.movedown();

	// Update scroll
	{
		auto focusButton = cmdMenu.focusButton();
		if (focusButton->y2 + scrollY > maxY)
		{
			scrollY = maxY - focusButton->y2;
		}
		else if (focusButton->y1 + scrollY < layoutOptions.startY)
		{
			scrollY = layoutOptions.startY - focusButton->y1;
		}

		showScrollUp = showScrollDown = false;
		for (auto const& button : cmdMenu.buttons)
		{
			if (button->y2 + scrollY > maxY)
				showScrollDown = true;
			if (button->y2 + scrollY < layoutOptions.startY)
				showScrollUp = true;
		}
	}

	// Detect keypresses
	const int initialKeyDelay = 10;
	static int currentKeyDelay = initialKeyDelay;
	static int counter = 0;
	if (key_delay > 0)
		key_delay--;			// todo make frame rate independent
	if (key_delay == 0)
	{
		const bool inGame = layoutOptions.inGame;
		int xDir = checkDirection(ALLEGRO_KEY_LEFT, inGame) ? -1 : (checkDirection(ALLEGRO_KEY_RIGHT, inGame) ? 1 : 0);
		int yDir = checkDirection(ALLEGRO_KEY_UP, inGame) ? -1 : (checkDirection(ALLEGRO_KEY_DOWN, inGame) ? 1 : 0);
		if (xDir != 0 || yDir != 0)
		{
			if (yDir != 0)
			{
				if (yDir == -1)
					cmdMenu.moveup();
				else
					cmdMenu.movedown();
				key_delay = currentKeyDelay;
			}
			else if (xDir != 0)
			{
				if (xDir == -1)
					cmdMenu.left();
				else
					cmdMenu.right();
				key_delay = layoutOptions.leftRightKeyDelayOverride.has_value() ? layoutOptions.leftRightKeyDelayOverride.value() : currentKeyDelay;
			}
			if (currentKeyDelay > 2 && ++counter % 20 == 0)
				--currentKeyDelay;
			//Sound::play(NewSound::Slide, sfxVolume255, 128, yDir != 0 ? 1000 : 2000);
		}
		else
		{
			currentKeyDelay = initialKeyDelay;
			counter = 0;
		}

		if (checkButtonNoRepeat(ALLEGRO_KEY_ENTER, layoutOptions.inGame))
		{
			//Sound::play(NewSound::Slide, sfxVolume255, 128, 2000);
			cmdMenu.select();		// warning: previously did menuStack.pop in some cases, causing this to be deleted! 
			key_delay = initialKeyDelay;
		}

		if (Keyboard::keyPressed(ALLEGRO_KEY_DELETE))
		{
			//Sound::play(NewSound::Slide, sfxVolume255, 128, 2000);
			cmdMenu.deletePressed();
			key_delay = initialKeyDelay;
		}
	}
}

void PSectorMenu::addInt(int x, string const& txt, int* value, int minValue, int maxValue, int step)
{
	cmdMenu.addInt(x, nextY, 344, nextY + buttonHeight, txt, value, minValue, maxValue, step);
	nextY += yInc;
}

void PSectorMenu::addEnum(int x, string const& txt, int* value, int minValue, int maxValue, shared_ptr<vector<string>> toStrLookup) {
	cmdMenu.addEnum(x, nextY, 344, nextY + buttonHeight, txt, value, minValue, maxValue, toStrLookup);
	nextY += yInc;
}

void PSectorMenu::addBool(int x, string const& txt, bool* value, shared_ptr<vector<string>> toStrLookup)
{
	cmdMenu.addBool(x, nextY, 344, nextY + buttonHeight, txt, value, toStrLookup);
	nextY += yInc;
}

std::shared_ptr<CmdButtonAction> PSectorMenu::addAction(int x, DialogTextSource txt, std::function<void()> action, std::function<void(CmdButtonAction&)> deleteAction, RGB* col)
{
	return addAction(x, txt, [action](CmdButtonAction&) { action(); }, deleteAction, col);
}

std::shared_ptr<CmdButtonAction> PSectorMenu::addAction(int x, DialogTextSource txt, std::function<void(CmdButtonAction&)> action, std::function<void(CmdButtonAction&)> deleteAction, RGB* col)
{
	auto button = cmdMenu.addAction(x, nextY, 344, nextY + buttonHeight, txt, action, deleteAction);
	cmdMenu.buttons.back()->setCol(col);
	nextY += yInc;
	return button;
}

void PSectorMenu::addHeading(int x, string const& text)
{
	cmdMenu.addHeading(x, nextY, 344, nextY + buttonHeight, text);
	nextY += yInc;
}

void PSectorMenu::addGap()
{
	nextY += yInc/2;
}

extern ALLEGRO_DISPLAY* g_display;

//void PSectorMenu::render(BitmapWrapper const& activePage)
void PSectorMenu::render()
{
	//assert((al_get_bitmap_flags(activePage.get()) & ALLEGRO_MEMORY_BITMAP) == 0);

	//Sprites::setTargetBitmap(activePage.get());

	//al_set_clipping_rectangle(0, layoutOptions.startY, activePage.w(), activePage.h() - layoutOptions.startY);
	int scWidth = al_get_display_width(g_display);

	RGB textCol = RGB(200, 255, 200);
	RGB headingCol = RGB(150, 200, 150);

	//int xCen = activePage.w() / 2;
	int xCen = scWidth / 2;

	// Draw dialog boxes
	for (size_t menucount = 0; menucount < cmdMenu.buttons.size(); menucount++)
	{
		auto const& button = cmdMenu.buttons[menucount];
		
		if (menucount == cmdMenu.getFocusIndex())
			al_draw_filled_rectangle(0, button->y1 + scrollY, scWidth - 1, button->y2 + scrollY, al_map_rgb(64, 64, 64));

		string str = button->getText();
		if (!button->valueStr().empty())
		{
			str += ": " + button->valueStr();
		}

		int textX = button->x1 + 3;
		int textY = button->y1 + scrollY;
		
		ALLEGRO_COLOR col = button->getCol() ? *(button->getCol()):
			(button->isHeading() ? headingCol : textCol);
		
		if (scale == 1)
			Fonts::drawText(nullptr, font, textX, textY, col, str);
		else
		{
#if 0
			clear_bitmap(scaledTextBMP);
			textout(scaledTextBMP, font, str.c_str(), 0, 0, col);
			stretch_blit(scaledTextBMP, activePage, 0, 0, scaledTextBMP->w, scaledTextBMP->h, textX, textY, scaledTextBMP->w * scale, scaledTextBMP->h * scale);
#endif
		}
	}

#if 0
	const int scrollArrowsX = cmdMenu.buttons.empty() ? 100 : std::max(cmdMenu.buttons[0]->x1 - 100, 0);
	const int scrollArrowsW = 50;
	const int scrollArrowsH = 50;

	if (showScrollUp)
		al_draw_filled_triangle(scrollArrowsX, layoutOptions.startY,
			scrollArrowsX - scrollArrowsW / 2, layoutOptions.startY + scrollArrowsH,
			scrollArrowsX + scrollArrowsW / 2, layoutOptions.startY + scrollArrowsH,
			textCol);

	if (showScrollDown)
		al_draw_filled_triangle(scrollArrowsX, activePage.h(),
			scrollArrowsX - scrollArrowsW / 2, activePage.h() - scrollArrowsH,
			scrollArrowsX + scrollArrowsW / 2, activePage.h() - scrollArrowsH,
			textCol);
#endif

	al_reset_clipping_rectangle();
}


//static 
bool PSectorMenu::checkDirection(int k, bool inGame)
{
	if (Keyboard::keyCurrentlyDown(k))
		return true;
	return false;
}

//static 
bool PSectorMenu::checkButton(int k, bool inGame, EscBehaviour escBehaviour)
{
	if (Keyboard::keyCurrentlyDown(k))
		return true;
	return false;
}

static std::unordered_set<int> held;

//static
bool PSectorMenu::checkButtonNoRepeat(int k, bool inGame)
{
	bool down = checkButton(k,  inGame);
	bool currentlyHeld = held.find(k) != cend(held);
	if (down && !currentlyHeld)
	{
//		Log("Press: " + to_string(k) + "\n");
		held.insert(k);
		return true;
	}
	else if (!down && currentlyHeld)
	{
//		Log("Release: " + to_string(k) + "\n");
		held.erase(k);
	}
	return false;
}
