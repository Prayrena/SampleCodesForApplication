#include "Game/HUD.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Game/graphicsplugin_d3d11.hpp"
#include "Game/Game.hpp"

extern BitmapFont* g_consoleFont;
extern XRRenderer* g_theXRRenderer;
extern Game* g_theGame;
extern Clock* g_theGameClock;

void HUD::UpdateHUDViewSpacePose(XrPosef updatePose)
{
	m_HUDViewSpacePose = updatePose;
}

void HUD::SetModelMatrix()
{
	// use the reference space located info to pass info to the shader
	// DirectX::XMFLOAT4X4 modelMatrix;
	// XMStoreFloat4x4(&modelMatrix, XMMatrixTranspose(LoadXrPose(m_HUDViewSpacePose)));
	// g_theXRRenderer->SetModelConstants(modelMatrix);

	Mat44 renderMat(Vec3(0.f, 0.f, -1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3());
	renderMat.Transpose();
	XrMatrix4x4f renderXrMatrix = renderMat.GetXrMatByMat();

	XMMATRIX renderXMMat = LoadXrMatrix(renderXrMatrix);
	XMMATRIX refSpaceXMMat = LoadXrPose(m_HUDViewSpacePose);

	refSpaceXMMat = refSpaceXMMat * renderXMMat;

	DirectX::XMFLOAT4X4 modelMatrix;
	XMStoreFloat4x4(&modelMatrix, XMMatrixTranspose(refSpaceXMMat));
	g_theXRRenderer->SetModelConstants(modelMatrix);
}

void HUD::RenderInstructionToStartGame()
{
	std::vector<Vertex_PCU> textVerts;
	std::string text = "Grab and squeeze both controllers to start the game";
	g_consoleFont->AddVertsForText3DAtOriginXForward(textVerts, TEXT_HEIGHT_DEFAULT, text, Rgba8::LIGHT_ORANGE, FONT_ASPECT);
	
	// yaw the billboard 180 degrees to face the player
	Mat44 rotationMat_1 = Mat44::CreateXRotationDegrees(-90.f);
	Mat44 rotationMat_2 = Mat44::CreateYRotationDegrees(-90.f);
	Mat44 translationMat = Mat44::CreateTranslation3D(m_displayPos);
	Mat44 transformMat = translationMat.MatMultiply(rotationMat_2.MatMultiply(rotationMat_1));
	
	SetModelMatrix();
	g_theXRRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theXRRenderer->BindTexture(&g_consoleFont->GetTexture());

	// Vec3 cubeHalfSize = Vec3(1.f, 1.f, 1.f) * 0.5f;
	// AABB3 bounds(cubeHalfSize * -1.f, cubeHalfSize);
	// AddVertsForAABB3D(textVerts, bounds, Rgba8::RED, Rgba8::RED, Rgba8::GREEN, Rgba8::GREEN, Rgba8::BLUE, Rgba8::BLUE);

	TransformVertexArray3D(textVerts, transformMat);

	g_theXRRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
}

void HUD::RenderCountDownToStartTheSong()
{
	if (!m_countDownStarts)
	{
		m_countDownTimer = new Timer(g_theGame->m_defaultBeatInterval, g_theGameClock);
		m_countDownTimer->Start();
		m_countDownStarts = true;
		m_timesCountingDown = 0;
	}
	else
	{
		if (m_countDownTimer->HasPeroidElapsed())
		{
			if (m_timesCountingDown == 4)
			{
				m_countDownStarts = false;
				g_theGame->m_currentState = GameState::PLAYING;
				g_theGame->EnterState(GameState::PLAYING);
				m_countDownStarts = false;
				return;
			}
			else
			{
				++m_timesCountingDown;
				m_countDownTimer->Restart();
			}
		}

		// show the the 3, 2, 1 on screen
		std::vector<Vertex_PCU> textVerts;
		if ((3 - m_timesCountingDown) > 0)
		{
			std::string text = Stringf("%i", (3 - m_timesCountingDown));
			g_consoleFont->AddVertsForText3DAtOriginXForward(textVerts, TEXT_HEIGHT_LARGE, text, Rgba8::BLUSH_PINK, FONT_ASPECT);
		}
		else
		{
			std::string text = "GO !!!";
			g_consoleFont->AddVertsForText3DAtOriginXForward(textVerts, TEXT_HEIGHT_LARGE, text, Rgba8::NEON_BLUE, FONT_ASPECT);
		}

		// yaw the billboard 180 degrees to face the player
		Mat44 rotationMat_1 = Mat44::CreateXRotationDegrees(-90.f);
		Mat44 rotationMat_2 = Mat44::CreateYRotationDegrees(-90.f);
		Mat44 translationMat = Mat44::CreateTranslation3D(m_displayPos);
		// Mat44 transformMat = translationMat.MatMultiply(rotationMat_3.MatMultiply(rotationMat_2.MatMultiply(rotationMat_1)));
		Mat44 transformMat = translationMat.MatMultiply(rotationMat_2.MatMultiply(rotationMat_1));

		SetModelMatrix();
		g_theXRRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		g_theXRRenderer->BindTexture(&g_consoleFont->GetTexture());

		TransformVertexArray3D(textVerts, transformMat);

		g_theXRRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
	}
}

