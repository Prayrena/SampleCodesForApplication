#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"

class Pillar
{
public:
	explicit Pillar(IntVec2 coords, float offset)
		: m_coords(coords)
		, m_floatingTimerOffset(offset)
	{
		m_pillarCenterPos.x = float(m_coords.x);
		m_pillarCenterPos.y = float(m_coords.y);
		m_pillarCenterPos.z = 0.f;
	}
	Pillar() 
	{
		m_pillarCenterPos.x = float(m_coords.x);
		m_pillarCenterPos.y = float(m_coords.y);
		m_pillarCenterPos.z = 0.f;
	}
	~Pillar() {}

	void Update();

	void AddPillarVerts();

	void ChangeScaleAccordingToBeats();

	IntVec2 m_coords;
	Vec3 m_pillarCenterPos;

	Vec3 m_defaultScale = Vec3(1.f, 1.f, 0.1f);
	Vec3 m_currentScale = Vec3(1.f, 1.f, 1.f);

	float m_floatingTimerOffset = 0.f;
};