#pragma once
#include <string>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

struct ShaderConfig
{
	std::string m_shaderPath;
	std::string m_vertexEntryPoint = "VertexMain";
	std::string m_pixelEntryPoint = "PixelMain";
};

class Shader
{
	friend class Render;

public:
	Shader(const ShaderConfig& config);
	Shader(const Shader& copy) = delete;// don't allow for copying shader because it will cause two pointer pointing to the same thing, risk for dangling pointers
	~Shader();

	const std::string& GetName() const;

	ShaderConfig		m_config;
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader*	m_pixelShader = nullptr;
	ID3D11InputLayout*	m_inputLayoutForVertex = nullptr; // Could be PCU or PCUTBN
};