#include "Engine/Math/Vec3.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Game/Game.hpp"
#include "Game/RhythmCube.hpp"
#include "Game/graphicsplugin_d3d11.hpp"
#include "Game/d3d_common.h"
#include "Game/HUD.hpp"

Clock* g_theGameClock = nullptr;
RandomNumberGenerator* g_rng = nullptr;
HUD* g_theHUD = nullptr;

extern BitmapFont* g_consoleFont;
extern XRRenderer* g_theXRRenderer;
extern AudioSystem* g_theAudio;

using namespace DirectX;

constexpr float ONE_NIGHT_FIVE_BEATS = (1.f / (95.f * 0.696f));
constexpr float ONE_ONE_THIRTYFIVE_BEATS = (1.f / (135.f * 0.696f));
constexpr float ONE_TWO_ZEROFIVE_BEATS = (1.f / (205.f * 0.696f));
constexpr float ONE_TWO_ONEONE_BEATS = (1.f / (211.f * 0.696f));

Game::Game()
{

}

Game::~Game()
{
	for (auto pillar : m_pillars)
	{
		delete pillar;
		pillar = nullptr;
	}

	if (m_grabToStartTimer)
	{
		delete m_grabToStartTimer;
		m_grabToStartTimer = nullptr;
	}

	if (m_defaultBeatsTimer)
	{
		delete m_defaultBeatsTimer;
		m_defaultBeatsTimer = nullptr;
	}
}

void Game::Startup()
{
	LoadAudioAssets();

	g_rng = new RandomNumberGenerator();

	g_theGameClock = new Clock();
	// set the timer according to the beats
	m_defaultBeatsTimer = new Timer(m_defaultBeatInterval, g_theGameClock);
	m_defaultBeatsTimer->Start();

	// calculate the center of all cubes and the left bottom corner pos
	float oneAndHalf_CubeSize_spacing = (CUBE_SIZE + m_cubeSpacing) * 1.5f;
	m_cubesCenter = Vec3(0.f, DIST_PLAYER_TO_CUBES_PLAYING, (HEIGHT_CUBES_TO_GROUND + oneAndHalf_CubeSize_spacing));
	m_LBOrigin = m_cubesCenter - Vec3(oneAndHalf_CubeSize_spacing, 0.f, oneAndHalf_CubeSize_spacing);
	m_scoresByCubePos = m_LBOrigin + Vec3((m_cubeSpacing + CUBE_SIZE) * -1.75f, 0.f, 0.f);
	Vec3 LBOrigin_back = m_cubesCenter - Vec3(oneAndHalf_CubeSize_spacing, 0.f, oneAndHalf_CubeSize_spacing) + Vec3(0.f, DIST_PLAYER_TO_CUBES_SCORES, 0.f);
	m_finalScoresCenter = m_cubesCenter+ Vec3(0.f, DIST_PLAYER_TO_CUBES_SCORES * 0.5f, 0.f);

	// initialize the position of each cube
	for (int x = 0; x < NUM_SIDE_X; ++x)
	{
		for (int y = 0; y < NUM_SIDE_Y; ++y)
		{
			int index = GetIndexForCoords(IntVec2(x, y));
			RhythmCube& cube = m_cubes[index];
			Vec3 pos = m_LBOrigin + Vec3(x * (m_cubeSpacing + CUBE_SIZE), 0.f, y * (m_cubeSpacing + CUBE_SIZE));
			cube.m_originPos = pos;
			Vec3 backPos = LBOrigin_back + Vec3(x * (m_cubeSpacing + CUBE_SIZE), 0.f, y * (m_cubeSpacing + CUBE_SIZE));
			cube.m_posWhenShowingScores = backPos;
		}
	}

	// initialize the position of the pillars
	for (int x = int(float(NUM_PILLAR_SIDE_X) * -0.5f); x < int(float(NUM_PILLAR_SIDE_X) * 0.5f); ++x)
	{
		for (int y = int(float(NUM_PILLAR_SIDE_Y) * -0.5f); y < int(float(NUM_PILLAR_SIDE_Y) * 0.5f); ++y)
		{
			IntVec2 coords(x, y);
			Vec3 pillarPos;
			pillarPos.x = float(coords.x);
			pillarPos.y = float(coords.y);
			pillarPos.z = 0.f;
			float dist = GetDistance3D(pillarPos, Vec3());
			float timeOffset = RangeMapClamped(dist, 0.f, (float(NUM_PILLAR_SIDE_Y) * 0.5f), 0.f, PILLAR_TIME_OFFSET_AMOUNT);
			Pillar* pillar = new Pillar(coords, timeOffset);
			m_pillars.push_back(pillar);
		}
	}

	m_hands[Side::LEFT].m_handIndex = Side::LEFT;
	m_hands[Side::RIGHT].m_handIndex = Side::RIGHT;

	g_theHUD = new HUD();

	m_grabToStartTimer = new Timer(DURATION_GRAB_TO_START);
}

