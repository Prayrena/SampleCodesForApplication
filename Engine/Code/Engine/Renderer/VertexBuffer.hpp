#pragma once
#include "Engine/core/Vertex_PCU.hpp"

struct ID3D11Buffer;
struct ID3D11Device;

class VertexBuffer
{
public:// only the renderer class has the right to create and manage this class(this is changed)
	VertexBuffer(size_t size, size_t stride = sizeof(Vertex_PCU), ID3D11Device* device = nullptr);
	VertexBuffer(VertexBuffer const& copy) = delete;
	virtual ~VertexBuffer(); // virtual deconstructor will also be triggered when its children deconstructor is deleted

	void Create();
	void Resize(size_t size);

	ID3D11Device*	m_device = nullptr;
	ID3D11Buffer*	m_buffer = nullptr;
	size_t			m_size = 0;
	unsigned int	m_stride = 0;
};