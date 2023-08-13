#include "Universe.h"

#include "ParticleUniverseGame.h"

#include "ARGCoreSrc\TimingManager.h"
#include "ARGCoreSrc\ARGUtils.h"
#include "ARGCoreSrc\ARGMath.h"
#include "ARGCoreSrc\Keyboard.h"

#include <cmath>

#include <unordered_set>
#include <queue>
#include <vector>
#include <iterator>
#include <random>
#include <future>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <limits>
#include <utility>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

auto versionText = "v1.4";

// Default async policy is that std::async will decide whether each particle update is run on a new thread or the main thread
#define ASYNC_POLICY_DEFAULT 0

// asyncPolicy of launch::deferred means threading will not be used
// asyncPolicy of launch::async means that each particle update will happen on a separate thread
#if !ASYNC_POLICY_DEFAULT
auto asyncPolicy = std::launch::deferred;
#endif

const int DEFAULT_TRAIL_INTERVAL = 4;
const double DEFAULT_G = 6.672 * 0.00001;	// some preset universes such as spiral use different G values

const int drawTrailInterval = 1; // 4;

const int spiralNumParticlesDefault = 10;
const float spiralMassDecrease = 0.95f;

const float rightClickDeleteMaxPixelDistance = 60.f;

extern ALLEGRO_DISPLAY *g_display;
extern ALLEGRO_FONT *g_font;
extern ALLEGRO_KEYBOARD_STATE g_kbState;
extern ALLEGRO_COLOR g_colWhite;
extern int g_fontSize;

using namespace std;

#define TRAILS_ON 1


template<typename T>
string ToString(unordered_set<T> const& set)
{
	bool first = true;
	string str = "{";
	for (auto i : set)
	{
		if (!first)
		{
			str += ",";
		}
		first = false;
		str += to_string(i);
	}
	return str + "}";
}

Universe::Universe(int _maxTrailParticles):
	m_gravitationalConstant(DEFAULT_G),
	m_defaultViewportWidth(800.0f),
	m_viewportWidth(m_defaultViewportWidth * 1.f),
	m_cameraPos(400.f, 300.f),
	m_scW(al_get_display_width(g_display)),
	m_scH(al_get_display_height(g_display)),
	m_worldAspectRatio((float)m_scW / (float)m_scH),
	//m_debug(true),
	m_debugParticleInfo(false),
	m_fastForward(false),
	m_currentMenuPage(MenuPage::Default),
	m_particles(500),
#if TRAILS_ON
	m_maxTrails(_maxTrailParticles),
	m_trails(_maxTrailParticles),
#else
	m_maxTrails(0),
	m_trails(0),
#endif
	m_trailsEnabled(false),
	m_createTrailInterval(DEFAULT_TRAIL_INTERVAL),
	m_createTrailIntervalCounter(0),
	m_gravityUpdatesPerFrame(1),
	m_freeze(false),
	m_userGeneratedParticleMass(1e5f),
	m_numSpiralParticles(spiralNumParticlesDefault)
{
	CreateUniverse(5);
}

void Universe::AddParticle(VectorType _pos,	VectorType _vel, float _mass,
	ALLEGRO_COLOR _col = al_map_rgb(255, 255, 255),	bool _immovable = false)
{
	m_particles.emplace_back( _pos, _vel, _mass, _col, _immovable );
}

void Universe::AddTrailParticle(VectorType _pos, float _mass)
{
	if (m_trails.size() >= m_maxTrails)
		m_trails.pop_front();

	//m_trails.push_back( Particle(_pos, _mass, 128, 128, 128) );
	m_trails.emplace_back(_pos, VectorType(0,0), _mass, al_map_rgb(128, 128, 128));
}

