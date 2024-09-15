#pragma once
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <vector>

class	Window;
class	Texture;
class	Image;
class	VertexBuffer;
class	IndexBuffer;
class	ConstantBuffer;
struct	ID3D11RasterizerState;
struct	ID3D11RenderTargetView;
struct	ID3D11Device;
struct	ID3D11DeviceContext;
struct	IDXGISwapChain;
struct  ID3D11BlendState;
struct	ID3D11SamplerState;
struct  ID3D11DepthStencilState;
struct  ID3D11DepthStencilView;

#if defined(OPAQUE)
#undef OPAQUE
#endif

struct LightingConstants
{
	LightingConstants(Vec3 direction, float sunIntensity, float ambientIntensity)
		: SunDirection(direction)
		, SunIntensity(sunIntensity)
		, AmbientIntensity(ambientIntensity)
	{}
	LightingConstants() {}
	~LightingConstants() {}

	Vec3 SunDirection;
	float SunIntensity;
	float AmbientIntensity;
	float EmptySpace[3]; // GPU requires every constant buffer has the multiple times size of 16
};

enum class BlendMode
{
	OPAQUE,
	ALPHA,
	ADDITIVE,
	COUNT
};

enum class VertexType
{
	Vertex_PCU,
	Vertex_PCUTBN,
	COUNT
};

// each enum is defining the 
enum class SamplerMode
{
	POINT_CLAMP,	// for bit font map
	BILINEAR_WRAP,	// for texture rendering on objects
	COUNT
};

enum class RasterizerMode
{
	SOLID_CULL_NONE,
	SOLID_CULL_BACK,
	WIREFRAME_CULL_NONE,
	WIREFRAME_CULL_BACK,
	COUNT
};

enum class DepthMode
{
	DISABLED,
	ENABLED,
	COUNT
};

struct RenderConfig
{
	Window* m_window = nullptr;
};

class Renderer 
{
public:
	Renderer( RenderConfig const& config );
	Renderer();

	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();
	void ReportErrorLeaksAndReleaseDebugModule();
	 
	void ClearScreen(const Rgba8& clearColor);
	void BeginCamera(const Camera& camera);
	void EndCamera(const Camera& camera);
	void DrawVertexArray(int numVertices, Vertex_PCU const* vertexArray);
	void DrawVertexArray(int numVertices, Vertex_PCUTBN const* vertexArray);
	void DrawVertexArrayWithIndexArray(VertexBuffer* vbo, IndexBuffer* ibo, int numIndexes);

	// void CreateRenderingContext();
	void CreateDeviceAndSwapChain();
	void GetBackBufferAndCreateRenderTargetView();
	void CreateAndBindDefaultShader();
	void CreateImmediateVertexBuffer(); // for vertex PCU
	void CreateImmediateVertexPCUTBNBuffer();

