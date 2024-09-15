#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/core/EngineCommon.hpp"

#include <d3d11.h>

VertexBuffer::VertexBuffer(size_t size, size_t stride /*= sizeof(Vertex_PCU)*/, ID3D11Device* device /*= nullptr*/)
	: m_size(size)
	, m_stride((unsigned int)(stride))
	, m_device(device)
{

}

VertexBuffer::~VertexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);
}

void VertexBuffer::Resize(size_t size)
{
	m_size = size;
}