void Universe::Advance(float _deltaTime)
{
	// Create trail particles
	if (m_maxTrails > 0 && (m_createTrailIntervalCounter++ % m_createTrailInterval == 0))
	{
		for (auto const& p : m_particles)
		{
			AddTrailParticle(p.GetPos(), p.GetMass());
		}
	}

	if (!m_freeze)
	{
		for (int i = 0; i < m_gravityUpdatesPerFrame; ++i)
		{
			// Update velocity of each particle
			AdvanceGravity();

			// Now apply the velocity of each particle to its position
			for (auto& p : m_particles)
			{
				if (!p.m_immovable)
				{
					p.SetPos(p.GetPos() + p.GetVel());
				}
			}
		}
	}

	// Advance input
	// mouse
	{
		static ALLEGRO_MOUSE_STATE lastMouseState;
		ALLEGRO_MOUSE_STATE mouseState;
		al_get_mouse_state(&mouseState);

		if (al_mouse_button_down(&mouseState, 1))// && !al_mouse_button_down(&lastMouseState, 1))
		{
			// Add random new particle
			float velx = 0.f; // (float)(getrandom(-100, 100)) / 100.0f;
			float vely = 0.f; // (float)(getrandom(-100, 100)) / 100.0f;

			AddParticle(
				//VectorType(getrandom(0, m_defaultViewportWidth), getrandom(0, m_defaultViewportWidth*0.75f)),
				ScreenToWorld(VectorType(mouseState.x, mouseState.y)),
				VectorType(velx, vely),
				m_userGeneratedParticleMass);
		}

#if 0
		if (al_mouse_button_down(&mouseState, 3))
		{
			auto pos = ScreenToWorld(VectorType(mouseState.x, mouseState.y));
			m_cameraFollow = FindNearest(pos);
		}
#endif

		if (al_mouse_button_down(&mouseState, 2) && !m_particles.empty())
		{
			VectorType mouseScreenPos = VectorType(mouseState.x, mouseState.y);
			VectorType mouseWorldPos = ScreenToWorld(mouseScreenPos);
			auto nearest = FindNearest(mouseWorldPos);
			if (nearest != m_particles.end())
			{
				// Only delete if it's within a certain distance from the mouse in screen space
				auto particleScreenPos = WorldToScreen(nearest->GetPos());
				float dist = (mouseScreenPos - particleScreenPos).Mag();
				if (dist < rightClickDeleteMaxPixelDistance)
					m_particles.erase(nearest);
			}
		}

		lastMouseState = mouseState;
	}
	
	al_get_keyboard_state(&g_kbState);

	// Zoom out
	if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_MINUS) || Keyboard::keyCurrentlyDown(ALLEGRO_KEY_PAD_MINUS))
	{
		float change = m_viewportWidth * 0.05f * (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_LSHIFT) ? 1.5f : 1.f);
		m_viewportWidth += change;
	}

	// Zoom in
	if ((Keyboard::keyCurrentlyDown(ALLEGRO_KEY_EQUALS) || Keyboard::keyCurrentlyDown(ALLEGRO_KEY_PAD_PLUS)) && m_viewportWidth > 1.0f)
	{
		float change = m_viewportWidth * 0.05f * (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_LSHIFT) ? 1.5f : 1.f);
		m_viewportWidth -= change;
	}

	// Camera
	{
		const float cameraSpeed = 0.75f;
		if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_UP)) { m_cameraPos.y -= m_viewportWidth * 0.75f * _deltaTime; }
		if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_DOWN)) { m_cameraPos.y += m_viewportWidth * 0.75f * _deltaTime; }
		if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_LEFT)) { m_cameraPos.x -= m_viewportWidth * 0.75f * _deltaTime; }
		if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_RIGHT)) { m_cameraPos.x += m_viewportWidth * 0.75f * _deltaTime; }
#if 0
		if (m_cameraFollow != m_particles.end())
		{
			m_cameraPos = m_cameraFollow->m_pos;
		}
#endif
	}

	if (Keyboard::keyPressed(ALLEGRO_KEY_F1)) { m_debugParticleInfo = !m_debugParticleInfo; }
	if (Keyboard::keyPressed(ALLEGRO_KEY_F2)) { m_freeze = !m_freeze; }
	if (Keyboard::keyPressed(ALLEGRO_KEY_F3)) { m_trailsEnabled = !m_trailsEnabled; }
	
	if (Keyboard::keyPressed(ALLEGRO_KEY_F4)) { m_numSpiralParticles -= 500; }
	if (Keyboard::keyPressed(ALLEGRO_KEY_F5)) { m_numSpiralParticles += 500; }

	m_fastForward = Keyboard::keyCurrentlyDown(ALLEGRO_KEY_Z);

	AdvanceMenu();
}

using ParticleList = vector<Particle*>;

int const gridRowsCols = 10;

