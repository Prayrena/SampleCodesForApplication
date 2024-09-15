#pragma once
#include "ThirdParty/OpenXR/include/openxr/openxr.h"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"

class Timer;

enum class CubeStatus
{
	ACTIVATED,
	DEACTIVATED,

	NUM_STATUS
};

class RhythmCube 
{
public:
	RhythmCube() {}
	~RhythmCube() {}
	
	void Update();
	void ChangeAccordingToDefaultBeats();
	void ChangeAccordingToActivation();

	void ActivateCubeToBeHit(float duration);

	void AddCubeVerts();

	// Deactivate mode
	CubeStatus m_status = CubeStatus::DEACTIVATED;

	// Activate mode
	Timer* m_activationtimer = nullptr;

	IntVec2 m_coords;
	Vec3 m_defaultScale = Vec3(1.f, 1.f, 1.f);
	Vec3 m_currentScale = Vec3(1.f, 1.f, 1.f);

	Vec3 m_localposInCubeSpace;
	Vec3 m_originPos;
	Vec3 m_posWhenShowingScores;

	Rgba8 m_colorX;
	Rgba8 m_colorY;
	Rgba8 m_colorZ;

	bool m_playerTouched = false;
};