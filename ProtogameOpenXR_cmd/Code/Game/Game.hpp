#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/input/XRInputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Game/RhythmCube.hpp"
#include "Game/PlayerHand.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Pillar.hpp"
#include <vector>

struct XrPosef;
class Timer;
class HUD;

enum class SoundEffectID
{
	GAMEMUSIC,
	SCORESMUSIC,

	NUM_SOUNDEFFECTS
};

enum class GameState
{
	NONE,
	ATTRACT,
	LOBBY,
	PLAYING,
	SHOWSCORES,
	VICTORY,
	FAILURE,
	COUNT
};

class Game
{
public:
	Game();
	~Game();

	void Startup();
	void Update();
	void Render();

	void ActivateRythmCubeByBeats();
	void RandomSelectCubesToActivate(int numCubes, float duration);
	void ActivateAllCubesByOrder(float duration);
	int m_allHitindex = 0;
	Timer* m_selectToActivateTimer_singleHit = nullptr;
	Timer* m_selectToActivateTimer_doubleHit = nullptr;
	Timer* m_selectToActivateTimer_allHit = nullptr;
	Timer* m_songDecreaseVolumeTimer = nullptr;
	int	   m_beatsCounter = 0;
	float m_activationDuration = 0.f;

	void SetModelMatrix();
	void UpdateReferenceWorldSpacePoseForCubes(XrPosef pose);
	void UpdateReferenceHandSpacePoses(XrPosef handPose, int handIndex);

	// Assets
	void LoadAudioAssets();
	SoundID m_soundEffectsID[int(SoundEffectID::NUM_SOUNDEFFECTS)];

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Game state machine
	void EnterState(GameState state);
	void ExitState(GameState state);

	void EnterAttract();
	void EnterLobby();
	void EnterPlaying();
	void EnterVictory();
	void EnterShowScores();

	void ExitAttract();
	void ExitLobby();
	void ExitPlaying();
	void ExitVictory();
	void ExitShowScores();

	void UpdateAttract();
	void UpdateLobby();
	void UpdatePlaying();
	void UpdateShowScores();
	void UpdateVictory();

	void RenderAttract();
	void RenderLobby();
	void RenderPlaying();
	void RenderShowScores();
	void RenderVictory();

	GameState m_currentState = GameState::ATTRACT;

	void AddVertsForWorldAxis();

	int GetIndexForCoords(IntVec2 coords);

	void DetectTwoHandsOverlapWithActivatedCubes();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	SoundPlaybackID m_gameMusic;
	SoundPlaybackID m_scoresMusic;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// instruction and count down start UI is drawn in the HUD class
	// scores will be drawn and render in the scene for not interfere with game play
	void AddVertsForCurrentScores();
	void AddVertsForFinalScores();

	int m_currentScores = 0;
	int m_activatedCubesCounter = 0;
	
	Timer* m_grabToStartTimer = nullptr;
	float m_skyStartToChangeTIme = 0.f;

	// default beat
	void ManageDefaultBeats();
	Timer* m_defaultBeatsTimer = nullptr;
	float m_defaultBeatInterval = 0.696f;

	Vec3 m_cubesCenter = Vec3();
	Vec3 m_finalScoresCenter = Vec3();
	Vec3 m_LBOrigin = Vec3();
	Vec3 m_scoresByCubePos;
	float m_cubeSpacing = 0.12f;

	RhythmCube m_cubes[NUM_CUBES];
	std::vector<Pillar*> m_pillars;

	std::vector<Vertex_PCU> m_cubeVerts;
	std::vector<Vertex_PCU> m_pillarVerts;
	std::vector<Vertex_PCU> m_scoreVerts;
	std::vector<Vertex_PCU> m_leftHandVerts;
	std::vector<Vertex_PCU> m_rightHandVerts;

	PlayerHand m_hands[2];

	XrPosef m_cubesLocalSpacePose;
};