void Game::Update()
{
	m_cubeVerts.clear();

	switch (m_currentState)
	{
	case GameState::ATTRACT:
	{
		UpdateAttract();
	}break;
	case GameState::LOBBY:
	{
		UpdateLobby();
	}break;
	case GameState::PLAYING:
	{
		UpdatePlaying();
	}break;
	case GameState::SHOWSCORES:
	{
		UpdateShowScores();
	}break;
	}

	// whatever the state current is, the world and hands axis will be rendered
	for (int i = 0; i < 2; ++i)
	{
		m_hands[i].Update();
	}
	AddVertsForWorldAxis();
}

void Game::SetModelMatrix()
{
	// use the reference space located info to pass info to the shader
	DirectX::XMFLOAT4X4 modelMatrix;
	XMStoreFloat4x4(&modelMatrix, XMMatrixTranspose(LoadXrPose(m_cubesLocalSpacePose)));

	// Mat44 modelMat(m_cubesLocalSpacePose);
	g_theXRRenderer->SetModelConstants(modelMatrix);
}

void Game::Render()
{
	switch (m_currentState)
	{
	case GameState::ATTRACT:
	{
		RenderAttract();
	}break;
	case GameState::LOBBY:
	{
		RenderLobby();
	}break;
	case GameState::PLAYING:
	{
		RenderPlaying();
	}break;
	case GameState::SHOWSCORES:
	{
		RenderShowScores();
	}break;
	}

	// always render hands	
	g_theXRRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
	g_theXRRenderer->BindTexture(nullptr);
	if (!m_leftHandVerts.empty())
	{
		m_hands[Side::LEFT].SetModelMatrix();
		g_theXRRenderer->DrawVertexArray((int)(m_leftHandVerts.size()), m_leftHandVerts.data());
	}
	if (!m_rightHandVerts.empty())
	{
		m_hands[Side::RIGHT].SetModelMatrix();
		g_theXRRenderer->DrawVertexArray((int)(m_rightHandVerts.size()), m_rightHandVerts.data());
	}
}