template<typename T>
void GetGridExtents(T const& particles, double& minX, double& maxX, double& minY, double& maxY, double& gridW, double& gridH)
{
	minX = minY = numeric_limits<double>::infinity();
	maxX = maxY = -numeric_limits<double>::infinity();
	for (auto const& p : particles)
	{
		minX = min(minX, p.GetPos().x);
		minY = min(minY, p.GetPos().y);
		maxX = max(maxX, p.GetPos().x);
		maxY = max(maxY, p.GetPos().y);
	}
	gridW = maxX - minX;
	gridH = maxY - minY;
}

void Universe::AdvanceGravity()
{
	// Check every other particle and for each one, adjust my velocity
	// according to the gravitational attraction.
	// Use m_particlesCopy to get the pos of each particle because that stores the
	// state of the particle poses at the start of the universe advance.

	// Gravitational equation:
	// Force = (GMm / r^2)
	// Where G is the universal gravitational constant
	// M and m are the masses of the two objects
	// r is the distance between the two objects

	vector<unordered_set<int>> mergeSets(0);
	std::mutex mergeMutex;

	int count = m_particles.size();

	// Get grid extents
	double minX, maxX, minY, maxY, gridW, gridH;
	GetGridExtents(m_particles, minX, maxX, minY, maxY, gridW, gridH);

	auto particleGridPos = [&](Particle const& p)
	{
		return pair<int, int>{ static_cast<int>((p.m_pos.x - minX) / gridW),
							   static_cast<int>((p.m_pos.y - minY) / gridH) };
	};

	vector<vector<ParticleList>> grid;
	for (int i = 0; i < gridRowsCols; ++i)
	{
		vector<ParticleList> row(gridRowsCols);
		grid.push_back(row);
	}

	// Assign each particle to a grid rectangle
	for (auto& p : m_particles)
	{
		auto [gx, gy] = particleGridPos(p);
		if (gx < 0 || gy < 0 || gx >= grid[0].size() || gy >= grid.size())
			continue;
		grid[gy][gx].push_back(&p);
	}

	std::vector<std::mutex> mutexes(count);

	auto execute = [&](int i)
	{
		Particle& me = m_particles[i];
		float const meMass = me.m_mass;
		float size = me.GetSize();

		scoped_lock lock1(mutexes[i]);

		// todo go through each grid square
		// if it's our own grid square or within certain distance, go through particles as normal, otherwise
		// be attracted based on total mass of other grid square
		// we need to know locations of grid squares so we can determine distance between our square and other squares
		auto [myGX, myGY] = particleGridPos(me);
		
		for (auto& row : grid)
		{
			for (auto& otherGridSquareParticles : row)
			{
				if (otherGridSquareParticles.empty())
					continue;

				// get grid coordinates from the first particle
				auto [otherGX, otherGY] = particleGridPos(*otherGridSquareParticles.front());

				// If other grid square is within this distance, go through particles individually
				const int maxDist = 8;
				int gridDistance = abs(myGX - otherGX) + abs(myGY + otherGY);
				if (gridDistance <= maxDist)
				{
					for (int p = 0; p < otherGridSquareParticles.size(); p++)
					{
						Particle& other = *(otherGridSquareParticles[p]);
						if (&me == &other)
							continue;

						// Get vector between objects
						VectorType objectsVector = other.GetPos() - me.GetPos();
						float distance = objectsVector.Mag();

						// For now, don't allow collisions between particles in different grid squres
#if 0
						if (gridDistance == 0 && distance < size + other.GetSize())
						{
							scoped_lock mergeLock(mergeMutex);

							//argDebugf("Merge %d,%d", i, p);
							bool mergedIntoExistingSet = false;
							for (auto& set : mergeSets)
							{
								bool foundI = set.find(i) != set.end();
								bool foundP = set.find(p) != set.end();
								if (foundI || foundP)
								{
									//assert(mergedIntoExistingSet == false); // , "We should never find an item in more than one set");
									//argDebugf("Found in existing set: i:%d p:%d", foundI, foundP);
									if (!mergedIntoExistingSet)
									{
										set.insert(i);
										set.insert(p);
										mergedIntoExistingSet = true;
									}
								}
							}

							if (!mergedIntoExistingSet)
								mergeSets.push_back(unordered_set<int>({ i, p }));

							continue;
						}
#endif

						// Calculate gravitational attraction
						float force = (m_gravitationalConstant * meMass * other.m_mass) / (distance * distance);

						// Apply force to velocity of particle (accel = force / mass)
						objectsVector.Normalise();

						VectorType objectsVectorOther = objectsVector;

						float accelMe = force / meMass;
						float accelOther = force / other.m_mass;

#if 0
						const float maxAccel = 100.f;

						if (accelMe > maxAccel)
						{
							accelMe = maxAccel;
							if (m_debug)
							{
								me.m_col = al_map_rgb(255, 0, 0);
							}
						}
						if (accelOther > maxAccel)
						{
							accelOther = maxAccel;
							if (m_debug)
							{
								other.m_col = al_map_rgb(255, 0, 0);
							}
					}
#endif

						scoped_lock lock2(mutexes[p]);

						objectsVector.SetLength(accelMe);
						objectsVectorOther.SetLength(accelOther);

						me.AddToVel(objectsVector);
						other.AddToVel(-objectsVectorOther);
					}

				}
			}
		}

	};

	vector<std::future<void>> futures;

	for (int i = 0; i < count - 1; i++)
#if ASYNC_POLICY_DEFAULT
		futures.push_back(std::async(execute, i));
#else
		futures.push_back(std::async(asyncPolicy, execute, i));
#endif

	// Await all
	for (size_t i = 0; i < futures.size(); ++i)
		futures[i].wait();

	// Priority queue because we are deleting based on index
	// We want to delete high indicies first so as not to invalidate lower indicies
	priority_queue<int> deleteQueue;

	for (auto& set : mergeSets)
	{
		auto i = set.cbegin();
		//argDebugf("Merge set: " + ToString(set) + " into %d", *i);
		Particle& p = m_particles[*i];
		while (++i != set.cend())
		{
			//argDebugf("merging %d", *i);
			p.Merge(m_particles[*i]);
			deleteQueue.push(*i);
		}
	}
	
	// now delete old particles
	while (!deleteQueue.empty())
	{
		//argDebugf("deleting %d", deleteQueue.top());
		m_particles.erase(m_particles.begin() + deleteQueue.top());
		deleteQueue.pop();
	}
}

