// todo
// combine recording file and save file?
// options for particle colours

#include "Universe.h"

#include "ParticleUniverseGame.h"

#include "ARGCore\TimingManager.h"
#include "ARGCore\ARGUtils.h"
#include "ARGCore\ARGMath.h"
#include "ARGCore\Keyboard.h"
#include "ARGCore\Fonts.h"

#include <cmath>

#include <algorithm>
#include <unordered_set>
#include <queue>
#include <vector>
#include <iterator>
#include <random>
#include <future>
#include <memory>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <utility>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

using namespace std;

auto versionText = "v1.6";

// Comment out to go back to old version of grid based gravity update
// New system is slightly faster than old system in some cases with large numbers of particles,
// slower in others. Recommend 35 rows/cols with the new system when using a very large number of
// particles (e.g. 10k or more)
#define GRID_BASED_MODE_NEW

// Default async policy is that std::async will decide whether each particle update is run on a new thread or the main thread
#define ASYNC_POLICY_DEFAULT 1

// asyncPolicy of launch::deferred means threading will not be used
// asyncPolicy of launch::async means that each particle update will happen on a separate thread
#if !ASYNC_POLICY_DEFAULT
auto asyncPolicy = std::launch::deferred;
#endif

// 35 may work better for the latest version of the grid-based system
const int defaultGridRowsCols = 20;

// High = faster but less accurate, if it's equal or close to gridRowsCols there's no benefit in the grid based 
// approach (in fact it will be worse than normal mode)
const int defaultHighAccuracyGridDistance = 2;

const int defaultTrailInterval = 4;
const double DEFAULT_G = 6.672 * 0.00001;	// some preset universes such as spiral use different G values

const int drawTrailInterval = 1; // 4;

const int spiralNumParticlesDefault = 1500;
const float spiralMassDecrease = 0.999f;		// used 0.99 in latest video, old version used 0.95

const float particleEdgeThickness = 1.5f;

// Recording: set num particles much higher (e.g. 15k like in my latest video), set recording mode
// to save, and update the filenames below. Probably also enable save on quit, and then enable
// load on start to resume the recording.

enum class RecordingMode
{
	None,
	Save,
	Load
};

const bool loadOnStart = false;
const bool saveOnQuit = false;
const RecordingMode recordingMode = RecordingMode::None;

// output/input filenames below are ones I used for recording of recent video,
// obviously change this if you plan to use this yourself

const string recordingOutputFileName = "F://ParticleUniverseRecording1.bin";

vector<string> recordingInputFileNames =
	{
		"F://ParticleUniverseRecording1.bin",
		"F://ParticleUniverseRecording2.bin",
	};

size_t currentRecordingFileI = 0;

const float rightClickDeleteMaxPixelDistance = 60.f;

extern ALLEGRO_DISPLAY *g_display;
extern ALLEGRO_FONT *g_font;
extern ALLEGRO_COLOR g_colWhite;
extern int g_fontSize;

ifstream inputFile;
ofstream outputFile;

std::ostream& operator<<(std::ostream& os, ALLEGRO_COLOR const& col)
{
	os << col.r << " " << col.g << " " << col.b;
	return os;
}

std::ostream& operator<<(std::ostream& os, Particle const& p)
{
	os << p.m_pos.x << " " << p.m_pos.y << " "
	   << p.m_vel.x << " " << p.m_vel.y << " "
	   << p.m_mass << " " << p.m_col << " ";
	return os;
}

inline std::istream& operator>> (std::istream& is, Particle& p)
{
	is >> p.m_pos.x >> p.m_pos.y >> p.m_vel.x >> p.m_vel.y;
	is >> p.m_mass;
	is >> p.m_col.r >> p.m_col.g >> p.m_col.b;
	return is;
}

const bool autoSaveConfigOptions = false;

