#pragma once

#include <optional>
#include <sstream>
#include <unordered_set>

#include "dialog.h"
#include "Config.h"

using std::string;
using std::vector;
using std::function;
using std::make_shared;
using std::to_string;
using std::unordered_set;

struct ALLEGRO_FONT;
struct ALLEGRO_BITMAP;
class BitmapWrapper;

#undef RGB
struct RGB;

struct PSectorMenuLayoutOptions
{
	ALLEGRO_FONT* font = nullptr;
	int startY = 200; // 100;
	int yInc = 20; // * scale;
	int buttonHeight = 10;
	int scale = 1;
	std::optional<int> leftRightKeyDelayOverride;
	bool inGame = false;
};

class PSectorMenu
{
	PSectorMenuLayoutOptions layoutOptions;

	int scale = 1;

	int yInc = 20;
	int buttonHeight = 10;

	int startY = 200;
	int nextY = startY;

	int key_delay = 10;

	int scrollY = 0;

	CmdMenu cmdMenu;

	ALLEGRO_FONT* font;

	std::function<void()> onExit;

	bool showScrollUp = false, showScrollDown = false;

public:

	PSectorMenu(PSectorMenuLayoutOptions const& options, std::function<void()> onExit = []() {});

	~PSectorMenu();

	static std::unordered_set<int> held;

	enum class EscBehaviour
	{
		Back,
		Pause
	};

	static bool checkDirection(int k, bool inGame = false);
	static bool checkButton(int k, bool inGame = false, EscBehaviour escBehaviour = EscBehaviour::Back);
	static bool checkButtonNoRepeat(int k, bool inGame = false);

	void update(int maxY);

	void addInt(int x, string const& txt, int* value, int minValue, int maxValue, int step = 1);
	void addEnum(int x, string const& txt, int* value, int minValue, int maxValue, std::shared_ptr<vector<string>> toStrLookup);
	void addBool(int x, string const& txt, bool* value, std::shared_ptr<vector<string>> toStrLookup);
	std::shared_ptr<CmdButtonAction> addAction(int x, DialogTextSource txt, std::function<void()> action, std::function<void(CmdButtonAction&)> deleteAction = nullptr, RGB* col = nullptr);
	std::shared_ptr<CmdButtonAction> addAction(int x, DialogTextSource txt, std::function<void(CmdButtonAction&)> action, std::function<void(CmdButtonAction&)> deleteAction = nullptr, RGB* col = nullptr);
	
	template<typename T>
	void add(int x, ConfigOptionWrapper<T>& option, T min, T max, T step = 1)
	{
		DialogTextFunc nameText = [&] { return option.getDisplayName(); };
		DialogTextFunc valueText = [&]
			{
				std::ostringstream ss;
				ss << option.get();
				return ss.str();
			};
		auto leftAction = [&, min, max, step] { option = option.get() - step; if (option < min) option = min; };
		auto rightAction = [&, min, max, step] { option = option.get() + step; if (option > max) option = max; };
		cmdMenu.add(make_shared<CmdButtonDynamic>(x, nextY, 344, nextY + buttonHeight, nameText, valueText, [] {}, leftAction, rightAction));
		nextY += yInc;
	}


	void addHeading(int x, string const& text);
	void addGap();

	//void render(BitmapWrapper const& activePage);
	void render();
};