void Universe::Render()
{
#if TRAILS_ON
	// Render trails
	if (m_trailsEnabled)
	{
		int iTrail = 0;
		for( auto const& particle : m_trails )
		{
			if (iTrail++ % drawTrailInterval == 0)
				RenderParticle(particle, true);
		}
}
#endif 

	// Grid lines
	double minX, maxX, minY, maxY, gridW, gridH;
	GetGridExtents(m_particles, minX, maxX, minY, maxY, gridW, gridH);
	int gx = 0, gy = 0;
	for (double x = minX; gx <= gridRowsCols; ++gx, x += (gridW / (float)gridRowsCols))
	{
		auto pos1 = WorldToScreen({ x, minY });
		auto pos2 = WorldToScreen({ x, maxY });
		al_draw_line(pos1.x, pos1.y, pos2.x, pos2.y, g_colWhite, 1.f);
	}
	for (double y = minY; gy <= gridRowsCols; ++gy, y += (gridH / (float)gridRowsCols))
	{
		auto pos1 = WorldToScreen({ minX, y });
		auto pos2 = WorldToScreen({ maxX, y });
		al_draw_line(pos1.x, pos1.y, pos2.x, pos2.y, g_colWhite, 1.f);
	}


	// Render each particle
	for (auto const& p : m_particles)
		RenderParticle(p);

	// Display text stuff

	// top right - version number and nearest particle details
	al_draw_text(g_font, g_colWhite, al_get_display_width(g_display), 0, ALLEGRO_ALIGN_RIGHT, versionText);

	{
		ALLEGRO_MOUSE_STATE mouseState;
		al_get_mouse_state(&mouseState);
		VectorType mouseScreenPos = VectorType(mouseState.x, mouseState.y);
		VectorType mouseWorldPos = ScreenToWorld(mouseScreenPos);
		auto nearest = FindNearest(mouseWorldPos);
		if (nearest != m_particles.end())
		{
			// Only delete if it's within a certain distance from the mouse in screen space
			auto particleScreenPos = WorldToScreen(nearest->GetPos());
			float dist = (mouseScreenPos - particleScreenPos).Mag();
			if (dist < rightClickDeleteMaxPixelDistance)
			{
				al_draw_text(g_font, g_colWhite, al_get_display_width(g_display), 30, ALLEGRO_ALIGN_RIGHT, "Particle:");
				ostringstream ss;
				ss << "Mass " << nearest->GetMass();
				al_draw_text(g_font, g_colWhite, al_get_display_width(g_display), 55, ALLEGRO_ALIGN_RIGHT, ss.str().c_str());
				ss.str("");
				ss.clear();
				ss << "Velocity " << nearest->GetVel().x << ", " << nearest->GetVel().y;
				al_draw_text(g_font, g_colWhite, al_get_display_width(g_display), 80, ALLEGRO_ALIGN_RIGHT, ss.str().c_str());
			}
		}
	}

	// top left
	std::vector<string> entries = { stringFormat("Particles: %d", m_particles.size()),
								stringFormat("Trail particles: %d", m_trails.size()),
								stringFormat("Zoom: %.2f (-/+)", 100.f * m_viewportWidth / m_defaultViewportWidth),
								stringFormat("Camera: %.1f, %.1f", m_cameraPos.x, m_cameraPos.y),
								stringFormat("G: %e", m_gravitationalConstant),
								stringFormat("Spiral particles to generate: %d", m_numSpiralParticles)	};
	float y = 100;
	for (auto const& str : entries)
	{
		al_draw_textf(g_font, g_colWhite, 0, y, ALLEGRO_ALIGN_LEFT, str.c_str());
		y += g_fontSize;
	}

	// bottom right
	entries = { "Left mouse: Add particles",
				"Right mouse: Remove particles",
				"+/-: Zoom", "Cursor keys: Move",
				"Z: Fast forward",
				"F1: Show/hide particle info",
				"F2: Freeze",
				"F3: Enable/disable trails",
				"F4/F5: Change spiral particles to generate",
				"ESC: Quit" };
	y = al_get_display_height(g_display) - g_fontSize * entries.size();

	for (auto const& str : entries)
	{
		al_draw_textf(g_font, g_colWhite, m_scW, y, ALLEGRO_ALIGN_RIGHT, str.c_str());
		y += g_fontSize;
	}

	RenderMenu();
}