	// texture and image functions
	Texture*	CreateOrGetTextureFromFile(char const* imageFilePath);
	Texture*	CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData); // only called when the texture is not loaded before
	Texture*	GetTextureForFileName(char const* imageFilePath);
	void		BindTexture(Texture const* texture);
	void		CreateDefaultTexture();
	Image*		CreateImageFromFile(char const* imageFilePath);
	Texture*	CreateTextureFromImage(Image const& image);

	BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension);
	BitmapFont* GetBitMapFontForFileName(char const* bitmapFontFilePathWithNoExtension);

	void		DetectDXGIMemoryLeak();

	// change rendering mode and states
	void		SetBlendMode(BlendMode blendMode);
	void		SetDepthMode(DepthMode depthMode);
	void		SetRasterizerMode(RasterizerMode rasterizerState);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	ConstantBuffer* CreateConstantBuffer(const size_t size);
	void			BindConstantBuffer(int slot, ConstantBuffer* cbo);
	void			CopyCPUToGPU(void const* data, size_t size, ConstantBuffer* cbo);

	// constant buffer
	ConstantBuffer* m_cameraCBO = nullptr;
	ConstantBuffer* m_modelCBO = nullptr;
	ConstantBuffer* m_lightingCBO = nullptr;

	VertexBuffer*   m_immediateVBO = nullptr; // backward compatibility for Libra and Starship
	VertexBuffer*   m_immediateVertex_PCUTBN_BO = nullptr; // buffer for vertex_PCUTBN
	// IndexBuffer* m_immediateIBO = nullptr; // because the index buffer is created and managed by different class like map

	// camera constant buffer related functions
	void			CreateCameraConstantBuffer();

	void			CreateLightingConstantBuffer();
	void			SetLightingConstants(Vec3 lightDirection, float sunIntensity, float ambientIntensity);
	void			SetLightingConstants(LightingConstants newSetting);

	Vec2			GetRenderWindowDimensions() const;
	AABB2			GetCameraViewportForD3D11(Camera const camera) const; // TL is (0.f, 0.f), BR is (2000.f, 1000.f)

	// model constant buffer related functions
	void		  CreateModelConstantBuffer();
	void		  SetModelConstants(const Mat44& modelMatrix = Mat44(), const Rgba8& modelColor = Rgba8::WHITE);
	void		  SetModelConstantsTesting(const Mat44& modelMatrix = Mat44(), const Rgba8& modelColor = Rgba8::WHITE);

	// Dynamic Buffers
	// vertex buffer functions
	VertexBuffer* CreateVertexBuffer(const size_t size, size_t stride = sizeof(Vertex_PCU), ID3D11Device* device = nullptr);
	void		  CopyCPUToGPU(void const* data, size_t size, VertexBuffer*& vbo);
	void		  BindVertexBuffer(VertexBuffer* vbo, int vertexOffset = 0);
	void		  DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset = 0);
	// index buffer functions
	IndexBuffer*  CreateIndexBuffer(const size_t size, size_t stride = sizeof(int), ID3D11Device* device = nullptr);
	void		  CopyCPUToGPU(void const* data, size_t size, IndexBuffer*& ibo);
	void		  BindIndexBuffer(IndexBuffer* ibo, int indexOffset);
	void		  DrawVertexAndIndexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, int indexCount, int indexOffset = 0, int vertexOffset = 0);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// dx11 shader creation functions
	Shader*		  GetLoadedShader(char const* shaderName);
	Shader*		  CreateOrGetShader(char const* shaderPath, VertexType vertexType = VertexType::Vertex_PCU); // see if we need to create a new shader or just reloaded the one
	Shader*		  CreateShader(char const* shaderPath, char const* shaderSource, VertexType vertexType = VertexType::Vertex_PCU);
	Shader*		  CreateShader(char const* shaderPath, VertexType vertexType = VertexType::Vertex_PCU);
	bool		  CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name,
											char const* shaderSource, char const* entryPoint, char const* target);
	void		  BindShader(Shader* shader);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// openXR support
	// std::vector<std::string> GetInstanceExtensions() const;
	// void InitializeDeviceForOpenXR(XrInstance instance, XrSystemId systemId);


private:
	Texture*	CreateTextureFromFile(char const* imageFilePath); // todo:??? why it is a texture* not texture
	BitmapFont* CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension);

	// should not let any else class modify the the loaded assets of textures and fonts
	std::vector<Texture*>		m_loadedTextures;
	Texture const*				m_defaultTexture = nullptr;
	Texture const*				m_currentTexture = nullptr;
	std::vector<BitmapFont*>	m_loadedFonts;


	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// blend mode 
	BlendMode		  m_desiredBlendMode = BlendMode::ALPHA;
	ID3D11BlendState* m_currentBlendState = nullptr;
	ID3D11BlendState* m_blendStates[(int)(BlendMode::COUNT)] = {};

	void			  SetStatesIfChanged();
	void			  CreateAllBlendStates();

	// sampler state
	ID3D11SamplerState* m_currentSamplerState = nullptr;
	SamplerMode			m_desiredSamplerMode = SamplerMode::POINT_CLAMP;
	ID3D11SamplerState* m_samplerStates[(int)(SamplerMode::COUNT)] = {};

	// rasterizer state
	void						CreateAllRasterizerStates();

	RasterizerMode				m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	RasterizerMode				m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	ID3D11RasterizerState*		m_rasterizerState[(int)(RasterizerMode::COUNT)];

	void						CreateSamplerState();
	void						SetSamplerMode(SamplerMode samplerMode);

	// depth mode
	void						CreateStencilTextureAndViewAndAllDepthStencilStates();
	// stencil depth property
	DepthMode					m_depthMode = DepthMode::ENABLED;
	DepthMode					m_desiredDepthMode = DepthMode::ENABLED;
	ID3D11DepthStencilState*	m_depthStencilStates[(int)(DepthMode::COUNT)] = {};
	ID3D11DepthStencilView*		m_depthStencilView = nullptr;
	ID3D11Texture2D*			m_depthStencilTexture = nullptr;

protected:
	RenderConfig m_config;

	void* m_rc = nullptr; // Gfx API rendering context: "HGLRC" in Windows/OpenGL

	std::vector<Shader*> m_loadedShaders;// cache pattern
	Shader* m_currentShader = nullptr;
	Shader* m_defaultShader = nullptr;

	ID3D11RenderTargetView*		m_renderTargetView = nullptr;
	ID3D11Device*				m_device = nullptr;
	ID3D11DeviceContext*		m_deviceContext = nullptr;
	IDXGISwapChain*				m_swapChain = nullptr;
};