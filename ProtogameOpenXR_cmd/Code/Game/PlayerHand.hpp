#pragma once
#include "ThirdParty/OpenXR/include/openxr/openxr.h"
#include "Engine/input/XRInputSystem.hpp"
#include "Game/GameCommon.hpp"

class PlayerHand
{
public:
	PlayerHand() {}
	~PlayerHand() {}

	void Update();
	void UpdateHandPosInAppSpace();
	void AddVertsForHand();

	void SetModelMatrix();

	bool m_overlapWithCube = false;
	bool m_isActive = false;

	float m_grabValue = 0.f;
	int m_handIndex = Side::COUNT;

	Vec3    m_handPos;
	XrPosef m_actionSpacePose;
};