#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/core/Timer.hpp"

class HUD
{
public:
	HUD() {}
	~HUD() {}

	void Update();
	void UpdateHUDViewSpacePose(XrPosef updatePose);

	void SetModelMatrix();

	void RenderInstructionToStartGame();
	void RenderCountDownToStartTheSong();

	bool m_countDownStarts = false;
	Timer* m_countDownTimer = nullptr;
	int m_timesCountingDown = 0;

	Vec3    m_displayPos = Vec3(0.f, 0.f, -3.5f);
	XrPosef m_HUDViewSpacePose;
};