void Universe::RenderParticle(Particle const & _particle, bool _isTrail)
{
	float viewportHeight = m_viewportWidth / m_worldAspectRatio;

	float leftEdge = m_cameraPos.x - (m_viewportWidth / 2.0f);
	float topEdge = m_cameraPos.y - (viewportHeight / 2.0f);

	const float scale = 1000.f;

	float size = _particle.GetSize() / ((m_defaultViewportWidth / m_scW) * m_viewportWidth) * scale;

	VectorType pos = _particle.GetPos();

	size = max(size, 1.1f);

	enum class SizeClass
	{
		Large,
		Normal,
		Small,
		None
	};

	auto sizeClass = SizeClass::None;
	// See if particle is within viewport
	if (
		(pos.GetX() + (size / 2.0f) > (m_cameraPos.x - (m_viewportWidth / 2.0f)))	// left
		&& (pos.GetX() - (size / 2.0f) < (m_cameraPos.x + (m_viewportWidth / 2.0f)))	// right
		&& (pos.GetY() + (size / 2.0f) > (m_cameraPos.y - (viewportHeight / 2.0f)))	// top
		&& (pos.GetY() - (size / 2.0f) < (m_cameraPos.y + (viewportHeight / 2.0f)))	// bottom
		)
	{
		if (size > m_scW / 2)
		{
			sizeClass = SizeClass::Large;
		}
		else if (size > 0.5f)
		{
			sizeClass = SizeClass::Normal;
		}
		else
		{
			sizeClass = SizeClass::Small;
		}
	}

	if (sizeClass != SizeClass::None)
	{
		// Calculate screen pos of particle
		auto scPos = WorldToScreen(pos);
		float x = scPos.x, y = scPos.y;

		switch (sizeClass)
		{
			case SizeClass::Large:
			{
				al_draw_circle(x, y, 10, _particle.m_col, 1.f);
				al_draw_line((int)(x - 10), (int)y, (int)(x + 10), y, _particle.m_col, 1.f);
				al_draw_line(x, (int)(y - 7.5f), x, (int)(y + 7.5f), _particle.m_col, 1.f);
				break;
			}
			case SizeClass::Normal:
			{
				al_draw_circle(x, y, size, _particle.m_col, 1.f);
				break;
			}
			case SizeClass::Small:
			{
				al_draw_pixel((int)x, (int)y, _particle.m_col);
				break;
			}
		}

		if (m_debugParticleInfo && !_isTrail)
		{
			al_draw_textf(g_font, g_colWhite, (int)x, (int)y, 0, "m:%.3f sp:%.3f", _particle.m_mass, _particle.GetVel().Mag());
			al_draw_line(x, y, x + _particle.GetVel().x, y + _particle.GetVel().y, _particle.m_col, 1.f);
		}
	}
}

