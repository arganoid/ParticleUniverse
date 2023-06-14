#include "Keyboard.h"

#include <allegro5/allegro.h>

#include <unordered_map>

bool Keyboard::keyCurrentlyDown(int k)
{
	ALLEGRO_KEYBOARD_STATE s;
	al_get_keyboard_state(&s);	// performance?
	return al_key_down(&s, k);
}

bool Keyboard::keyPressed(int _key)
{
	static std::unordered_map<int, bool> keyMap;

	bool down = Keyboard::keyCurrentlyDown(_key);
	if (!down && keyMap[_key] == true)
	{
		keyMap[_key] = false;
		return true;
	}

	keyMap[_key] = down;
	return false;
}