Universe::Universe():
	m_gravitationalConstant(DEFAULT_G),
	m_defaultViewportWidth(800.0f),
	m_viewportWidth(m_defaultViewportWidth * 1.f),
	m_cameraPos(400.f, 300.f),
	m_scW(al_get_display_width(g_display)),
	m_scH(al_get_display_height(g_display)),
	m_worldAspectRatio((float)m_scW / (float)m_scH),
	//m_debug(true),
	m_debugParticleInfo(false),
	m_currentMenuPage(MenuPage::Default),
	m_particles(500),
	m_showTrails(false),
	m_createTrailInterval("trails", "createTrailInterval", "Create trail interval", defaultTrailInterval, autoSaveConfigOptions),
	m_maxTrails("trails", "maxTrails", "Max trails", 100000, autoSaveConfigOptions),
	m_sizeLogBase("particles", "sizeLogBase", "Size log base", 2.7, autoSaveConfigOptions),
	m_gridRowsCols("grid", "gridRowsCols", "Grid rows and columns", defaultGridRowsCols, autoSaveConfigOptions),
	m_numSpiralParticles("spiral", "numSpiralParticles", "Spiral particles to generate", spiralNumParticlesDefault, autoSaveConfigOptions),
	m_highAccuracyGridDistance("grid", "highAccuracyGridDistance", "High accuracy grid distance", defaultHighAccuracyGridDistance, autoSaveConfigOptions),
	m_createTrailIntervalCounter(0),
	m_freeze(false),
	m_userGeneratedParticleMass(1e5f),
	m_showConfigMenu(false),
	m_useGridBasedMode(false)
{
	m_allOptions = {
		&m_gridRowsCols,
		&m_highAccuracyGridDistance,
		&m_numSpiralParticles,
		&m_createTrailInterval,
		&m_maxTrails,
		&m_sizeLogBase
	};


	Config::configFilename = "ParticleUniverse.cfg";
	Config::config = al_load_config_file(Config::configFilename.c_str());
	if (!Config::config)
	{
		// create a config file with all config option wrapper default values (todo put these in a list or something)
		Config::config = al_create_config();
		for (auto& option : m_allOptions)
			option->save();
	}

	Fonts::load();

	m_configMenu = CreateConfigMenu();

	CreateUniverse(8);

	switch (recordingMode)
	{
		case RecordingMode::Load:
		{
			// now done in Advance
			break;
		}
		case RecordingMode::Save:
		{
			outputFile.open(recordingOutputFileName, ios::binary);
			break;
		}
	}

	if (loadOnStart)
	{
		Load();
	}
}

void Universe::AddParticle(VectorType _pos,	VectorType _vel, float _mass,
	ALLEGRO_COLOR _col = al_map_rgb(255, 255, 255))
{
	m_particles.emplace_back( _pos, _vel, _mass, _col );
}

void Universe::AddTrailParticle(VectorType _pos, float _mass)
{
	if (m_trails.size() >= (size_t)m_maxTrails)
		m_trails.pop_front();

	//m_trails.push_back( Particle(_pos, _mass, 128, 128, 128) );
	m_trails.emplace_back(_pos, VectorType(0,0), _mass, al_map_rgb(128, 128, 128));
}

void Universe::Advance(float _deltaTime)
{
/*
	// When I wrote this I had forgotten that the gravity simulation isn't linked to the frame rate
	if (recordingMode == RecordingMode::Save)
		_deltaTime = 1 / 60.f;
*/

	// Create trail particles
	if (m_maxTrails > 0 && m_createTrailIntervalCounter++ % m_createTrailInterval == 0)
	{
		for (auto const& p : m_particles)
		{
			AddTrailParticle(p.GetPos(), p.GetMass());
		}
	}

	if (recordingMode == RecordingMode::Load)
	{
		if (!inputFile.is_open())
		{
			auto filename = recordingInputFileNames[currentRecordingFileI];
			inputFile.open(filename, ios::binary);
			argDebugf("opened %s\n", filename.c_str());
		}
		else if (inputFile.eof())
		{
			// open next file in sequence
			if (currentRecordingFileI < recordingInputFileNames.size() - 1)
			{
				++currentRecordingFileI;
				inputFile.close();
				auto filename = recordingInputFileNames[currentRecordingFileI];
				inputFile.open(filename, ios::binary);
				argDebugf("opened %s\n", filename.c_str());
			}
		}

		if (inputFile.good())
		{
			size_t pCount;
			read(inputFile, pCount);
			if (inputFile.good())
			{
				m_particles.resize(pCount);
				for (size_t i = 0; i < pCount; ++i)
				{
					// todo could save pos as floats rather than doubles
					read(inputFile, m_particles[i].m_mass);
					read(inputFile, m_particles[i].m_pos.x);
					read(inputFile, m_particles[i].m_pos.y);
					read(inputFile, m_particles[i].m_vel.x);
					read(inputFile, m_particles[i].m_vel.y);
					read(inputFile, m_particles[i].m_col.r);
					read(inputFile, m_particles[i].m_col.g);
					read(inputFile, m_particles[i].m_col.b);
				}
			}
		}
	}
	else if (!m_freeze)
	{
		// todo make fast forward cut off if time taken is too long
		int numGravityUpdates = Keyboard::keyCurrentlyDown(ALLEGRO_KEY_Z) ? 100 : 1;

		for (int i = 0; i < numGravityUpdates; ++i)
		{
			// Update velocity of each particle
			if (!m_useGridBasedMode)
				AdvanceGravityNormalMode();
			else
				AdvanceGravityGridBasedMode();

			// Now apply the velocity of each particle to its position
			for (auto& p : m_particles)
				p.SetPos(p.GetPos() + p.GetVel());
		}
	}

	// Advance input
	// Disable most controls when config menu is open
	if (!m_showConfigMenu)
	{
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
	}

	if (Keyboard::keyPressed(ALLEGRO_KEY_TAB)) { m_showConfigMenu = !m_showConfigMenu; }

	if (Keyboard::keyPressed(ALLEGRO_KEY_F1)) { m_debugParticleInfo = !m_debugParticleInfo; }
	if (Keyboard::keyPressed(ALLEGRO_KEY_F2)) { m_freeze = !m_freeze; }
	if (Keyboard::keyPressed(ALLEGRO_KEY_F3)) { m_showTrails = !m_showTrails; }
	
	if (Keyboard::keyPressed(ALLEGRO_KEY_G)) { m_useGridBasedMode = !m_useGridBasedMode; }

	AdvanceMenu();

	if (recordingMode == RecordingMode::Save)
	{
		write(outputFile, m_particles.size());
		for (auto const& p : m_particles)
		{
			write(outputFile, p.m_mass);
			write(outputFile, p.m_pos.x);
			write(outputFile, p.m_pos.y);
			write(outputFile, p.m_vel.x);
			write(outputFile, p.m_vel.y);
			write(outputFile, p.m_col.r);
			write(outputFile, p.m_col.g);
			write(outputFile, p.m_col.b);
		}
	}
}

