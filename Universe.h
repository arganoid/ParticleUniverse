#pragma once

#include <deque>
#include <vector>
#include <list>

#include "ARGCoreSrc\ARGUtils.h"
#include "ARGCoreSrc\Vector2Dbl.h"

#include <allegro5/allegro.h>

using VectorType = Vector2;

struct ALLEGRO_VERTEX;

struct Particle
{
	VectorType m_pos;
	VectorType m_vel;

	float m_mass;

	ALLEGRO_COLOR m_col;

	bool m_trails;
	bool m_immovable;

	Particle() :m_mass(1)
	{
	}

	Particle(VectorType _pos, VectorType _vel, float _mass, ALLEGRO_COLOR _col,
		     bool _trails = true, bool _immovable = false):
			m_pos(_pos),
			m_vel(_vel),
			m_mass(_mass),
			m_col(_col),
			m_trails(_trails),
			m_immovable(_immovable)
	{
	}

	Particle(Particle&&) noexcept = default;
	Particle& operator=(Particle&&) noexcept = default;

	__forceinline VectorType GetPos() const { return m_pos; }
	__forceinline VectorType GetVel() const { return m_vel; }
	__forceinline void SetPos(VectorType _pos) { m_pos = _pos; }
	__forceinline void SetVel(VectorType _vel) { m_vel = _vel; }
	__forceinline void AddToVel(VectorType _vel) { m_vel += _vel; }

	__forceinline float GetMass() const { return m_mass; }
	__forceinline void SetMass(float _mass) { m_mass = _mass; }

	__forceinline float GetSize() const { return log(m_mass) * 2.5f; }

	void operator = (Particle const& _param)
	{
		m_pos = _param.GetPos();
		m_vel = _param.GetVel();
		m_mass = _param.GetMass();
		m_col = _param.m_col;
		m_trails = _param.m_trails;
		m_immovable = _param.m_immovable;
	}

	void Merge(Particle const& _other)
	{
		float ratio = m_mass / (m_mass + _other.m_mass);
		float otherRatio = 1.f - ratio;
		m_pos = m_pos * ratio + _other.m_pos * otherRatio;
		m_vel = m_vel * ratio + _other.m_vel * otherRatio;
		m_mass += _other.m_mass;
	}
};

class Universe
{
private:
	std::vector<Particle> m_particles;
	std::deque<Particle> m_trails;

	size_t m_maxTrails;
	int m_createTrailInterval;	// 1 = add to trails every frame, etc
	int m_createTrailIntervalCounter;
	bool m_trailsEnabled;

	double m_gravitationalConstant;

	double const m_defaultViewportWidth;
	double m_viewportWidth;

	int m_scW;
	int m_scH;
	float m_worldAspectRatio;

	VectorType m_cameraPos;
	//decltype(m_particles)::iterator m_cameraFollow;	// disabled as becomes invalidated when vector size changes

	bool m_debug;
	bool m_debugParticleInfo;
	bool m_freeze;

	bool m_fastForward;

	int m_gravityAdvancesPerFrame;


	/**************/
	/* Menu stuff */

	enum class MenuPage
	{
		Default,
		CreateUniverse,
	} m_currentMenuPage;

	void AdvanceMenu();

	void RenderMenu();

	/**************/


public:
	Universe(int _maxParticleTrails);
	void Advance();
	void Render();
	int GetFastForward() { return m_fastForward; }

private:
	void AddParticle(VectorType _pos, VectorType _vel, float _mass, ALLEGRO_COLOR _col, bool _trails, bool _immovable);
	void AddTrailParticle(VectorType _pos, float _mass);

	void AdvanceGravity();

	void RenderParticle(Particle const & _particle, bool _isTrail = false);

	void CreateUniverse(int _id);

	void Save();
	void Load();


	VectorType WorldToScreen(const VectorType& _world);
	VectorType ScreenToWorld(const VectorType& _screen);

	decltype(m_particles)::iterator FindNearest(VectorType const& _pos);
};
