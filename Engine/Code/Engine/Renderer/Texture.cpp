#include "Engine/Renderer/Texture.hpp"
#include "Engine/core/EngineCommon.hpp"
#include <d3d11.h>


Texture::Texture()
{

}

Texture::~Texture()
{
	DX_SAFE_RELEASE(m_texture);
	DX_SAFE_RELEASE(m_shaderResourceView);
}