void Universe::CreateUniverse(int _id)
{
	m_gravitationalConstant = DEFAULT_G;

	m_particles.clear();
	m_trails.clear();

	m_cameraPos.x = 400.f;
	m_cameraPos.y = 300.f;
	m_viewportWidth = m_defaultViewportWidth * 1.f;

	switch (_id)
	{
		case 1:	// Original
		{
			AddParticle(VectorType(0, 0), VectorType(0.4, 0), 500000);
			AddParticle(VectorType(320, 240), VectorType(0, 0.5), 1000000);
			AddParticle(VectorType(640, 480), VectorType(0.3, 0), 40000);
			AddParticle(VectorType(100, 200), VectorType(0.4, 0.4), 50000);
			AddParticle(VectorType(150, 400), VectorType(0, 0), 60000);
			AddParticle(VectorType(600, 0), VectorType(0, -0.8), 70000);
			AddParticle(VectorType(200, 300), VectorType(-0.4, 0), 80000);
			m_createTrailInterval = 5;
			break;
		}
		case 2:	// Rectangular
		{
			float size = 1e3f;
			int const increment = 80;
			float x = 0;
			float y = 0;
			while (x < 800)
			{
				AddParticle(VectorType(x, y), VectorType(0, 0), size);
				x += increment;
			}
			x = 800;
			while (y < 600)
			{
				AddParticle(VectorType(x, y), VectorType(0, 0), size);
				y += increment * 0.75f;
			}
			y = 600;
			while (x > 0)
			{
				AddParticle(VectorType(x, y), VectorType(0, 0), size);
				x -= increment;
			}
			x = 0;
			while (y > 0)
			{
				AddParticle(VectorType(x, y), VectorType(0, 0), size);
				y -= increment * 0.75f;
			}

			//		AddParticle(VectorType(400,300),	VectorType(0,0),	size0);

			m_createTrailInterval = DEFAULT_TRAIL_INTERVAL;

			break;
		}
#if 0
		case 3:	// Circle
		{
			// Disabled as there is a crash bug with merging when running multiple threads
			float size = 1e2f;
			double const pi = 3.1415926535897932384626433832795;
			double const radius = 300;
			int desiredParticles = 64;
			double increment = (pi*2.0) / (double)desiredParticles;
			int p = 0;
			for (double count = 0; p < desiredParticles; count += increment, p++)
			{
				double x = (sin(count) * radius) + (m_defaultViewportWidth / 2.0);
				double y = (cos(count) * radius) + ((m_defaultViewportWidth / 2.0) * 0.75);

				// No movement
				AddParticle(VectorType(x, y), VectorType(0, 0), size);

				// Outward movement
				//				AddParticle(VectorType(x,y),	VectorType(sin(count),cos(count)),	size);

				// Weird
				//				AddParticle(VectorType(x,y),	VectorType(tan(count) / 8.0f,tan(count) / 8.0f),	size);
			}

			m_createTrailInterval = DEFAULT_TRAIL_INTERVAL;

			break;
		}
#endif
		case 3: // Grid
		{
			for (int x = 0; x <= 800; x += 80)
				for (int y = 0; y <= 600; y += 60)
					AddParticle(VectorType(x, y), VectorType(0, 0), 1e4);

			m_createTrailInterval = DEFAULT_TRAIL_INTERVAL;

			break;
		}
		case 4: // Solar system
		{
			// Mass is defined in earth masses
			// Distances in AU
			// Data from http://www.jpl.nasa.gov/solar_system/sun/sun_index.html
			// This doesn't work very well as Jupiter and inner planets are too close to the sun and collide with it

			//m_gravitationalConstant /= 100;
			//m_gravitationalConstant *= 1e6;

			//m_gravityAdvancesPerFrame = 100;

			float sunX = 400.f;
			float sunY = 300.f;

			float velX = 0.0f;
			float velY = 0.01f;

			Particle* p;

			float const massMult = 10;
			float const sunMass = 332830 * massMult;

			float const distMult = 10.f;

			auto addPlanet = [&](float _distAU, float _earthMasses, ALLEGRO_COLOR _col = al_map_rgb(255,255,255), bool _immovable = false)
			{
				_distAU *= distMult;
				_earthMasses *= massMult;
				float speed = sqrtf((m_gravitationalConstant * sunMass) / _distAU) * 1.f;
				AddParticle(VectorType(sunX + _distAU, sunY), VectorType(0.f, speed), _earthMasses, _col, _immovable);
				return p;
			};

			// Sun
			AddParticle(VectorType(sunX, sunY), VectorType(0, 0), sunMass, al_map_rgb(255, 255, 0), false);

			// Mercury
			addPlanet(0.387f, 0.054f);

			// Venus
			addPlanet(0.723f, 0.814f);

			// Earth
			addPlanet(1.f, 1.f);

			// Mars
			addPlanet(1.52f, 0.17f, al_map_rgb(255,64,64));

			// Jupiter
			float jRadius = 5.2f;
			p = addPlanet(jRadius, 317.8f, al_map_rgb(255, 128, 64));
			//p->SetVel(VectorType(0.f, 0.182));
#if 1
			// Saturn
			addPlanet(9.54f, 95.2f, al_map_rgb(192, 192, 0));
			//p->SetVel(VectorType(0.f, 0.15));

			// Uranus
			addPlanet(19.19f, 14.48f, al_map_rgb(64, 255, 64));
			//p->SetVel(VectorType(0.f, 0.1));

			// Neptune
			addPlanet(30.07f, 17.2f, al_map_rgb(128, 128, 255));
			//p->SetVel(VectorType(0.f, 0.08));

			// Pluto
			addPlanet(39.48f, 0.0025f);
			//p->SetVel(VectorType(0.f, 0.06));
#endif

#if 0
			for (int i = 0; i < 10; ++i)
			{
				addPlanet((float)getrandom(1, 100) / 10.f, ((float)getrandom(1, 200) / 10.f));
			}
#endif

			m_createTrailInterval = DEFAULT_TRAIL_INTERVAL;
			//m_createTrailInterval = 5;

			break;
		}
		case 5:
		case 6:
		case 7:
		{
			// spirals with lower gravity
			m_gravitationalConstant /= 10000;
			m_viewportWidth = m_defaultViewportWidth * 10.f;

			if (_id == 5)
				MakeSpiralUniverse(1e12f, m_numSpiralParticles, 10.f, 20.5f, 0.4f, spiralMassDecrease, 5.f);
			else if (_id == 6)
				MakeSpiralUniverse(1e9f, m_numSpiralParticles, 5.f, 8.f, 0.25f, 0.999f, 0.6f);
			else if (_id == 7)
				MakeSpiralUniverse(1e9f, m_numSpiralParticles, 5.f, 14.f, 1.25f, 0.999f, 1.5f);
			
			break;
		}
		case 8:
		{
			// spiral with normal gravity
			m_viewportWidth = m_defaultViewportWidth * 10.f;

			MakeSpiralUniverse(1e6f, 4000, 8.f, 13.f, 1.5f, 0.9999f, 2.5f);

			break;
		}
		case 9:
		{
			// two suns
			AddParticle(VectorType(300, 300), VectorType(0, -0.2), 100000);
			AddParticle(VectorType(400, 300), VectorType(0, 0.2), 100000);
		}
	}

#if 0
	m_cameraFollow = m_particles.end();
#endif
}

