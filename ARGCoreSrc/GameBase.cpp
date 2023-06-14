// todo merge with AL5Test equivalent

#define CIRCULAR_BUFFER 1

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_native_dialog.h>

#include "GameBase.h"
#include "TimingManager.h"
#include "ARGUtils.h"

#include <map>
#include <string>
#include <sstream>

#include <windows.h>

// http://stackoverflow.com/questions/34211610/are-stdstack-and-stdqueue-cache-friendly
// "[std::deque] is not all that cache friendly (in every implementation I have examined)"
// see also Profiling results.txt
#if CIRCULAR_BUFFER
#include <boost/circular_buffer.hpp>
using TimingBuffer = boost::circular_buffer<float>;
#else
#include <deque>
using TimingBuffer = std::deque<float>;
#endif

#define LOG_TIMING 0

#define DEFAULT_RES_X 1920
#define DEFAULT_RES_Y 1080

unsigned int g_advanceCounter;
unsigned int g_frameCounter;

static const int bufLength = 1024;
static char buf[bufLength];

ALLEGRO_DISPLAY *g_display = NULL;
ALLEGRO_FONT *g_font;
ALLEGRO_KEYBOARD_STATE g_kbState;
ALLEGRO_COLOR g_colWhite;

int g_fontSize = 20;


GameBase::GameBase(bool _requireMouse, int _advanceLimit):
	m_advanceLimit(_advanceLimit)
{
	int area_width = DEFAULT_RES_X;
	int area_height = DEFAULT_RES_Y;

	if (!al_init()) {
		sprintf_s(buf, "Failed to initialise Allegro 5\n");
		fprintf_s(stderr, buf);
		al_show_native_message_box(NULL, NULL, "Error", buf, NULL, ALLEGRO_MESSAGEBOX_ERROR);
	}

	al_init_primitives_addon();
	al_install_keyboard();

	if (_requireMouse)
	{
		al_install_mouse();
	}

#if 0
	set_config_file("config.cfg");
	area_width = get_config_int("video", "Xresolution", DEFAULT_RES_X);
	area_height = get_config_int("video", "Yresolution", DEFAULT_RES_Y);

	// windowed mode can cause performance issues
	bool fullScreen = get_config_int("video", "fullscreen", false) == 1;
	int video_mode;
	if (!fullScreen)
	{
		video_mode = GFX_AUTODETECT_WINDOWED;
	}
	else
	{
		video_mode = GFX_DIRECTX_ACCEL; // GFX_AUTODETECT;

		bool autoMaxRes = get_config_int("video", "fullscreenAutoMaxResolution", 0) == 1;
		if (autoMaxRes)
		{
			auto modes = get_gfx_mode_list(GFX_DIRECTX_ACCEL);
			std::stringstream ss;
			for (int i = 0; i < modes->num_modes; ++i)
			{
				auto m = modes->mode[i];
				ss << m.width << " " << m.height << " " << m.bpp << std::endl;
			}
			argDebug(ss.str());
			area_width = modes->mode[modes->num_modes - 1].width;
			area_height = modes->mode[modes->num_modes - 1].height;
			destroy_gfx_mode_list(modes);
		}
	}
#endif

#if 0
	al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
#else
	al_set_new_display_flags(ALLEGRO_WINDOWED);
	al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_REQUIRE);	// 1 = on, 2 = off, 0 = whatever. docs: can be ignored by driver despite require
	//al_set_new_display_option(ALLEGRO_SAMPLES, 1, ALLEGRO_REQUIRE);	// vsync doesn't work when this is on!

	ALLEGRO_MONITOR_INFO aminfo;
	al_get_monitor_info(0, &aminfo);
	int desktop_width = aminfo.x2 - aminfo.x1 + 1;
	int desktop_height = aminfo.y2 - aminfo.y1 + 1;
	area_width = desktop_width - 300;
	area_height = desktop_height - 300;
#endif

	// Create display
	g_display = al_create_display(area_width, area_height);
	if (!g_display)
	{
		sprintf_s(buf, "Couldn't set video mode %dx%d\n", area_width, area_height);
		fprintf_s(stderr, buf);
		al_show_native_message_box(NULL, NULL, "Error", buf, NULL, ALLEGRO_MESSAGEBOX_ERROR);
		exit(1);
	}

	// Load font
	al_init_font_addon();
	al_init_ttf_addon();
	auto fontName = "LektonCode/LektonCode-Regular.ttf";
	g_font = al_load_ttf_font(fontName, g_fontSize, 0);
	if (!g_font)
	{
		sprintf_s(buf, "Could not load '%s'.\n", fontName);
		fprintf_s(stderr, buf);
		al_show_native_message_box(g_display, "Error", "Error", buf, NULL, ALLEGRO_MESSAGEBOX_ERROR);
		exit(1);
	}

	// Event queue
	m_eventQueue = al_create_event_queue();
	if (!m_eventQueue) {
		al_show_native_message_box(g_display, "Error", "Error", "Failed to create event queue", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
	}
	al_register_event_source(m_eventQueue, al_get_display_event_source(g_display));

	g_colWhite = al_map_rgb(255, 255, 255);

	m_showFPS = true; // get_config_int("video", "showFPS", false) == 1;

	m_timingManager = new TimingManager();
	//m_timingManager->RunTest();
}

