#pragma once
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include <string>

class Renderer;

enum TestingScene
{
	RAYCAST_VS_3DSHAPES,
	PACHINKO,
	CURVES2D,
	RAYCAST2D_VS_AABB2S,
	RAYCAST2D_VS_LINESEGMENTS,
	RAYCAST2D_VS_DISCS,
	GET_NEARESTPOINT,
	NUM_TESTINGMODES
};

struct GameModeConfig
{
	int			m_numMessageOnScreen = 32;
	float		m_lineHeightAndTextBoxRatio = 0.8f;
	float		m_cellAspect = 0.6f;
	BitmapFont* m_font = nullptr;
	Renderer* m_renderer = nullptr;
};

class GameMode
{
public:
	GameMode();
	virtual ~GameMode();
	virtual void Startup() = 0;
	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;
	virtual void RenderDebug() const;
	virtual void Shutdown() = 0;

	virtual void CreateRandomShapes() = 0;

	virtual void UpdateModeInfo() = 0;
	virtual void RenderScreenMessage() const;

	static GameMode* CreateNewGame(TestingScene type);

public:
	// reference raycast control
	void ControlTheReferenceRay(float deltaSeconds);
	// ray properties
	Vec2 m_tailPos;
	Vec2 m_tipPos;

	// camera setting
	Camera m_worldCamera;
	Camera m_screenCamera;

	// mode info
	std::string m_modeName;
	std::string m_controlInstruction;
	GameModeConfig m_gameModeConfig;

	// verts
	std::vector<Vertex_PCU> m_rayVerts;// contain all the reference point and nearest points
};