void Game::DetectTwoHandsOverlapWithActivatedCubes()
{
	// // if there is any hand overlap with activated cubes, deactivate the cube and set the hand is overlap
	// XMMATRIX modelMatrix = XMMatrixTranspose(LoadXrPose(m_refSpacePose));
	// 
	// for (int i = 0; i < NUM_CUBES; ++i)
	// {
	// 	if (m_cubes[i].m_status == CubeStatus::ACTIVATED)
	// 	{
	// 		for (int handIndex = 0; handIndex < Side::COUNT; ++handIndex)
	// 		{
	// 			// convert all pos into the m_appSpace for collision detection
	// 
	// 			Vec3 cubeLocalPos = Vec3(m_cubes[i].m_pose.position);
	// 			XMMATRIX localSpaceXMMat = XMMatrixTranslation(cubeLocalPos.x, cubeLocalPos.y, cubeLocalPos.z);
	// 			XMMATRIX worldSpaceXMMat = localSpaceXMMat * modelMatrix;
	// 			// Decompose the matrix to extract the translation component
	// 			XMVECTOR scale, rotation, translation;
	// 			DirectX::XMMatrixDecompose(&scale, &rotation, &translation, worldSpaceXMMat);
	// 
	// 			// Store the translation components in an XMFLOAT3
	// 			XMFLOAT3 translationComponents;
	// 			XMStoreFloat3(&translationComponents, translation);
	// 
	// 			Vec3 cubeWorldPos;
	// 			cubeWorldPos.x = translationComponents.x;
	// 			cubeWorldPos.y = translationComponents.y;
	// 			cubeWorldPos.z = translationComponents.z;
	// 
	// 			if (DoSpheresOverlap3D(m_hands[handIndex].m_handPos, HAND_SPHERE_RADIUS, cubeWorldPos, CUBE_SIZE))
	// 			{
	// 				m_cubes[i].m_playerTouched = true;
	// 				m_hands[handIndex].m_overlapWithCube = true;
	// 			}
	// 		}
	// 	}
	// }

	// if there is any hand overlap with activated cubes, deactivate the cube and set the hand is overlap
	for (int i = 0; i < NUM_CUBES; ++i)
	{
		if (m_cubes[i].m_status == CubeStatus::ACTIVATED)
		{
			for (int handIndex = 0; handIndex < Side::COUNT; ++handIndex)
			{
				if (m_hands[handIndex].m_isActive && !m_cubes[i].m_playerTouched)
				{
					// convert all pos into the m_appSpace for collision detection
					Vec3 cubeLocalPos = Vec3(m_cubes[i].m_localposInCubeSpace);
					Mat44 localToWorldMat(m_cubesLocalSpacePose);
					Vec3 cubeWorldPos = localToWorldMat.TransformPosition3D(cubeLocalPos);

					if (DoSpheresOverlap3D(m_hands[handIndex].m_handPos, HAND_SPHERE_RADIUS, cubeWorldPos, (CUBE_SIZE * 0.5f)))
					{
						m_cubes[i].m_playerTouched = true;
						m_hands[handIndex].m_overlapWithCube = true;

						++m_currentScores;
					}
				}
			}
		}
	}

	// if there is no overlap for the hand, set the hand overlap is false
	for (int handIndex = 0; handIndex < Side::COUNT; ++handIndex)
	{
		bool overlap = false;

		for (int i = 0; i < NUM_CUBES; ++i)
		{
			if (m_cubes[i].m_status == CubeStatus::ACTIVATED)
			{
				// convert all pos into the m_appSpace for collision detection
				Vec3 cubeLocalPos = Vec3(m_cubes[i].m_localposInCubeSpace);
				Mat44 localToWorldMat(m_cubesLocalSpacePose);
				Vec3 cubeWorldPos = localToWorldMat.TransformPosition3D(cubeLocalPos);

				if (DoSpheresOverlap3D(m_hands[handIndex].m_handPos, HAND_SPHERE_RADIUS, cubeWorldPos, (CUBE_SIZE * 0.5f)))
				{
					overlap = true;
				}
			}
		}

		if (!overlap)
		{
			m_hands[handIndex].m_overlapWithCube = false;
		}
	}
}

void Game::AddVertsForCurrentScores()
{
	m_scoreVerts.clear();
	std::string scores = Stringf("%i", m_beatsCounter);
	g_consoleFont->AddVertsForText3DAtOriginXForward(m_scoreVerts, TEXT_HEIGHT_LARGE, scores, Rgba8::PURPLE_BLUE, FONT_ASPECT);

	// yaw the billboard 180 degrees to face the player
	Mat44 rotationMat_3 = Mat44::CreateZRotationDegrees(-90.f);
	Mat44 translationMat = Mat44::CreateTranslation3D(m_scoresByCubePos);
	Mat44 transformMat = translationMat.MatMultiply(rotationMat_3);
		
	TransformVertexArray3D(m_scoreVerts, transformMat);
}