void Universe::AdvanceGravityNormalMode()
{
	// Check every other particle and for each one, adjust my velocity
	// according to the gravitational attraction.

	// Gravitational equation:
	// Force = (GMm / r^2)
	// Where G is the universal gravitational constant
	// M and m are the masses of the two objects
	// r is the distance between the two objects

	vector<unordered_set<int>> mergeSets;
	mutex mergeMutex;

	size_t count = m_particles.size();

	vector<mutex> mutexes(count);

	const float sizeLogBase = m_sizeLogBase;

	// Cache sizes to avoid having to call GetSize (with slow logarithm calls) multiple times per particle
	// 10k particles, with only a single log, update = 170ms
	// with two logs, update = 203ms
	// with caching sizes, update = 154ms
	vector<float> sizes(count);
	for (size_t i = 0; i < m_particles.size(); ++i)
		sizes[i] = m_particles[i].GetSize(sizeLogBase);

	auto execute = [&](int i)
	{
		Particle& me = m_particles[i];
		float const meMass = me.m_mass;
		float size = sizes[i];

		scoped_lock lock1(mutexes[i]);

		for (int p = i + 1; p < count; p++)
		{
			Particle& other = m_particles[p];

			// Get vector between objects
			VectorType objectsVector = other.GetPos() - me.GetPos();

			float distanceSq = objectsVector.MagSq();

			float combinedRadius = size + sizes[p];
			if (distanceSq < combinedRadius * combinedRadius)
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
						set.insert(i);
						set.insert(p);
						mergedIntoExistingSet = true;
						break;
					}
				}

				if (!mergedIntoExistingSet)
					mergeSets.push_back(unordered_set<int>({ i, p }));

				continue;
			}

			// Calculate gravitational attraction
			float force = (m_gravitationalConstant * meMass * other.m_mass) / distanceSq;

			// Apply force to velocity of particle (accel = force / mass)
			objectsVector.Normalise();

			VectorType objectsVectorOther = objectsVector;

			float accelMe = force / meMass;
			float accelOther = force / other.m_mass;

			scoped_lock lock2(mutexes[p]);

			objectsVector.SetLength(accelMe);
			objectsVectorOther.SetLength(accelOther);

			me.AddToVel(objectsVector);
			other.AddToVel(-objectsVectorOther);
		}
	};

	vector<future<void>> futures;

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
	priority_queue<size_t> deleteQueue;

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