void GameBase::MainLoop()
{
	int const averageOverThisManyFrames = 120;

	TimingBuffer advanceTimes(averageOverThisManyFrames);	// Store advance times for last 2 seconds
	TimingBuffer renderTimes(averageOverThisManyFrames);
	TimingBuffer fullFrameTimes(averageOverThisManyFrames);	// Total time between frames, including screen_swap

	double elapsedTime = 0.;
	double totalAdvanceTime = 0.;

	do
	{
		ALLEGRO_EVENT event;
		if (al_get_next_event(m_eventQueue, &event))
		{
			if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
				break;
		}

		m_timingManager->BeginProfileSection("fullFrame");
		m_timingManager->BeginProfileSection("advance");

		float deltaTime = m_timingManager->GetDeltaTime();
		Advance(deltaTime);

		elapsedTime += deltaTime;
		g_advanceCounter++;

		double const advanceTime = m_timingManager->EndProfileSection("advance");
		float advanceMs = advanceTime * 1000.0;
		if (advanceTimes.size() == averageOverThisManyFrames)
		{
			advanceTimes.pop_front();
		}
		advanceTimes.push_back(advanceMs);

		totalAdvanceTime += advanceTime;

#if LOG_TIMING
		sprintf_s(buf, "advance: %g ms\t", advanceMs);		argDebug(buf);
#endif

		// Graphics
		if (true)
		{
			m_timingManager->BeginProfileSection("render");

			Render();

			// Note - render timing doesn't take vsync or screen swap into account
			double renderMs = m_timingManager->EndProfileSection("render") * 1000.0;
			//				sprintf(buf, "Render: %g ms\n",renderMs);		argDebug(buf);

			if (renderTimes.size() == averageOverThisManyFrames)
			{
				renderTimes.pop_front();
			}
			renderTimes.push_back(renderMs);

#if LOG_TIMING
			sprintf_s(buf, "render: %g ms\t", renderMs);		argDebug(buf);
#endif

			float averageA = 0.f;
			float maxA = 0.f;
			{
				for (auto const n : advanceTimes)
				{
					averageA += n;
					maxA = std::max(n, maxA);
				}
			}
			averageA /= (float)advanceTimes.size();

			float averageR = 0.f;
			float maxRender = 0.f;
			{
				for (auto const n : renderTimes)
				{
					averageR += n;
					maxRender = std::max(n, maxRender);
				}
			}
			averageR /= (float)renderTimes.size();

			// full frame display will be 1 frame behind
			float averageFull = 0.f;
			float minFull = 9999.f;
			float maxFull = 0.f;
			{
				for (auto const n : fullFrameTimes)
				{
					averageFull += n;
					minFull = std::min(n, minFull);
					maxFull = std::max(n, maxFull);
				}
			}
			averageFull /= (float)fullFrameTimes.size();

			if (m_showFPS)
			{
#if 0
				int y = 0;
								 al_draw_textf(g_font, g_colWhite, 0, y, 0, "Update: %5.2f ms (avg: %5.2f, max: %5.2f, allAvg: %5.2f)  ", advanceMs, averageA, maxA, (totalAdvanceTime / (double)g_advanceCounter* 1000));
				y += g_fontSize; al_draw_textf(g_font, g_colWhite, 0, y, 0, "Render:  %5.2f ms (avg: %5.2f, max: %5.2f)  ", renderMs, averageR, maxRender);
				y += g_fontSize; al_draw_textf(g_font, g_colWhite, 0, y, 0, "Full frame: (avg: %5.2f, min: %5.2f, max: %5.2f)  ", averageFull, minFull, maxFull);
				y += g_fontSize; al_draw_textf(g_font, g_colWhite, 0, y, 0, "Frames: %d   Seconds: %.0f  ", g_frameCounter, elapsedTime);
#else
				// Simplified timing display
				int y = 0;
				al_draw_textf(g_font, g_colWhite, 0, y, 0, "Update: %5.2fms", averageA);
				y += g_fontSize; al_draw_textf(g_font, g_colWhite, 0, y, 0, "Render: %5.2fms", averageR);
				y += g_fontSize; al_draw_textf(g_font, g_colWhite, 0, y, 0, "Frames: %d   Seconds: %.0f", g_frameCounter, elapsedTime);
#endif
			}
			ScreenSwap();

			double const fullFrameMs = m_timingManager->EndProfileSection("fullFrame") * 1000.0;
#if LOG_TIMING
			sprintf_s(buf, "fullFrame: %g ms\n", fullFrameMs);		argDebug(buf);
#endif

			if (fullFrameTimes.size() == averageOverThisManyFrames)
			{
				fullFrameTimes.pop_front();
			}
			fullFrameTimes.push_back(fullFrameMs);


			g_frameCounter++;
		}
		else
		{
			Sleep(0);
		}
		al_get_keyboard_state(&g_kbState);
	}
	while (!al_key_down(&g_kbState, ALLEGRO_KEY_ESCAPE) && (m_advanceLimit <= 0 || g_advanceCounter < (unsigned)m_advanceLimit));

	argDebugf("totalAdvanceTime: %.2f\n", totalAdvanceTime);

	delete m_timingManager;
	m_timingManager = nullptr;
}

void GameBase::ScreenSwap(void)
{
	al_flip_display();
}