void Game::AddVertsForFinalScores()
{
	m_scoreVerts.clear();
	std::string accuracy = Stringf("Accuracy", int(float(m_currentScores / m_activatedCubesCounter) * 100.f));
	g_consoleFont->AddVertsForText3DAtOriginXForward(m_scoreVerts, TEXT_HEIGHT_LARGE, accuracy, Rgba8::PASTEL_BLUE, FONT_ASPECT, Vec2(0.5f, -0.5f));

	int accuracyResult = int((float(m_currentScores) / float(m_activatedCubesCounter) * 100.f));
	std::string scores = Stringf("%i %%", accuracyResult);
	g_consoleFont->AddVertsForText3DAtOriginXForward(m_scoreVerts, TEXT_HEIGHT_LARGE, scores, Rgba8::PASTEL_BLUE, FONT_ASPECT);

	// yaw the billboard 180 degrees to face the player
	Mat44 rotationMat_3 = Mat44::CreateZRotationDegrees(-90.f);
	Mat44 translationMat = Mat44::CreateTranslation3D(m_finalScoresCenter);
	Mat44 transformMat = translationMat.MatMultiply(rotationMat_3);

	TransformVertexArray3D(m_scoreVerts, transformMat);
}

void Game::ActivateRythmCubeByBeats()
{
	if (m_beatsCounter < 68) // single beat
	{
		m_activationDuration = m_defaultBeatInterval * 2.f;

		if (m_selectToActivateTimer_singleHit->HasPeroidElapsed())
		{
			RandomSelectCubesToActivate(1, m_activationDuration);
			m_selectToActivateTimer_singleHit->Restart();
		}

		if (m_beatsCounter == 67)
		{
			m_skyStartToChangeTIme = g_theGameClock->GetTotalSeconds();
		}
	}
	else if (m_beatsCounter < 95) // double beat
	{
		m_activationDuration = m_defaultBeatInterval * 2.f;

		if (m_selectToActivateTimer_doubleHit->HasPeroidElapsed())
		{
			RandomSelectCubesToActivate(2, m_activationDuration);

			m_selectToActivateTimer_doubleHit->Restart();
		}

		// slowly converting the clean screen color to deep blue
		Rgba8 color = InterpolateRGBA(Rgba8::BLACK, Rgba8::BLUSH_PINK, ((g_theGameClock->GetTotalSeconds() - m_skyStartToChangeTIme) * ONE_TWO_ZEROFIVE_BEATS));
		g_theXRRenderer->SetClearScreenColor(color);
	}
	else if (m_beatsCounter >= 97 && m_beatsCounter < 101) // all beat
	{
		ActivateAllCubesByOrder(m_defaultBeatInterval * 4.f);
		Rgba8 color = InterpolateRGBA(Rgba8::BLACK, Rgba8::BLUSH_PINK, ((g_theGameClock->GetTotalSeconds() - m_skyStartToChangeTIme) * ONE_TWO_ZEROFIVE_BEATS));
		g_theXRRenderer->SetClearScreenColor(color);
	}
	else if (m_beatsCounter >= 103 && m_beatsCounter < 135) // double beat
	{
		m_activationDuration = m_defaultBeatInterval * 2.f;

		if (m_selectToActivateTimer_doubleHit->HasPeroidElapsed())
		{
			RandomSelectCubesToActivate(2, m_activationDuration);

			m_selectToActivateTimer_doubleHit->Restart();
		}
		Rgba8 color = InterpolateRGBA(Rgba8::BLACK, Rgba8::BLUSH_PINK, ((g_theGameClock->GetTotalSeconds() - m_skyStartToChangeTIme) * ONE_TWO_ZEROFIVE_BEATS));
		g_theXRRenderer->SetClearScreenColor(color);
	}
	else if (m_beatsCounter >= 135 && m_beatsCounter < 205) // single beat
	{
		m_activationDuration = m_defaultBeatInterval * 2.f;

		if (m_selectToActivateTimer_doubleHit->HasPeroidElapsed())
		{
			RandomSelectCubesToActivate(1, m_activationDuration);

			m_selectToActivateTimer_doubleHit->Restart();
		}
		Rgba8 color = InterpolateRGBA(Rgba8::BLACK, Rgba8::BLUSH_PINK, ((g_theGameClock->GetTotalSeconds() - m_skyStartToChangeTIme) * ONE_TWO_ZEROFIVE_BEATS));
		g_theXRRenderer->SetClearScreenColor(color);

	}
	else if (m_beatsCounter >= 205 && m_beatsCounter < 211) // decrease volume to end
	{
		if (m_songDecreaseVolumeTimer->IsStopped())
		{
			m_songDecreaseVolumeTimer->Start();
		}
		else
		{
			float volumeFraction = 1.f - GetClamped(m_songDecreaseVolumeTimer->GetElapsedFraction(), 0.f, 1.f);
			g_theAudio->SetSoundPlaybackVolume(m_gameMusic, volumeFraction);
		}

		Rgba8 color = InterpolateRGBA(Rgba8::BLUSH_PINK, Rgba8::SALMON_PINK, ((g_theGameClock->GetTotalSeconds() - m_skyStartToChangeTIme) * ONE_TWO_ONEONE_BEATS));
		g_theXRRenderer->SetClearScreenColor(color);
	}
	else if (m_beatsCounter >= 211)
	{
		ExitState(m_currentState);
		m_currentState = GameState::SHOWSCORES;
		EnterState(m_currentState);
	}
}

