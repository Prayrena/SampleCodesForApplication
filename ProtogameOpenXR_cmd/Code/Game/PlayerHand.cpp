#include "Game/PlayerHand.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Game/Game.hpp"
#include "Game/graphicsplugin_d3d11.hpp"

extern Game* g_theGame;
extern XRRenderer* g_theXRRenderer;

void PlayerHand::Update()
{
	UpdateHandPosInAppSpace();
	AddVertsForHand();
}

void PlayerHand::UpdateHandPosInAppSpace()
{
	// Mat44 renderMat(Vec3(0.f, 0.f, -1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3());
	// renderMat.Transpose();
	// XrMatrix4x4f renderXrMatrix = renderMat.GetXrMatByMat();
	// 
	// XMMATRIX renderXMMat = LoadXrMatrix(renderXrMatrix);
	// XMMATRIX refSpaceXMMat = LoadXrPose(m_actionSpacePose);
	// 
	// XMMATRIX modelXMMat = XMMatrixTranspose(refSpaceXMMat * renderXMMat);
	// XMMATRIX worldSpaceXMMat = XMMatrixIdentity() * modelXMMat;
	// 
	// // Decompose the matrix to extract the translation component
	// XMVECTOR scale, rotation, translation;
	// DirectX::XMMatrixDecompose(&scale, &rotation, &translation, worldSpaceXMMat);
	// 
	// // Store the translation components in an XMFLOAT3
	// XMFLOAT3 translationComponents;
	// XMStoreFloat3(&translationComponents, translation);
	// 
	// m_handPos.x = translationComponents.x;
	// m_handPos.y = translationComponents.y;
	// m_handPos.z = translationComponents.z;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	Mat44 renderMat(Vec3(0.f, 0.f, -1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3());
	renderMat.Transpose();
	Mat44 actionSpaceMat(m_actionSpacePose);
	// actionSpaceMat.Append(renderMat);
	// renderMat.Append(actionSpaceMat);

m_handPos = (renderMat.MatMultiply(actionSpaceMat)).GetTranslation3D();

	// if (m_handIndex == Side::RIGHT)
	// {
	// 	Log::Write(Log::Level::Info, Fmt("hand %i", m_handIndex));
	// 	Log::Write(Log::Level::Info, Fmt("handPos: X = %.02f,Y = %.02f, Z = %.02f", m_handPos.x, m_handPos.y, m_handPos.z));
	// }
}

void PlayerHand::AddVertsForHand()
{	
	if (m_isActive)
	{
		// add verts by hand index
		if (m_handIndex == Side::LEFT)
		{
			std::vector<Vertex_PCU>& verts = g_theGame->m_leftHandVerts;
			verts.clear();
			if (m_overlapWithCube)
			{
				AddVertsForSphere3D(verts, Vec3(), HAND_SPHERE_RADIUS, Rgba8::YELLOW);
			}
			else
			{
				AddVertsForSphere3D(verts, Vec3(), HAND_SPHERE_RADIUS, Rgba8::PASTEL_BLUE);
			}
		}
		else if (m_handIndex == Side::RIGHT)
		{
			std::vector<Vertex_PCU>& verts = g_theGame->m_rightHandVerts;
			verts.clear();
			if (m_overlapWithCube)
			{
				AddVertsForSphere3D(verts, Vec3(), HAND_SPHERE_RADIUS, Rgba8::YELLOW);
			}
			else
			{
				AddVertsForSphere3D(verts, Vec3(), HAND_SPHERE_RADIUS, Rgba8::PASTEL_BLUE);
			}
		}
	}
}

void PlayerHand::SetModelMatrix()
{
	Mat44 renderMat(Vec3(0.f, 0.f, -1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3());
	renderMat.Transpose();
	XrMatrix4x4f renderXrMatrix = renderMat.GetXrMatByMat();

	XMMATRIX renderXMMat = LoadXrMatrix(renderXrMatrix);
	XMMATRIX refSpaceXMMat = LoadXrPose(m_actionSpacePose);

	refSpaceXMMat = refSpaceXMMat * renderXMMat;

	DirectX::XMFLOAT4X4 modelMatrix;
	XMStoreFloat4x4(&modelMatrix, XMMatrixTranspose(refSpaceXMMat));
	g_theXRRenderer->SetModelConstants(modelMatrix);
}
