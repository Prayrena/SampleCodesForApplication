#include "Game/Pillar.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Math/Easing.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Game/Game.hpp"

extern Game* g_theGame;
extern Clock* g_theGameClock;

void Pillar::Update()
{
	ChangeScaleAccordingToBeats();
	AddPillarVerts();
}

void Pillar::AddPillarVerts()
{
	Vec3 cubeHalfSize = m_currentScale * 0.5f;
	AABB3 bounds(cubeHalfSize * -1.f + m_pillarCenterPos, cubeHalfSize + m_pillarCenterPos);
	AddVertsForAABB3D(g_theGame->m_cubeVerts, bounds, Rgba8::BLUSH_PINK, Rgba8::BLUSH_PINK, Rgba8::ROYAL_PURPLE, Rgba8::ROYAL_PURPLE, Rgba8::WARM_PURPLE, Rgba8::WARM_PURPLE);
}

void Pillar::ChangeScaleAccordingToBeats()
{
	// if the column pos is close to the center of the world, it is going to keep the scale as min
	if (GetDistanceSquared3D(m_pillarCenterPos, Vec3()) < (PILLAR_FIXED_RADIUS * PILLAR_FIXED_RADIUS))
	{
		m_currentScale.z = PILLAR_MAX_SCALEZ * PILLAR_MIN_SCALEZ;
	}
	else
	{
		// each pillar will offset the beat fraction to have a weave effect
		float beatsFraction = g_theGame->m_defaultBeatsTimer->GetElapsedFraction();

		if (!g_theGame->m_defaultBeatsTimer->IsRewinding()) // forward
		{
			if ((beatsFraction + m_floatingTimerOffset) > 1.f)
			{
				float extra = beatsFraction + m_floatingTimerOffset - 1.f;
				beatsFraction = 1.f - extra;
			}
			else
			{
				beatsFraction += m_floatingTimerOffset;
			}

		}
		else // rewinding
		{
			if ((beatsFraction - m_floatingTimerOffset) < 0.f)
			{
				float under = 0.f - (beatsFraction - m_floatingTimerOffset);
				beatsFraction = under;
			}
			else
			{
				beatsFraction -= m_floatingTimerOffset;
			}
		}
		
		float scaleFraction = RangeMapClamped(beatsFraction, 0.f, 1.f, 0.01f, 0.5f * 3.1415926f);

		m_currentScale.z = PILLAR_MAX_SCALEZ * sinf(scaleFraction);
	}
}