void Universe::MakeSpiralUniverse(float startMass, int numParticles, float r, float rStep, float step, float massDecrease, float velMultiplier)
{
	//std::mt19937 mersenneEngine;
	//std::uniform_real_distribution<double> distribution1(0, ARGMath::PI * 2);
	//std::uniform_real_distribution<double> distribution2(0.1, 1.0);

	float mass = startMass;
	//float mass = 1e8f;
	double sinCount = 0.f;
	for (int i = 0; i < numParticles; ++i)
	{
		VectorType circlePos = VectorType(sin(sinCount), cos(sinCount));
		VectorType pos = (circlePos * r) + m_cameraPos;
		//VectorType vel = VectorType(cos(sinCount), sin(sinCount));
		//VectorType vel = VectorType(sin(sinCount + ARGMath::PI), cos(sinCount + ARGMath::PI));
		VectorType vel = circlePos.Rotated(ARGMath::PI * 0.75f) * velMultiplier;
		//VectorType vel = circlePos.Rotated(distribution1(mersenneEngine)) * distribution2(mersenneEngine);
		AddParticle(pos, vel, mass);
		sinCount += step;
		r += rStep;
		if (mass > 1.f)
			mass *= massDecrease;
	}
}

void Universe::Save()
{
	ofstream file("save.txt", std::ios::trunc);
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
	ostringstream ss;
	ss << m_particles.size() << endl;
	file.write(ss.str().c_str(), ss.str().size());
	for (auto const& p : m_particles)
	{
		ss.str("");
		ss.clear();
		ss << p.m_pos.x << " " << p.m_pos.y << " " << p.m_mass << " " << p.m_vel.x << " " << p.m_vel.y << " " << p.m_col.r << " " << p.m_col.g << " " << p.m_col.b << endl;
		file.write(ss.str().c_str(), ss.str().size());
	}
}

