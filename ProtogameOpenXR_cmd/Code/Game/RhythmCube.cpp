#include "Engine/Math/AABB3.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Easing.hpp"
#include "Game/RhythmCube.hpp"
#include "Game/Game.hpp"

extern Game* g_theGame;
extern Clock* g_theGameClock;

void RhythmCube::Update()
{
	if (m_status == CubeStatus::DEACTIVATED)
	{
		ChangeAccordingToDefaultBeats();
	}
	if (m_status == CubeStatus::ACTIVATED)
	{
		ChangeAccordingToActivation();
	}

	if (g_theGame->m_currentState == GameState::PLAYING)
	{
	}

	AddCubeVerts();
}

void RhythmCube::ChangeAccordingToDefaultBeats()
{
	// the origin position of the cube is going to change if the player enter showig scores state
	if (g_theGame->m_currentState == GameState::PLAYING || g_theGame->m_currentState == GameState::LOBBY)
	{
		m_localposInCubeSpace = m_originPos;
	}
	else if(g_theGame->m_currentState == GameState::SHOWSCORES)
	{
		m_localposInCubeSpace = m_posWhenShowingScores;
	}
	m_currentScale = m_defaultScale;

	// all cubes by default will scale up according to the universal game beats timer
	float beatsFraction = g_theGame->m_defaultBeatsTimer->GetElapsedFraction();

	if (!g_theGame->m_defaultBeatsTimer->IsRewinding())
	{
		float easedFractionXY = EaseInQuintic(beatsFraction);
		// float easedFractionZ = Hesitate5(0.f, 1.f, beatsFraction);
		float easedFractionZ = EaseOutQuintic(beatsFraction);
		float scaleMultiplerXY = RangeMapClamped(easedFractionXY, 0.f, 1.f, 1.f, SCALE_DEFAULT_BEAT_MAX);
		float scaleMultiplerZ = RangeMapClamped(easedFractionZ, 0.f, 1.f, 1.f, SCALE_DEFAULT_BEAT_MAX);

		m_currentScale.x *= scaleMultiplerXY;
		m_currentScale.y *= scaleMultiplerXY;
		m_currentScale.z *= scaleMultiplerZ;

	}
	else
	{
		float easedFractionXY = EaseOutQuintic(beatsFraction);
		// float easedFractionZ = Hesitate5(0.f, 1.f, beatsFraction);
		float easedFractionZ = EaseInQuintic(beatsFraction);
		float scaleMultiplerXY = RangeMapClamped(easedFractionXY, 0.f, 1.f, 1.f, SCALE_DEFAULT_BEAT_MAX);
		float scaleMultiplerZ = RangeMapClamped(easedFractionZ, 0.f, 1.f, 1.f, SCALE_DEFAULT_BEAT_MAX);

		m_currentScale.x *= scaleMultiplerXY;
		m_currentScale.y *= scaleMultiplerXY;
		m_currentScale.z *= scaleMultiplerZ;
	}

	// deactivation color
	m_colorY = Rgba8::CYSTAL_BLUE;
	m_colorX = Rgba8::TEAL_BLUE;
	m_colorZ = Rgba8::PURPLE_BLUE;
}

void RhythmCube::ChangeAccordingToActivation()
{
	if (m_activationtimer)
	{
		if (!m_activationtimer->IsRewinding())
		{
			if (m_activationtimer->HasPeroidElapsed())
			{
				m_activationtimer->Rewind();
			}
		}
		else
		{
			// if the timer is rewinding to the start, deactivate cube
			if (m_activationtimer->HasTimerRewindToStart())
			{
				m_status = CubeStatus::DEACTIVATED;
				m_playerTouched = false;
				delete m_activationtimer;
				m_activationtimer = nullptr;
				return;
			}
		}
	}
	else
	{
		return;
	}

	 // during the activation, the cube will slide out
	 float activationFraction = m_activationtimer->GetElapsedFraction();
	 float slideOutDist = m_activationtimer->m_period * DIST_SLIDEOUT;
	 Vec3 originPos = m_originPos;	
	 Vec3 slideOutPos = m_originPos + Vec3(0.f, slideOutDist * -1.f, 0.f);
	 float easedFraction = 0.f;

	 if (!m_activationtimer->IsRewinding())
	 {
	 	// float easedFractionXY = EaseInQuintic(activationFraction);
	 	// // float easedFractionZ = Hesitate5(0.f, 1.f, beatsFraction);
	 	// float easedFractionZ = EaseOutQuintic(activationFraction);
	 	// float scaleMultiplerXY = RangeMapClamped(easedFractionXY, 0.f, 1.f, 1.f, SCALE_DEFAULT_BEAT_MAX);
	 	// float scaleMultiplerZ = RangeMapClamped(easedFractionZ, 0.f, 1.f, 1.f, SCALE_DEFAULT_BEAT_MAX);
	 	// 
	 	// m_currentScale.x *= scaleMultiplerXY;
	 	// m_currentScale.y *= scaleMultiplerXY;
	 	// m_currentScale.z *= scaleMultiplerZ;

		 easedFraction = EaseOutQuintic(activationFraction);
	 }
	 else
	 {
	 	// float easedFractionXY = EaseOutQuintic(activationFraction);
	 	// // float easedFractionZ = Hesitate5(0.f, 1.f, beatsFraction);
	 	// float easedFractionZ = EaseInQuintic(activationFraction);
	 	// float scaleMultiplerXY = RangeMapClamped(easedFractionXY, 0.f, 1.f, 1.f, SCALE_DEFAULT_BEAT_MAX);
	 	// float scaleMultiplerZ = RangeMapClamped(easedFractionZ, 0.f, 1.f, 1.f, SCALE_DEFAULT_BEAT_MAX);
	 	// 
	 	// m_currentScale.x *= scaleMultiplerXY;
	 	// m_currentScale.y *= scaleMultiplerXY;
	 	// m_currentScale.z *= scaleMultiplerZ;

		 easedFraction = EaseInQuintic(activationFraction);
	 }
	 
	 m_localposInCubeSpace = Interpolate(originPos, slideOutPos, easedFraction);

	 // player once touch the cube, it will change color
	 if (m_playerTouched)
	 {
		 m_colorY = Rgba8::CYSTAL_BLUE;
		 m_colorX = Rgba8::TEAL_BLUE;
		 m_colorZ = Rgba8::PURPLE_BLUE;
	 }
	 else
	 {
		 // activation color
		 m_colorY = Rgba8::CANDLE_YELLOW;
		 m_colorX = Rgba8::BRIGHT_ORANGE;
		 m_colorZ = Rgba8::BURNT_RED;
	 }

	// delete the timer when it rewinds to start
	if (m_activationtimer->HasTimerRewindToStart())
	{
		delete m_activationtimer;
		m_activationtimer = nullptr;
	}
}

void RhythmCube::ActivateCubeToBeHit(float duration)
{
	m_status = CubeStatus::ACTIVATED;
	m_activationtimer = new Timer(duration * 0.5f, g_theGameClock); // we will rewind the timer to make a full beat period
	m_activationtimer->Start();
	m_playerTouched = false;
}

void RhythmCube::AddCubeVerts()
{
	Vec3 cubeHalfSize = Vec3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE) * m_currentScale * 0.5f;
	AABB3 bounds(cubeHalfSize * -1.f, cubeHalfSize);
	bounds.SetTranslation(m_localposInCubeSpace);
	AddVertsForAABB3D(g_theGame->m_cubeVerts, bounds, m_colorX, m_colorX, m_colorY, m_colorY, m_colorZ, m_colorZ);
}