void Game::RandomSelectCubesToActivate(int numCubes, float duration)
{
	for (int i = 0; i < numCubes; ++i)
	{
		int cubeIndex = g_rng->RollRandomIntInRange(0, NUM_CUBES-1);
		m_cubes[cubeIndex].ActivateCubeToBeHit(duration);
		++m_activatedCubesCounter;
	}
}

void Game::ActivateAllCubesByOrder(float duration)
{
	int hitSequence[16] = { 15, 12, 14, 13, 10, 9, 11, 8, 7, 4, 6, 5, 2, 1, 3, 0 };

	if (!m_selectToActivateTimer_allHit)
	{
		float eachDuration = duration / 16;
		m_selectToActivateTimer_allHit = new Timer(eachDuration, g_theGameClock);
		m_selectToActivateTimer_allHit->Start();

		m_allHitindex = 0;
		m_cubes[hitSequence[m_allHitindex]].ActivateCubeToBeHit(duration);
	}
	else
	{
		if (m_selectToActivateTimer_allHit->HasPeroidElapsed() && m_allHitindex <= 14)
		{
			++m_allHitindex;
			m_cubes[hitSequence[m_allHitindex]].ActivateCubeToBeHit(duration);
			m_selectToActivateTimer_allHit->Restart();
		}
		else
		{
			return;
		}
	}
}

void Game::UpdateReferenceWorldSpacePoseForCubes(XrPosef pose)
{
	m_cubesLocalSpacePose = pose;
}

void Game::UpdateReferenceHandSpacePoses(XrPosef handPose, int handIndex)
{
	if (handIndex == Side::LEFT)
	{
		m_hands[0].m_actionSpacePose = handPose;
	}
	else if (handIndex == Side::RIGHT)
	{
		m_hands[1].m_actionSpacePose = handPose;
	}
}

