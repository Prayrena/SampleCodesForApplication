#pragma once
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/Camera.hpp"

class Camera;
class Map;
class Actor;

class Player
{
public:
	Player(Vec3 position = Vec3());
	~Player();

	void Startup();
	void Update();
	void Render() const;

	Mat44 GetModelMatrix() const;

	Vec3		m_position;
	Vec3		m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	Vec3		m_deltaMovement;

	// but if this is an instance, how should I able to assign other camera to player's camera
	Camera m_playerCamera; // if this is camera*, then when I try to set up set transform will throw an error

	float m_moveSpeed = 2.f; // 2 units per second
	float m_turnRate = 90.f; // 90 degrees per second
	float m_floatingSpeed = 2.f; // 2 units per second going upwards or downwards
	float m_sprintModifier = 15.f;
	float m_onePerSprintModifier = 1.f; // 1 / sprint modifier, used to get back to normal speed

	float m_mousePitchSensitiveMultiplier = 0.075f;
	float m_mouseYawSensitiveMultiplier = 0.075f;

	float m_controllerPitchSensitiveMultiplier = 3.f;
	float m_controllerYawSensitiveMultiplier = 3.f;
	float m_controllerMovementMultiplier = 0.15f;
	float m_controllerRollMultiplier = 12.f;

	bool  m_controllingCamera = true;

	// ActorPtrList	m_projectiles;
	// Actor* m_projectile = nullptr;

private:
	void CalculateTransformAndRoationBasedOnInput();
	void UpdateCameraTransformation();
	void SpawnDebugRenderGeometry();
};