void Universe::Load()
{
	m_particles.clear();
	m_trails.clear();
	ifstream file("save.txt");
	//file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
	int numParticles = -1;
	string line;
	while (getline(file, line))
	{
		istringstream iss(line);
		if (numParticles == -1)
			iss >> numParticles;
		else
		{
			VectorType pos, vel;
			ALLEGRO_COLOR col;
			decltype(Particle::m_mass) mass;
			iss >> pos.x >> pos.y >> mass >> vel.x >> vel.y >> col.r >> col.g >> col.b;
			m_particles.emplace_back(pos, vel, mass, col);
		}
	}
}

void Universe::AdvanceMenu()
{
	al_get_keyboard_state(&g_kbState);
	switch (m_currentMenuPage)
	{
		case MenuPage::Default:
		{
			if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_C))
			{
				m_currentMenuPage = MenuPage::CreateUniverse;
			}
			else if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_J))
			{
				Save();
			}
			else if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_L))
			{
				Load();
			}
			break;
		}
		case MenuPage::CreateUniverse:
		{
			if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_0))			// Previous
			{
				m_currentMenuPage = MenuPage::Default;
			}
			for (int key = 1; key <= 9; ++key)
			{
				if (Keyboard::keyCurrentlyDown(ALLEGRO_KEY_0 + key))
				{
					CreateUniverse(key);
					m_currentMenuPage = MenuPage::Default;
				}
			}
			break;
		}
	}
}

void Universe::RenderMenu()
{
	vector<string> entries;
	float y = al_get_display_height(g_display);
	switch (m_currentMenuPage)
	{
		case MenuPage::Default:
		{
			entries = { "C: Create Universe", "J: Save", "L: Load"};
			break;
		}
		case MenuPage::CreateUniverse:
		{
			entries = { "0: Previous menu",
						"1: Original",
						"2: Rectangle",
						//"3: Circle",
						"3: Grid",
						"4: Solar system",
						"5: Spiral 1",
						"6: Spiral 2",
						"7: Spiral 3",
						"8: Spiral 4",
						"9: Two suns"};
			break;
		}
	}
	y -= g_fontSize * entries.size();
	for(auto str : entries)
	{
		al_draw_textf(g_font, g_colWhite, 0, y, 0, str.c_str());
		y += g_fontSize;
	}
}

VectorType Universe::WorldToScreen(const VectorType& _world)
{
	float viewportHeight = m_viewportWidth / m_worldAspectRatio;
	float leftEdge = m_cameraPos.x - (m_viewportWidth / 2.0f);
	float topEdge = m_cameraPos.y - (viewportHeight / 2.0f);
	return VectorType((_world.x - leftEdge) * (m_scW / m_viewportWidth), (_world.y - topEdge) * (m_scH / viewportHeight));
}

VectorType Universe::ScreenToWorld(const VectorType& _screen)
{
	float viewportHeight = m_viewportWidth / m_worldAspectRatio;
	float leftEdge = m_cameraPos.x - (m_viewportWidth / 2.0f);
	float topEdge = m_cameraPos.y - (viewportHeight / 2.0f);
	float const rx = m_scW / m_viewportWidth;
	float const ry = m_scH / viewportHeight;
	return VectorType((_screen.x + (leftEdge * rx)) / rx, (_screen.y + (topEdge * ry)) / ry);
}

decltype(Universe::m_particles)::iterator Universe::FindNearest(VectorType const& _pos)
{
	float nearestDistSq = FLT_MAX;
	auto bestP = m_particles.end();
	auto i = m_particles.begin();
	for (; i != m_particles.end(); ++i)
	{
		float distSq = (_pos - i->GetPos()).MagSq();
		if (distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			bestP = i;
		}
	}
	return bestP;
}