void Game::LoadAudioAssets()
{
	m_soundEffectsID[int(SoundEffectID::GAMEMUSIC)] = g_theAudio->CreateOrGetSound("Data/Audio/Signals_LazerBoomerang.mp3");
	m_soundEffectsID[int(SoundEffectID::SCORESMUSIC)] = g_theAudio->CreateOrGetSound("Data/Audio/SClass_ZZZ.mp3");
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// State machine
void Game::EnterState(GameState state)
{
	// start new music
	switch (state)
	{
	case GameState::ATTRACT: // Draw the game start instruction on HUD
	{
		EnterAttract();
	}break;
	case GameState::LOBBY: // draw count down on the HUD
	{
		EnterLobby();
	}break;
	case GameState::PLAYING:
	{
		EnterPlaying();
	}break;	
	case GameState::SHOWSCORES: // show the final score and restart instruction
	{
		EnterShowScores();
	}break;
	}
}

void Game::ExitState(GameState state)
{
	// start new music
	switch (state)
	{
	case GameState::ATTRACT: // Draw the game start instruction on HUD
	{
		ExitAttract();
	}break;
	case GameState::LOBBY: // draw count down on the HUD
	{
		ExitLobby();
	}break;
	case GameState::PLAYING:
	{
		ExitPlaying();
	}break;
	case GameState::SHOWSCORES: // show the final score and restart instruction
	{
		ExitShowScores();
	}break;
	}
}

void Game::EnterAttract()
{
	m_currentScores = 0;
	m_beatsCounter = 0;
	m_activatedCubesCounter = 0;

	g_theXRRenderer->SetClearScreenColor(Rgba8::BLACK);
}

void Game::EnterLobby()
{
	m_gameMusic = g_theAudio->StartSound(m_soundEffectsID[int(SoundEffectID::GAMEMUSIC)], false, 1.f);
}

void Game::EnterPlaying()
{
	if (m_selectToActivateTimer_singleHit)
	{
		delete m_selectToActivateTimer_singleHit;
		m_selectToActivateTimer_singleHit = nullptr;
	}
	m_selectToActivateTimer_singleHit = new Timer(m_defaultBeatInterval, g_theGameClock);
	m_selectToActivateTimer_singleHit->Start();

	if (m_selectToActivateTimer_doubleHit)
	{
		delete m_selectToActivateTimer_doubleHit;
		m_selectToActivateTimer_doubleHit = nullptr;
	}
	m_selectToActivateTimer_doubleHit = new Timer(m_defaultBeatInterval, g_theGameClock);
	m_selectToActivateTimer_doubleHit->Start();

	if (m_songDecreaseVolumeTimer)
	{
		delete m_songDecreaseVolumeTimer;
		m_songDecreaseVolumeTimer = nullptr;
	}
	m_songDecreaseVolumeTimer = new Timer(DURATION_SONG_VOLUME_DECREASE_TO_END * m_defaultBeatInterval, g_theGameClock);
}

void Game::EnterShowScores()
{
	m_scoresMusic = g_theAudio->StartSound(m_soundEffectsID[int(SoundEffectID::SCORESMUSIC)], false, 1.f);
	g_theXRRenderer->SetClearScreenColor(Rgba8::BLACK);
}

void Game::ExitAttract()
{

}

void Game::ExitLobby()
{

}

void Game::ExitPlaying()
{
	g_theAudio->SetSoundPlaybackSpeed(m_gameMusic, 0.f);
}

void Game::ExitShowScores()
{
	g_theAudio->SetSoundPlaybackSpeed(m_scoresMusic, 0.f);
}

void Game::UpdateAttract()
{
	// both hands to need to grab and hold over 50% for a period of time to start the game
	if (m_hands[Side::LEFT].m_grabValue < 0.5f || m_hands[Side::LEFT].m_grabValue < 0.5f)
	{
		m_grabToStartTimer->Restart();
	}
	else
	{
		if (m_grabToStartTimer->IsStopped())
		{
			m_grabToStartTimer->Start();
		}
		else
		{
			if (m_grabToStartTimer->HasPeroidElapsed())
			{
				m_grabToStartTimer->Stop();

				m_currentState = GameState::LOBBY;
				ExitState(GameState::ATTRACT);
				EnterState(GameState::LOBBY);
			}
		}
	}

	AddVertsForWorldAxis();
}

void Game::UpdateLobby()
{
	ManageDefaultBeats();

	for (int cubeIndex = 0; cubeIndex < NUM_CUBES; ++cubeIndex)
	{
		m_cubes[cubeIndex].Update();
	}

	for (auto pillar : m_pillars)
	{
		pillar->Update();
	}
}

void Game::UpdatePlaying()
{
	AddVertsForCurrentScores();

	ManageDefaultBeats();
	ActivateRythmCubeByBeats();

	for (int cubeIndex = 0; cubeIndex < NUM_CUBES; ++cubeIndex)
	{
		m_cubes[cubeIndex].Update();
	}

	for (auto pillar : m_pillars)
	{
		pillar->Update();
	}

	DetectTwoHandsOverlapWithActivatedCubes();
}

void Game::UpdateShowScores()
{
	ManageDefaultBeats();

	for (int cubeIndex = 0; cubeIndex < NUM_CUBES; ++cubeIndex)
	{
		m_cubes[cubeIndex].Update();
	}

	AddVertsForFinalScores();

	// if player grab both hands to start again, we'll renter the playing state
	if (m_hands[Side::LEFT].m_grabValue < 0.5f || m_hands[Side::LEFT].m_grabValue < 0.5f)
	{
		m_grabToStartTimer->Restart();
	}
	else
	{
		if (m_grabToStartTimer->IsStopped())
		{
			m_grabToStartTimer->Start();
		}
		else
		{
			if (m_grabToStartTimer->HasPeroidElapsed())
			{
				m_grabToStartTimer->Stop();

				ExitState(GameState::SHOWSCORES);
				m_currentState = GameState::ATTRACT;
				EnterState(GameState::ATTRACT);
			}
		}
	}
}

void Game::RenderAttract()
{
	g_theHUD->RenderInstructionToStartGame();

	SetModelMatrix();
	g_theXRRenderer->BindTexture(nullptr);
	g_theXRRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theXRRenderer->DrawVertexArray((int)(m_cubeVerts.size()), m_cubeVerts.data());

}

void Game::RenderLobby()
{
	g_theHUD->RenderCountDownToStartTheSong();

	SetModelMatrix();
	g_theXRRenderer->BindTexture(nullptr);
	g_theXRRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theXRRenderer->DrawVertexArray((int)(m_cubeVerts.size()), m_cubeVerts.data());
}

void Game::RenderPlaying()
{
	SetModelMatrix();
	g_theXRRenderer->BindTexture(nullptr);
	g_theXRRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theXRRenderer->DrawVertexArray((int)(m_cubeVerts.size()), m_cubeVerts.data());

	g_theXRRenderer->BindTexture(&g_consoleFont->GetTexture());
	g_theXRRenderer->DrawVertexArray((int)m_scoreVerts.size(), m_scoreVerts.data());
}

void Game::RenderShowScores()
{
	SetModelMatrix();
	g_theXRRenderer->BindTexture(nullptr);
	g_theXRRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theXRRenderer->DrawVertexArray((int)(m_cubeVerts.size()), m_cubeVerts.data());

	g_theXRRenderer->BindTexture(&g_consoleFont->GetTexture());
	g_theXRRenderer->DrawVertexArray((int)m_scoreVerts.size(), m_scoreVerts.data());

}

void Game::AddVertsForWorldAxis()
{
	float spacingXYZ = 1.f;
	int numOfXYZ = (int)(100.f / spacingXYZ) + 1;
	int numOfGrid = 100 + 1;
	float dimensionRed = 0.025f;
	float dimensionGreen = 0.02f;
	float dimensionGray = 0.01f;
	// // gray grid
	// for (int i = 0; i < numOfGrid; ++i)
	// {
	// 	AABB3 pipe(Vec3(-50.f + (float)i - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
	// 		Vec3(-50.f + (float)i + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
	// 	AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	// }
	// for (int i = 0; i < numOfGrid; ++i)
	// {
	// 	AABB3 pipe(Vec3(-50.f, -50.f + (float)i - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
	// 		Vec3(50.f, -50.f + (float)i + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
	// 	AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	// }
	// // GREEN lane
	// for (int i = 0; i < numOfXYZ; ++i)
	// {
	// 	if (i == (numOfXYZ / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f + (float)i * spacingXYZ - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
	// 			Vec3(-50.f + (float)i * spacingXYZ + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f + (float)i * spacingXYZ - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
	// 			Vec3(-50.f + (float)i * spacingXYZ + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GREEN, AABB2::ZERO_TO_ONE);
	// 	}
	// }
	// // RED lane
	// for (int i = 0; i < numOfXYZ; ++i)
	// {
	// 	if (i == (numOfXYZ / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// }
	// // BLUE lane
	// for (int i = 0; i < numOfXYZ; ++i)
	// {
	// 	if (i == (numOfXYZ / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// }

	// gray grid
	for (int i = 0; i < numOfGrid; ++i)
	{
		AABB3 pipe(Vec3(-50.f + (float)i - (dimensionGray * 0.5f), -50.f, -(dimensionGray * 0.5f)),
			Vec3(-50.f + (float)i + (dimensionGray * 0.5f), 50.f, (dimensionGray * 0.5f)));
		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	}
	for (int i = 0; i < numOfGrid; ++i)
	{
		AABB3 pipe(Vec3(-50.f, -50.f + (float)i - (dimensionGray * 0.5f), -(dimensionGray * 0.5f)),
			Vec3(50.f, -50.f + (float)i + (dimensionGray * 0.5f), (dimensionGray * 0.5f)));
		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::GRAY, AABB2::ZERO_TO_ONE);
	}
	// GREEN lane
	for (int i = 0; i < numOfXYZ; ++i)
	{
		if (i == (numOfXYZ / 2))
		{
			AABB3 pipe(Vec3(-50.f + (float)i * spacingXYZ - (dimensionGreen * 1.2f), -50.f, -(dimensionGreen * 2.f)),
				Vec3(-50.f + (float)i * spacingXYZ + (dimensionGreen * 2.f), 50.f, (dimensionGreen * 2.f)));
			AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::NEON_BLUE, AABB2::ZERO_TO_ONE);
		}
		else
		{
			AABB3 pipe(Vec3(-50.f + (float)i * spacingXYZ - (dimensionGreen * 0.5f), -50.f, -(dimensionGreen * 0.5f)),
				Vec3(-50.f + (float)i * spacingXYZ + (dimensionGreen * 0.5f), 50.f, (dimensionGreen * 0.5f)));
			AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::NEON_BLUE, AABB2::ZERO_TO_ONE);
		}
	}
	// RED lane
	for (int i = 0; i < numOfXYZ; ++i)
	{
		if (i == (numOfXYZ / 2))
		{
			AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
				Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 2.f), (dimensionRed * 2.f)));
			AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::NEON_PINK, AABB2::ZERO_TO_ONE);
		}
		else
		{
			AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
				Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
			AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::NEON_PINK, AABB2::ZERO_TO_ONE);
		}
	}
	// BLUE lane
	// for (int i = 0; i < numOfXYZ; ++i)
	// {
	// 	if (i == (numOfXYZ / 2))
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 1.2f), -(dimensionRed * 2.f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 2.f), (dimensionRed * 2.f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// 	else
	// 	{
	// 		AABB3 pipe(Vec3(-50.f, -50.f + (float)i * spacingXYZ - (dimensionRed * 0.5f), -(dimensionRed * 0.5f)),
	// 			Vec3(50.f, -50.f + (float)i * spacingXYZ + (dimensionRed * 0.5f), (dimensionRed * 0.5f)));
	// 		AddVertsForAABB3D(m_cubeVerts, pipe, Rgba8::RED, AABB2::ZERO_TO_ONE);
	// 	}
	// }

	// AddVertsForArrow3D(m_cubeVerts, Vec3(), Vec3(1.f, 0.f, 0.f), 0.1f, Rgba8::RED, Rgba8::RED);
	// AddVertsForArrow3D(m_cubeVerts, Vec3(), Vec3(0.f, 1.f, 0.f), 0.1f, Rgba8::GREEN, Rgba8::GREEN);
	// AddVertsForArrow3D(m_cubeVerts, Vec3(), Vec3(0.f, 0.f, 1.f), 0.1f, Rgba8::BLUE, Rgba8::BLUE);
}


int Game::GetIndexForCoords(IntVec2 coords)
{
	return (coords.y * NUM_SIDE_X + coords.x);
}

void Game::ManageDefaultBeats()
{
	// set the timer according to the beats
	if (!m_defaultBeatsTimer)
	{
		m_defaultBeatsTimer = new Timer(m_defaultBeatInterval, g_theGameClock);
	}
	else
	{
		// if the timer is not rewinding, and reachs the period, starts to rewind
		if (!m_defaultBeatsTimer->IsRewinding())
		{
			if (m_defaultBeatsTimer->HasPeroidElapsed())
			{
				m_defaultBeatsTimer->Rewind();
				++m_beatsCounter;
			}
		}
		else
		{
			// if the timer is rewinding to the start, starts forward
			if (m_defaultBeatsTimer->HasTimerRewindToStart())
			{
				m_defaultBeatsTimer->RestartAndGoForward();
				++m_beatsCounter;
			}
		}
	}
}