void Universe::AdvanceGravityGridBasedMode()
{
	// Check every other particle and for each one, adjust my velocity
	// according to the gravitational attraction.

	// Gravitational equation:
	// Force = (GMm / r^2)
	// Where G is the universal gravitational constant
	// M and m are the masses of the two objects
	// r is the distance between the two objects

	// Merging particles: when a collision is detected, we make a note that we will merge the other particle into
	// the current particle at the end of the frame. If the current particle collides with multiple other particles
	// during the same frame, they'll all be added to the same merge set.
	// Used to reference the particles by index but do it by pointer because we don't know the other
	// particle's index in m_particles when we find it in a grid square
	vector<unordered_set<Particle*>> mergeSets;

	mutex mergeMutex;

	const size_t count = m_particles.size();
	const int gridRowsCols = m_gridRowsCols;

	// Get grid extents
	double minX, maxX, minY, maxY, gridW, gridH, stepX, stepY;
	GetGridExtents(m_particles, minX, maxX, minY, maxY, gridW, gridH, stepX, stepY);

	auto particleGridPos = [&](Particle const& p)
	{
		// Count a particle off the bottom/right as being in the bottom/right square
		return pair<int, int>{ min(static_cast<int>((p.m_pos.x - minX) / stepX), gridRowsCols - 1),
							   min(static_cast<int>((p.m_pos.y - minY) / stepY), gridRowsCols - 1) };
	};

	struct GridSquare
	{
		vector<Particle*> particles;
		vector<size_t> particleIndices;
		VectorType centre;
		float mass = 0;
		size_t index = 0;
	};

	vector<vector<GridSquare>> grid;
	for (int rowI = 0; rowI < gridRowsCols; ++rowI)
	{
		vector<GridSquare> row(gridRowsCols);
		for (int colI = 0; colI < gridRowsCols; ++colI)
			row[colI].index = rowI * gridRowsCols + colI;
		grid.push_back(row);
	}

	// Track which grid squares which contain particles so we don't waste time checking particles against 
	// empty squares
	unordered_set<GridSquare*> nonEmptyGridSquaresSet;
	vector<GridSquare*> nonEmptyGridSquares;

	// Assign each particle to a grid rectangle
	for (size_t i = 0; i < m_particles.size(); ++i)
	{
		auto& p = m_particles[i];
		auto [gx, gy] = particleGridPos(p);
		if (gx < 0 || gy < 0 || (size_t)gx >= grid[0].size() || (size_t)gy >= grid.size())
			continue;	// shouldn't ever happen
		grid[gy][gx].particles.push_back(&p);
		grid[gy][gx].particleIndices.push_back(i);
		grid[gy][gx].mass += p.GetMass();
		grid[gy][gx].centre = { minX + gx * stepX + (stepX / 2.), minY + gy * stepY + (stepY / 2.) };
		grid[gy][gx].index = gy * gridRowsCols + gx;
		nonEmptyGridSquaresSet.insert(&grid[gy][gx]);
	}

	for (GridSquare* p : nonEmptyGridSquaresSet)
		nonEmptyGridSquares.push_back(p);

	const float sizeLogBase = m_sizeLogBase;

	// Cache sizes to avoid having to call GetSize (with slow logarithm calls) multiple times per particle
	// 10k particles, with only a single log, update = 87ms
	// with two logs, update = 103ms
	// with caching sizes, update = 95ms
	// Unlike normal mode, we have to use a map (much slower than the vector) for some particles because we
	// don't always know the particle index
	// No longer need to use sizesMap since grid squares track particle indices
	vector<float> sizes(count);
	//unordered_map<Particle*, float> sizesMap;
	for (size_t i = 0; i < m_particles.size(); ++i)
	{
		//sizesMap[&m_particles[i]] =
		sizes[i] = m_particles[i].GetSize(sizeLogBase);
	}

#ifndef GRID_BASED_MODE_NEW
	// Old grid based mode
	// Run a thread for each particle

	// Each particle had its own mutex because another particle might change its velocity, but we're not
	// doing two-way interactions in the current system
	//vector<mutex> mutexes(count);

	auto execute = [&](int i)
		{
			Particle& me = m_particles[i];
			float const meMass = me.m_mass;
			float size = sizes[i];

			//scoped_lock lock1(mutexes[i]);

			// go through each grid square
			// if it's our own grid square or within certain distance, go through particles as normal, otherwise
			// be attracted based on total mass of other grid square
			auto [myGX, myGY] = particleGridPos(me);

			// In the original simulation the interaction between any pair of particles is calculated only once, we
			// avoid doing the interaction twice by doing nested for loops
			// for outer = 0 to size-1
			//   for inner = outer+1 to size-1
			// Here we have three scenarios which need to be considered
			// 1. Interaction with particles in same square. We could make a thread for each non-empty grid square and
			//    run the simulation in the same way that we used to.
			// 2. Interaction with particles in neighbouring squares. Could handle double interactions by skipping square
			//    interactions with lower indexed squares
			// 3. Interaction with distant grid cells. Could be combined with 1.

			// The above would probably be much faster than the way I'm doing it at the moment

			for (auto& otherGridSquareP : nonEmptyGridSquaresSet)
			{
				auto otherGridSquare = *otherGridSquareP;

				// get grid coordinates from the first particle
				auto [otherGX, otherGY] = particleGridPos(*otherGridSquare.particles.front());

				// If other grid square is within this many grid squares, go through particles individually
				int gridDistance = abs(myGX - otherGX) + abs(myGY - otherGY);
				if (gridDistance <= m_highAccuracyGridDistance)
				{
					for (size_t p = 0; p < otherGridSquare.particles.size(); p++)
					{
						Particle& other = *(otherGridSquare.particles[p]);
						const int index2 = otherGridSquare.particleIndices[p];

						if (&me == &other)
							continue;

						// Get vector between objects
						VectorType objectsVector = other.GetPos() - me.GetPos();
						float distance = objectsVector.Mag();

#if 1
						if (distance < size + sizes[index2])
						{
							scoped_lock mergeLock(mergeMutex);

							//argDebugf("Merge %d,%d", i, p);

							// Find if there's an existing merge set which contains at least one of the two particles,
							// if so merge particles into that existing merge set
							bool mergedIntoExistingSet = false;
							for (auto& set : mergeSets)
							{
								bool foundI = set.find(&m_particles[i]) != set.end();
								bool foundP = set.find(&other) != set.end();
								if (foundI || foundP)
								{
									//argDebugf("Found in existing set: i:%d p:%d", foundI, foundP);
									set.insert(&m_particles[i]);
									set.insert(&other);
									mergedIntoExistingSet = true;
									break;
								}
							}

							// Otherwise create a new merge set
							if (!mergedIntoExistingSet)
								mergeSets.push_back({ &m_particles[i], &other });

							// Don't do gravitational force with another particle if we're going to merge with it
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

						// This lock was to allow for two-way particle interations without having to do the calculations
						// twice. We can't do that here because we don't know the other particle's index in m_particles
						// any more
						//scoped_lock lock2(mutexes[p]);

						objectsVector.SetLength(accelMe);
						//objectsVectorOther.SetLength(accelOther);

						me.AddToVel(objectsVector);
						//other.AddToVel(-objectsVectorOther);
					}
				}
				else
				{
					// Gravitational attraction from this particle to a whole grid square
					VectorType vec = grid[otherGY][otherGX].centre - me.GetPos();
					float distance = vec.Mag();

					// Calculate gravitational attraction
					float force = (m_gravitationalConstant * meMass * grid[otherGY][otherGX].mass) / (distance * distance);

					// Apply force to velocity of particle (accel = force / mass)
					vec.Normalise();

					float accelMe = force / meMass;
					vec.SetLength(accelMe);
					me.AddToVel(vec);
				}
			}
		};

	vector<future<void>> futures;

	for (int i = 0; i < count; i++)
#if ASYNC_POLICY_DEFAULT
		futures.push_back(std::async(execute, i));
#else
		futures.push_back(std::async(asyncPolicy, execute, i));
#endif
#else
	// New approach
	// Run a thread for each grid square
	// Each grid square runs the traditional simulation for particles in itself, including two-way interactions
	// It also runs two way interactions with nearby grid squares with a higher index
	// And for each particle we apply force for distant grid squares
	vector<future<void>> futures;

	vector<mutex> mutexes(count);

	auto executeGridSquare = [&](int row, int col)
		{
			const int gridIdx = row * m_gridRowsCols + col;
			auto& gridSquare = grid[row][col];
			const auto gridSquareCount = gridSquare.particles.size();

			// Go through particles in own square
			for (size_t i = 0; i < gridSquare.particles.size(); ++i)
			{
				Particle& me = *(gridSquare.particles[i]);
				const auto index1 = gridSquare.particleIndices[i];
				const float meMass = me.m_mass;
				const float size = sizes[index1];

				VectorType accumulatedVelChange;

				scoped_lock lock1(mutexes[index1]);

				// Go through particles in same square
				for (size_t p = i + 1; p < gridSquareCount; p++)
				{
					Particle& other = *(gridSquare.particles[p]);
					const auto index2 = gridSquare.particleIndices[p];

					// Get vector between objects
					VectorType objectsVector = other.GetPos() - me.GetPos();
					
					float distanceSq = objectsVector.MagSq();
					
					objectsVector.Normalise();
					
					float combinedRadius = size + sizes[index2];

					// Check for collision
					if (distanceSq < combinedRadius * combinedRadius)
					{
						scoped_lock mergeLock(mergeMutex);

						// Find if there's an existing merge set which contains at least one of the two particles,
						// if so merge particles into that existing merge set
						// TODO now that we have particleIndices we can go back to using ints here
						bool mergedIntoExistingSet = false;
						for (auto& set : mergeSets)
						{
							bool foundI = set.find(&m_particles[index1]) != set.end();
							bool foundP = set.find(&other) != set.end();
							if (foundI || foundP)
							{
								//argDebugf("Found in existing set: i:%d p:%d", foundI, foundP);
								set.insert(&m_particles[index1]);
								set.insert(&other);
								mergedIntoExistingSet = true;
								break;
							}
						}

						// Otherwise create a new merge set
						if (!mergedIntoExistingSet)
							mergeSets.push_back({ &m_particles[index1], &other });

						// Don't do gravitational force with another particle if we're going to merge with it
						continue;
					}

					// Calculate gravitational attraction
					float force = (m_gravitationalConstant * meMass * other.m_mass) / distanceSq;

					// Apply force to velocity of particle (accel = force / mass)

					float accelMe = force / meMass;
					float accelOther = force / other.m_mass;

					objectsVector.SetLength(accelMe);

					scoped_lock lock2(mutexes[index2]);

					accumulatedVelChange += objectsVector;

					VectorType objectsVectorOther = objectsVector;
					objectsVectorOther.SetLength(accelOther);
					other.AddToVel(-objectsVectorOther);
				}

				// Go through particles in nearby squares (only for squares with higher index) and distant squares
				// Is there any benefit in going through nearby squares rather than just having bigger grid squares with
				// no buffer zone? Well, yes, if two particles overlap in different squares.
				// Also force vector will be super inaccurate for neighbouring grid squares
				for (GridSquare* otherGridSquareP : nonEmptyGridSquares)
				{
					auto const& otherGridSquare = *otherGridSquareP;

					if (otherGridSquare.index == gridSquare.index)
						continue;

					// get grid coordinates from the first particle
					auto [otherGX, otherGY] = particleGridPos(*otherGridSquare.particles.front());

					// If other grid square is within this many grid squares, go through particles individually
					int gridDistance = abs(col - otherGX) + abs(row - otherGY);
					if (gridDistance <= m_highAccuracyGridDistance)
					{
						// Don't do two-way interactions with other grid squares with lower index
						if (otherGridSquare.index < gridSquare.index)
							continue;

						for (size_t p = 0; p < otherGridSquare.particles.size(); p++)
						{
							Particle& other = *(otherGridSquare.particles[p]);
							const auto index2 = otherGridSquare.particleIndices[p];

							if (&me == &other)	// shouldn't be necessary, we won't be checking current grid square here
								continue;

							// Get vector between objects
							VectorType objectsVector = other.m_pos - me.m_pos;

							float distanceSq = objectsVector.MagSq();
							float combinedRadius = size + sizes[index2];

							// Check for collision
							if (distanceSq < combinedRadius * combinedRadius)
							{
								scoped_lock mergeLock(mergeMutex);

								//argDebugf("Merge %d,%d", i, p);

								// Find if there's an existing merge set which contains at least one of the two particles,
								// if so merge particles into that existing merge set
								bool mergedIntoExistingSet = false;
								for (auto& set : mergeSets)
								{
									bool foundI = set.find(&m_particles[index1]) != set.end();
									bool foundP = set.find(&other) != set.end();
									if (foundI || foundP)
									{
										//argDebugf("Found in existing set: i:%d p:%d", foundI, foundP);
										set.insert(&m_particles[index1]);
										set.insert(&other);
										mergedIntoExistingSet = true;
										break;
									}
								}

								// Otherwise create a new merge set
								if (!mergedIntoExistingSet)
									mergeSets.push_back({ &m_particles[index1], &other });

								// Don't do gravitational force with another particle if we're going to merge with it
								continue;
							}

							// Calculate gravitational attraction
							float force = (m_gravitationalConstant * meMass * other.m_mass) / distanceSq;

							// Apply force to velocity of particle (accel = force / mass)
							objectsVector.Normalise();

							VectorType objectsVectorOther = objectsVector;

							float accelMe = force / meMass;
							float accelOther = force / other.m_mass;
							
							objectsVector.SetLength(accelMe);
							accumulatedVelChange += objectsVector;

							// Apply interaction to other particle
							scoped_lock lock2(mutexes[index2]);

							objectsVectorOther.SetLength(accelOther);
							other.AddToVel(-objectsVectorOther);
						}
					}
					else
					{
						// Gravitational attraction from this particle to a whole grid square
						VectorType vec = grid[otherGY][otherGX].centre - me.GetPos();
						
						float distanceSq = vec.MagSq();
						
						vec.Normalise();

						// Calculate gravitational attraction
						float force = (m_gravitationalConstant * meMass * grid[otherGY][otherGX].mass) / distanceSq;

						float accelMe = force / meMass;
						vec.SetLength(accelMe);
						accumulatedVelChange += vec;
					}
				}

				// Add accumulated vel change to vel
				me.AddToVel(accumulatedVelChange);
			}
		};

	// todo just do empty ones
	for (int rowI = 0; rowI < gridRowsCols; ++rowI)
		for (int colI = 0; colI < gridRowsCols; ++colI)
	#if ASYNC_POLICY_DEFAULT
			futures.push_back(std::async(executeGridSquare, rowI, colI));
	#else
			futures.push_back(std::async(asyncPolicy, executeGridSquare, rowI, colI));
	#endif

#endif

	// Await all
	for (size_t i = 0; i < futures.size(); ++i)
		futures[i].wait();

	// Priority queue because we are deleting based on index
	// We want to delete high indicies first so as not to invalidate lower indicies
	priority_queue<size_t> deleteQueue;

	for (auto& set : mergeSets)
	{
		//argDebugf("Merge set: " + ToString(set) + " into %d", *i);	// old

		auto it = set.cbegin();
		Particle* firstParticle = *it;
		while (++it != set.cend())
		{
			firstParticle->Merge(**it);

			// Find index of other particle in m_particles so we can delete it later
			size_t i;
			for (i = 0; i < m_particles.size(); ++i)
			{
				if (&m_particles[i] == *it)
					break;
			}
			assert(i < m_particles.size());
			deleteQueue.push(i);
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
	const float sizeLogBase = m_sizeLogBase;

	// Render trails
	if (m_showTrails)
	{
		int iTrail = 0;
		for( auto const& particle : m_trails )
		{
			if (iTrail++ % drawTrailInterval == 0)
				RenderParticle(particle, sizeLogBase, true);
		}
	}

	// Grid lines
	if (m_useGridBasedMode)
	{
		const int gridRowsCols = m_gridRowsCols;
		ALLEGRO_COLOR gridCol = al_map_rgb(32, 32, 32);
		double minX, maxX, minY, maxY, gridW, gridH, stepX, stepY;
		GetGridExtents(m_particles, minX, maxX, minY, maxY, gridW, gridH, stepX, stepY);
		int gx = 0, gy = 0;
		for (double x = minX; gx <= gridRowsCols; ++gx, x += stepX)
		{
			auto pos1 = WorldToScreen({ x, minY });
			auto pos2 = WorldToScreen({ x, maxY });
			al_draw_line(pos1.x, pos1.y, pos2.x, pos2.y, gridCol, 1.f);
		}
		for (double y = minY; gy <= gridRowsCols; ++gy, y += stepY)
		{
			auto pos1 = WorldToScreen({ minX, y });
			auto pos2 = WorldToScreen({ maxX, y });
			al_draw_line(pos1.x, pos1.y, pos2.x, pos2.y, gridCol, 1.f);
		}
	}

	// Render each particle
	for (auto const& p : m_particles)
		RenderParticle(p, sizeLogBase);

	// Display text stuff

	// top right - version number and nearest particle details
	al_draw_text(g_font, g_colWhite, al_get_display_width(g_display), 0, ALLEGRO_ALIGN_RIGHT, versionText);

	// Use mouse to show particle stats
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
								stringFormat("Gravity: %e", m_gravitationalConstant),
								"",
								stringFormat("Grid mode: %s (G)", m_useGridBasedMode ? "On" : "Off")
							};

	float y = 100;
	for (auto const& str : entries)
	{
		al_draw_textf(g_font, g_colWhite, 0, y, ALLEGRO_ALIGN_LEFT, str.c_str());
		y += g_fontSize;
	}

	// bottom right
	entries = { "TAB: Show/hide config menu",
				"+/-: Zoom",
				"Cursor keys: Move",
				"Left/right mouse: Add/remove particles",
				"G: Toggle grid-based mode",
				"Z: Fast forward",
				"F1: Show/hide particle info",
				"F2: Freeze",
				"F3: Show/hide trails",
				"ESC: Quit" };
	y = al_get_display_height(g_display) - g_fontSize * entries.size();

	for (auto const& str : entries)
	{
		al_draw_textf(g_font, g_colWhite, m_scW, y, ALLEGRO_ALIGN_RIGHT, str.c_str());
		y += g_fontSize;
	}

	// Show config menu
	if (m_showConfigMenu)
		m_configMenu->render();

	RenderMenu();
}

void Universe::OnClose()
{
	if (saveOnQuit)
		Save();
}

void Universe::RenderParticle(Particle const & _particle, float _sizeLogBase, bool _isTrail)
{
	float viewportHeight = m_viewportWidth / m_worldAspectRatio;

	float size = _particle.GetSize(_sizeLogBase) * (m_scW / m_viewportWidth);

	VectorType pos = _particle.GetPos();

	// Uncomment this to show the smallest particles as circles rather than dots
	//size = max(size, 1.1f);

	enum class SizeClass
	{
		Large,
		Normal,
		Small,
		None
	};

	auto sizeClass = SizeClass::None;
	// See if particle is within viewport
	// todo use screen pos of particle
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
				al_draw_circle(x, y, 10, _particle.m_col, particleEdgeThickness);
				al_draw_line((int)(x - 10), (int)y, (int)(x + 10), y, _particle.m_col, 1.f);
				al_draw_line(x, (int)(y - 7.5f), x, (int)(y + 7.5f), _particle.m_col, 1.f);
				break;
			}
			case SizeClass::Normal:
			{
				al_draw_circle(x, y, size, _particle.m_col, particleEdgeThickness);
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
			ostringstream ss;
			ss << "m:" << setprecision(2) << _particle.m_mass << " sp:" << setprecision(8) << _particle.GetVel().Mag();
			al_draw_textf(g_font, g_colWhite, (int)x, (int)y, 0, ss.str().c_str(), _particle.m_mass, _particle.GetVel().Mag());
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

			break;
		}
#endif
		case 3: // Grid
		{
			for (int x = 0; x <= 800; x += 80)
				for (int y = 0; y <= 600; y += 60)
					AddParticle(VectorType(x, y), VectorType(0, 0), 1e4);

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

			float const distMult = 150.f;

			auto addPlanet = [&](float _distAU, float _earthMasses, ALLEGRO_COLOR _col = al_map_rgb(255,255,255))
			{
				_distAU *= distMult;
				_earthMasses *= massMult;
				float speed = sqrtf((m_gravitationalConstant * sunMass) / _distAU) * 1.f;
				AddParticle(VectorType(sunX + _distAU, sunY), VectorType(0.f, speed), _earthMasses, _col);
				return p;
			};

			// Sun
			AddParticle(VectorType(sunX, sunY), VectorType(0, 0), sunMass, al_map_rgb(255, 255, 0));

#if 1
			// Mercury
			addPlanet(0.387f, 0.054f, al_map_rgb(192, 128, 128));
#endif
#if 1
			// Venus
			addPlanet(0.723f, 0.814f, al_map_rgb(0, 255, 64));
#endif
#if 1
			// Earth
			addPlanet(1.f, 1.f, al_map_rgb(64, 192, 255));
#endif
#if 1
			// Mars
			addPlanet(1.52f, 0.17f, al_map_rgb(255,64,64));

			// Jupiter
			p = addPlanet(5.2f, 317.8f, al_map_rgb(255, 128, 64));

			// Saturn
			addPlanet(9.54f, 95.2f, al_map_rgb(192, 192, 0));
#endif
#if 1
			// Uranus
			addPlanet(19.19f, 14.48f, al_map_rgb(64, 255, 64));
#endif
#if 1
			// Neptune
			addPlanet(30.07f, 17.2f, al_map_rgb(128, 128, 255));

			// Pluto
			addPlanet(39.48f, 0.0025f);
#endif

#if 0
			for (int i = 0; i < 10; ++i)
			{
				addPlanet((float)getrandom(1, 100) / 10.f, ((float)getrandom(1, 200) / 10.f));
			}
#endif

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
				MakeSpiralUniverse(1e9f, m_numSpiralParticles, 5.f, 8.f, 0.25f, spiralMassDecrease, 0.6f);
			else if (_id == 7)
				MakeSpiralUniverse(1e9f, m_numSpiralParticles, 5.f, 14.f, 1.25f, spiralMassDecrease, 1.5f);
			
			break;
		}
		case 8:
		{
			// spiral with normal gravity
			m_viewportWidth = m_defaultViewportWidth * 10.f;

			MakeSpiralUniverse(1e6f, m_numSpiralParticles, 8.f, 13.f, 1.5f, spiralMassDecrease, 1.5f);

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

const string saveLoadFilename = "save.txt";

void Universe::Save()
{
	// todo change to use Particle operator <<
	ofstream file(saveLoadFilename, std::ios::trunc);
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
	// todo change to use Particle operator >>
	if (!std::filesystem::exists(saveLoadFilename))
		return;

	m_particles.clear();
	m_trails.clear();
	ifstream file(saveLoadFilename);
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
	// We now have two menus, old and new!
	if (m_showConfigMenu)
		m_configMenu->update(al_get_display_height(g_display));

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
						"5: Spiral 1 (low gravity, high mass)",
						"6: Spiral 2 (low gravity, high mass)",
						"7: Spiral 3 (low gravity, high mass)",
						"8: Spiral 4 (normal gravity)",
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

std::unique_ptr<PSectorMenu> Universe::CreateConfigMenu()
{
	PSectorMenuLayoutOptions menuLayoutOptions =
	{
		g_font,
		300,	// yStart
		35,		// yInc
		32		// buttonHeight
	};
	auto menu = make_unique<PSectorMenu>(menuLayoutOptions);
	int headingX = 100;
	int textX = 120;

	menu->addHeading(headingX, "Config menu");
	menu->addHeading(headingX, "todo: make this menu less frame rate dependent.");
	menu->addGap();

	menu->addHeading(headingX, "Save/reset");
	menu->addAction(textX, "Save all options to config file", [&] { for (auto& option : m_allOptions) option->save(); });
	//menu->addAction(textX, "Load all options to config file (NOT YET IMPLEMENTED)", [&] {  });
	menu->addAction(textX, "Reset all to defaults", [&] { for (auto& option : m_allOptions) option->resetToDefault(); });
	
	menu->addHeading(headingX, "Grid");
	menu->add(textX, m_gridRowsCols, 1, 100);
	menu->add(textX, m_highAccuracyGridDistance, 0, 100);

	menu->addHeading(headingX, "Spiral");
	menu->add(textX, m_numSpiralParticles, 0, 100000, 250);

	menu->addHeading(headingX, "Particles");
	menu->add(textX, m_sizeLogBase, 1.05f, 10.f, 0.05f);

	menu->addHeading(headingX, "Trails");
	menu->add(textX, m_createTrailInterval, 1, 1000);
	menu->add(textX, m_maxTrails, 0, 500000, 1000);
	menu->addAction(textX, "Clear trails", [&] { m_trails.clear(); });

	return menu;
}
