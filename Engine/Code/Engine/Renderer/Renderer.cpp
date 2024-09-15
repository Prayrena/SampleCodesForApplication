#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/core/Image.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/FileUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "ThirdParty/stb/stb_image.h"

#define WIN32_LEAN_AND_MEAN		
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
//BitmapFont* Renderer::CreateOrGetTextureFromFile(const char* bitmapFilePathWithNoExtension);

// openXR support
// #include "ThirdParty/OpenXR/include/openxr/openxr.h"
// #include "ThirdParty/OpenXR/include/openxr/openxr_platform.h"
// #include "ThirdParty/OpenXR/include/openxr/openxr_reflection.h"
// bool  m_renderingInXR = true;

#if defined(ENGINE_DEBUG_RENDER)
void* m_dxgiDebugModule = nullptr;
void* m_dxgiDebug = nullptr;
#endif

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#if defined(OPAQUE)
#undef OPAQUE
#endif

//----------------------------------------------------------------------------------------------------------------------------------------------------
const char* defaultShaderSource = R"(
float GetFractionWithinRange(float value, float rangeStart, float rangeEnd)
{
	float disp = rangeEnd - rangeStart;
	float proportion = (value - rangeStart) / disp;
	return proportion;
}

float Interpolate(float start, float end, float fractionTowardEnd)
{
	float disp = end - start;
	float distWithinRange = disp * fractionTowardEnd;
	float interpolatedPosition = start + distWithinRange;
	return interpolatedPosition;
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float proportion = GetFractionWithinRange(inValue, inStart, inEnd);

	float outValue = Interpolate(outStart, outEnd, proportion);
	return outValue;
}

Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

	cbuffer CameraConstants : register(b2)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

	cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

struct vs_input_t
	{
		float3 localPosition : POSITION;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
	};

	struct v2p_t
	{
		float4 position : SV_Position;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
	};


	v2p_t VertexMain(vs_input_t input)
	{
		float4 localPosition = float4(input.localPosition, 1);
		float4 worldPosition = mul(ModelMatrix, localPosition);
		float4 renderPosition = mul(ViewMatrix, worldPosition);
		float4 clipPosition = mul(ProjectionMatrix, renderPosition);

		v2p_t v2p;
		v2p.position = clipPosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
		textureColor *= ModelColor;
		textureColor *= input.color;
		clip(textureColor.a - 0.01f);
		return float4(textureColor);
	}

	)";

//----------------------------------------------------------------------------------------------------------------------------------------------------
struct CameraConstants
{
	Mat44 ViewMatrix;
	Mat44 ProjectionMatrix;
};

struct ModelConstants
{
	Mat44 ModelMatrix;
	float ModelColor[4];
};

static const int k_lightingConstantsSlot = 1;
static const int k_cameraConstantsSlot = 2;
static const int k_modelConstantsSlot = 3;

Renderer::Renderer(RenderConfig const& config)
	:m_config(config)
{
	
}
 
Renderer::Renderer()
{

}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Renderer::Startup()
{
	CreateDeviceAndSwapChain();
	GetBackBufferAndCreateRenderTargetView();

	CreateAndBindDefaultShader();
	CreateImmediateVertexBuffer();
	CreateImmediateVertexPCUTBNBuffer();
	CreateCameraConstantBuffer();
	CreateModelConstantBuffer();

	// lighting constant creation for diffuse shader
	CreateLightingConstantBuffer();

	SetModelConstants();

	CreateAllBlendStates();

	CreateAllRasterizerStates();

	CreateStencilTextureAndViewAndAllDepthStencilStates();

	CreateDefaultTexture();
	BindTexture(m_defaultTexture);

	CreateSamplerState();
	SetSamplerMode(SamplerMode::POINT_CLAMP);

	DetectDXGIMemoryLeak();
}

void Renderer::CreateDeviceAndSwapChain()
{
// render startup
	unsigned int deviceFlags = 0;
#if defined(ENGINE_DEBUG_RENDER)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// create device and swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferDesc.Width = m_config.m_window->GetClientDimensions().x;
	swapChainDesc.BufferDesc.Height = m_config.m_window->GetClientDimensions().y;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = (HWND)m_config.m_window->GetHwnd();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, nullptr, 0, D3D11_SDK_VERSION,
		&swapChainDesc, &m_swapChain, &m_device, nullptr, &m_deviceContext);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create D3D 11 device and swap chain.");
	}
}

void Renderer::GetBackBufferAndCreateRenderTargetView()
{
	HRESULT hr;
	// Get back buffer texture
	ID3D11Texture2D* backBuffer;
	hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not get swap chain buffer.")
	}

	hr = m_device->CreateRenderTargetView(backBuffer, NULL, &m_renderTargetView);// view is a wrapper 
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not render target view for swap chain buffer.");
	}

	backBuffer->Release();
}

void Renderer::CreateAndBindDefaultShader()
{
	// create and bind the default shader
	const char* shaderName = "Default";
	m_defaultShader = CreateShader(shaderName, defaultShaderSource);
	BindShader(m_defaultShader);
}

void Renderer::CreateImmediateVertexBuffer()
{
	// create the immediate vertex and specify an initial size big enough for one Vertex_PCU
	size_t vertexSize = sizeof(Vertex_PCU);
	m_immediateVBO = new VertexBuffer((3 * vertexSize), vertexSize, m_device);
}

void Renderer::CreateImmediateVertexPCUTBNBuffer()
{
	// create the immediate vertex and specify an initial size big enough for one Vertex_PCUTBN
	size_t vertexSize = sizeof(Vertex_PCUTBN);
	m_immediateVertex_PCUTBN_BO = new VertexBuffer(3, vertexSize, m_device); // the smallest will have three verts
}

void Renderer::CreateStencilTextureAndViewAndAllDepthStencilStates()
{
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = m_config.m_window->GetClientDimensions().x;
	textureDesc.Height = m_config.m_window->GetClientDimensions().y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.SampleDesc.Count = 1;

	HRESULT hr = m_device->CreateTexture2D(&textureDesc, nullptr, &m_depthStencilTexture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create texture for depth stencil.");
	}

	hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create depth stencil view.");
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::DISABLED]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::Disable failed");
	}

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::ENABLED]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::Enabled failed");
	}

	UINT stencilRef = 0xffffffff;
	m_deviceContext->OMSetDepthStencilState(m_depthStencilStates[(int)m_depthMode], stencilRef);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Renderer::Shutdown()
{
	// release all the shaders
	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		delete m_loadedShaders[shaderIndex];
	}

	// release all blend states
	for (int i = 0; i < (int)BlendMode::COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_blendStates[i]);
	}

	// release all the sampler mode
	for (int i = 0; i < (int)SamplerMode::COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_samplerStates[i]);
	}

	// release all the rasterizer states
	for (int i = 0; i < (int)(RasterizerMode::COUNT); ++i)
	{
		DX_SAFE_RELEASE(m_rasterizerState[i]);
	}

	// release all depth stencil states
	for (int i = 0; i < (int)DepthMode::COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_depthStencilStates[i]);
	}
	
	// delete all the textures
	for (int i = 0; i < (int)m_loadedTextures.size(); ++i)
	{
		delete(m_loadedTextures[i]);
	}

	// delete all the fonts
	for (int i = 0; i < (int)m_loadedFonts.size(); ++i)
	{
		delete(m_loadedFonts[i]);
	}

	// release the constant buffers
	delete m_cameraCBO;
	m_cameraCBO = nullptr;

	delete m_modelCBO;
	m_modelCBO = nullptr;

	// delete the immediate buffer
	delete m_immediateVBO;
	m_immediateVBO = nullptr;
	delete m_immediateVertex_PCUTBN_BO;
	m_immediateVertex_PCUTBN_BO = nullptr;

	delete m_lightingCBO;
	m_lightingCBO = nullptr;

	// release all directX objects	  
	DX_SAFE_RELEASE(m_renderTargetView);
	DX_SAFE_RELEASE(m_swapChain);
	DX_SAFE_RELEASE(m_deviceContext);
	DX_SAFE_RELEASE(m_device);
	DX_SAFE_RELEASE(m_depthStencilView);
	DX_SAFE_RELEASE(m_depthStencilTexture);

	ReportErrorLeaksAndReleaseDebugModule();
}

void Renderer::ReportErrorLeaksAndReleaseDebugModule()
{
	// Report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDER)
	// for void*, does static cast and (point type) method have difference
	((IDXGIDebug*)m_dxgiDebug)->ReportLiveObjects(
		DXGI_DEBUG_ALL,
		(DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
	);

	((IDXGIDebug*)m_dxgiDebug)->Release();
	m_dxgiDebug = nullptr;

	::FreeLibrary((HMODULE)m_dxgiDebugModule);
	m_dxgiDebugModule = nullptr;
#endif
}

void Renderer::BeginFrame()
{
	// set render target
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

	// set depth stencil view
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
}

void Renderer::EndFrame()
{
	double timeAtStart = GetCurrentTimeSeconds();

	// present
	HRESULT hr;
 	hr = m_swapChain->Present(0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		ERROR_AND_DIE("Device has been lost, application will now terminate.");
	}

	double timeAtEnd = GetCurrentTimeSeconds();
	double timeElapsed = timeAtEnd - timeAtStart;
	g_theDevConsole->AddLine(Stringf("RendererEndFrame = %.02f ms", timeElapsed * 1000.0), Rgba8::LIGHT_ORANGE);
}

//void Renderer::CreateRenderingContext()
//{
//	// Creates an OpenGL rendering context (RC) and binds it to the current window's device context (DC)
//	PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
//	memset(&pixelFormatDescriptor, 0, sizeof(pixelFormatDescriptor));
//	pixelFormatDescriptor.nSize = sizeof(pixelFormatDescriptor);
//	pixelFormatDescriptor.nVersion = 1;
//	pixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
//	pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
//	pixelFormatDescriptor.cColorBits = 24; // 8 bits each for R, G, B
//	pixelFormatDescriptor.cDepthBits = 24; // 24 bits of precision for our
//	pixelFormatDescriptor.cAccumBits = 0;
//	pixelFormatDescriptor.cStencilBits = 8;
//
//	// Create an OpenGL rendering context (RC) and bind it to the current window's device context ( DC )
//	HDC dc = reinterpret_cast<HDC>(m_config.m_window->GetDeviceContext()); // transform the m_dc type
//	int pixelFormatCode = ChoosePixelFormat( dc, &pixelFormatDescriptor);
//	SetPixelFormat(dc, pixelFormatCode, &pixelFormatDescriptor);
//	HGLRC hglrc = wglCreateContext(dc); // Create a new OpenGL bound to this display context
//	wglMakeCurrent(dc, hglrc);
//	m_rc = hglrc; // This is an OpenGL "Rendering Context" (RC) which means "an instance of OpenGL"???????????????????????????????????????????????????????????????????????????
//}

void Renderer::ClearScreen(Rgba8 const& clearColor)
{
	// transform the input color variable from char to float
	// float r = float(clearColor.r) / 255.f;
	// float g = float(clearColor.g) / 255.f;
	// float b = float(clearColor.b) / 255.f;
	// float a = float(clearColor.a) / 255.f;

	// because gl take in float
	//glClearColor(r, g, b, a);
	//glClear(GL_COLOR_BUFFER_BIT);
	 
	//dx11-clear the screen
	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);

	// clear the depth stencil view
	m_deviceContext->ClearDepthStencilView(m_depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}	


//get the camera's info to define the glOrtho location
void Renderer :: BeginCamera(Camera const& camera)
{
	// for openGL
	// Vec2 BL = camera.GetOrthoBottomLeft();
	// Vec2 TR = camera.GetOrthoTopRight();
	// glLoadIdentity();
	// glOrtho(BL.x, TR.x, BL.y, TR.y, 0.f, 1.f);// stretch everything on the camera to the window, so different camera size match the same window space

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// for dx11
	// copy the camera info to the camera constant
	CameraConstants cameraInfo;
	cameraInfo.ProjectionMatrix = camera.GetProjectionMatrix();
	cameraInfo.ViewMatrix = camera.GetViewMatrix();

	CopyCPUToGPU(&cameraInfo, sizeof(CameraConstants), m_cameraCBO);
	BindConstantBuffer(k_cameraConstantsSlot, m_cameraCBO);
	 
	// dx11 set viewport
	D3D11_VIEWPORT viewport = { 0 };
	if (camera.GetNormalizedViewport() == AABB2()) // for orthographic cameras which has not set the normalized viewport, we give it full windows display
	{
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.Width = (float)m_config.m_window->GetClientDimensions().x;
		viewport.Height = (float)m_config.m_window->GetClientDimensions().y;
	}
	else // if the camera has a normalized viewport set up
	{
		// we need a coordinate for the top left, but asking for dimensions for the width and height
		viewport.TopLeftX = GetCameraViewportForD3D11(camera).m_mins.x;
		viewport.TopLeftY = GetCameraViewportForD3D11(camera).m_mins.y;
		viewport.Width = GetCameraViewportForD3D11(camera).GetDimensions().x;
		viewport.Height = GetCameraViewportForD3D11(camera).GetDimensions().y;
	}
	viewport.MinDepth = 0.f;	
	viewport.MaxDepth = 1.f;

	m_deviceContext->RSSetViewports(1, &viewport);
}

void Renderer::EndCamera(const Camera& camera)
{
	UNUSED(camera);
}

// take every three vertex of the array and draw a triangle
// cont* means this function will not change the vertex Array comming from the input
void Renderer :: DrawVertexArray(int numVertices, Vertex_PCU const* vertexArray)
{
	//glBegin(GL_TRIANGLES);
	//for (int vertexIndex = 0; vertexIndex < numVertexes; ++vertexIndex)
	//{
	//	// use a temp variable to shorten the name
	//	//const& means we are only creating a read-only nickname for a read-only variable
	//	Vertex_PCU const& v = vertexArray[vertexIndex];

	//	glColor4ub(v.m_color.r, v.m_color.g, v.m_color.b, v.m_color.a);
	//	glTexCoord2f(v.m_uvTexCoords.x, v.m_uvTexCoords.y);
	//	glVertex3f(v.m_position.x, v.m_position.y, v.m_position.z);
	//}
	//glEnd();


	// dx11 backwards compatibility for Libra and starship
	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArraySize = numVertices * vertexSize;
	CopyCPUToGPU(vertexArray, vertexArraySize, m_immediateVBO);
	DrawVertexBuffer(m_immediateVBO, numVertices, 0);
}

void Renderer::DrawVertexArray(int numVertices, Vertex_PCUTBN const* vertexArray)
{
	size_t vertexSize = sizeof(Vertex_PCUTBN);
	size_t vertexArraySize = numVertices * vertexSize;
	CopyCPUToGPU(vertexArray, vertexArraySize, m_immediateVertex_PCUTBN_BO);
	DrawVertexBuffer(m_immediateVertex_PCUTBN_BO, numVertices, 0);
}

// we are doing this part in the map class at the start up, because the vertexArray and indexArray never changes
// so we don't need to input these two arguments: std::vector<Vertex_PCUTBN> vertexArray, std::vector<unsigned int> indexArray
void Renderer::DrawVertexArrayWithIndexArray(VertexBuffer* vbo, IndexBuffer* ibo, int numIndexes)
{
	// size_t vertexSize = sizeof(Vertex_PCUTBN);
	// size_t vertexArraySize = numVertices * vertexSize;
	// CopyCPUToGPU(vbo, vertexArraySize, vbo);
	// 
	// size_t indexSize = sizeof(int);
	// size_t indexArraySize = numIndexes * indexSize;
	// CopyCPUToGPU(ibo, indexArraySize, ibo);

	DrawVertexAndIndexBuffer(vbo, ibo, numIndexes);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}
	else
	{
		// Never seen this texture before!  Let's load it.
		Texture* newTexture = CreateTextureFromFile(imageFilePath);
		return newTexture;
	}
}

Image* Renderer::CreateImageFromFile(char const* imageFilePath)
{
	Image* newImage = new Image(imageFilePath);
	return newImage;
}

Texture* Renderer::GetTextureForFileName(char const* imageFilePath)
{
	for (int i = 0; i < (int)m_loadedTextures.size(); i++)
	{
		if (!strcmp(m_loadedTextures[i]->GetImageFilePath().c_str(), imageFilePath))
		{
			return m_loadedTextures[i];
		}
	}

	return nullptr;
}

Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	// IntVec2 dimensions;		// This will be filled in for us to indicate image width & height
	// int bytesPerTexel = 0; // This will be filled in for us to indicate how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)
	// int numComponentsRequested = 0; // don't care; we support 3 (24-bit RGB) or 4 (32-bit RGBA)

	// because we are using image class and stbi there to handle the texture data, we don't deal with it here
	// // Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
	// stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
	// unsigned char* texelData = stbi_load(imageFilePath, &dimensions.x, &dimensions.y, &bytesPerTexel, numComponentsRequested);
	//  
	// // Check if the load was successful
	// GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath));
	
	// // Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
	// stbi_image_free(texelData);

	Image* newImagePtr = CreateImageFromFile(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(*newImagePtr);
	 
	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

Texture* Renderer::CreateTextureFromImage(const Image& image)
{
	Texture* newTexture = new Texture();
	newTexture->m_name = image.GetImageFilePath(); // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
	newTexture->m_dimensions = image.GetDimensions();

	D3D11_TEXTURE2D_DESC textureDesc = {0};
	textureDesc.Width = image.GetDimensions().x;
	textureDesc.Height = image.GetDimensions().y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA textureData;
	textureData.pSysMem = image.GetRawData();
	textureData.SysMemPitch = 4 * image.GetDimensions().x;

	HRESULT hr;
	hr = m_device->CreateTexture2D(&textureDesc, &textureData, &newTexture->m_texture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("CreateTextureFromImage function failed for image file \"%\".",
			image.GetImageFilePath().c_str()));
	}

	hr = m_device->CreateShaderResourceView(newTexture->m_texture, NULL, &newTexture->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("CreateShaderResourceView failed for image file \"%\".",
			image.GetImageFilePath().c_str()));
	}

	return newTexture;
}

BitmapFont* Renderer::GetBitMapFontForFileName(char const* bitmapFontFilePathWithNoExtension)
{
	// see if can find the load font with the same file path name
	for (int i = 0; i < (int)m_loadedFonts.size(); i++)
	{
		if (!strcmp(m_loadedFonts[i]->m_fontFilePathNameWithNoExtension.c_str(), bitmapFontFilePathWithNoExtension))
		{
			return m_loadedFonts[i];
		}
	}
	return nullptr;
}

void Renderer::SetBlendMode(BlendMode blendMode)
{
	switch (blendMode)
	{
	case BlendMode::OPAQUE: {m_desiredBlendMode = BlendMode::OPAQUE; } break;
	case BlendMode::ALPHA: {m_desiredBlendMode = BlendMode::ALPHA; } break;
	case BlendMode::ADDITIVE: {m_desiredBlendMode = BlendMode::ADDITIVE; } break;
	default: {ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", blendMode)); } break;
	}

	// if (blendMode == BlendMode::ALPHA)
	// {
	// 	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// }
	// else if (blendMode == BlendMode::ADDITIVE)
	// {
	// 	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	// }
	// else
	// {
	// 	ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", blendMode));
	// }
}

void Renderer::SetDepthMode(DepthMode depthMode)
{
	switch (depthMode)
	{
	case DepthMode::DISABLED: {m_desiredDepthMode = DepthMode::DISABLED; }
		break;
	case DepthMode::ENABLED: {m_desiredDepthMode = DepthMode::ENABLED; }
		break;
	default: {m_desiredDepthMode = DepthMode::ENABLED; }
		break;
	}
}

void Renderer::SetRasterizerMode(RasterizerMode rasterizerState)
{
	switch (rasterizerState)
	{
	case RasterizerMode::SOLID_CULL_NONE: {m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_NONE; }
		break;
	case RasterizerMode::SOLID_CULL_BACK: {m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK; }
		break;
	case RasterizerMode::WIREFRAME_CULL_NONE: {m_desiredRasterizerMode = RasterizerMode::WIREFRAME_CULL_NONE; }
		break;
	case RasterizerMode::WIREFRAME_CULL_BACK: {m_desiredRasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK; }
		break;
	default: {ERROR_AND_DIE(Stringf("Unknown / unsupported rasterizer State #%i", rasterizerState)); }
		break;
	}
}

void Renderer::CreateSamplerState()
{
	// create the point clamp sampler state
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr;
	hr = m_device->CreateSamplerState(&samplerDesc,
		&m_samplerStates[(int)SamplerMode::POINT_CLAMP]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for samplerMode::POINT_CLAMP failed.")
	}

	// create the bilinear wrap sampler state
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	hr = m_device->CreateSamplerState(&samplerDesc,
		&m_samplerStates[(int)SamplerMode::BILINEAR_WRAP]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for samplerMode::POINT_WRAP failed.")
	}
}

void Renderer::SetSamplerMode(SamplerMode samplerMode)
{
	switch (samplerMode)
	{
	case SamplerMode::POINT_CLAMP: { m_desiredSamplerMode = SamplerMode::POINT_CLAMP; } break;
	case SamplerMode::BILINEAR_WRAP: { m_desiredSamplerMode = SamplerMode::BILINEAR_WRAP; } break;
	default: { m_desiredSamplerMode = SamplerMode::POINT_CLAMP; } break;
	}
}

void Renderer::DetectDXGIMemoryLeak()
{
	// create debug module
#if defined(ENGINE_DEBUG_RENDER)
	m_dxgiDebugModule = (void*)::LoadLibraryA("dxgidebug.dll");
	if (m_dxgiDebugModule == nullptr)
	{
		ERROR_AND_DIE("Could not load dxgidebug.dll");
	}

	typedef HRESULT(WINAPI* GetDebugModuleCB)(REFIID, void**);
	(
		(GetDebugModuleCB)::GetProcAddress((HMODULE)m_dxgiDebugModule, "DXGIGetDebugInterface")
		)
		(__uuidof(IDXGIDebug), &m_dxgiDebug);

	if (m_dxgiDebug == nullptr)
	{
		ERROR_AND_DIE("Could not load debug module.");
	}
#endif
}

BitmapFont* Renderer::CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	// See if we already have this font previously loaded
	BitmapFont* existingFont = GetBitMapFontForFileName(bitmapFontFilePathWithNoExtension);
	if (existingFont)
	{
		return existingFont;
	}
	// Never seen this font before!  Let's load it.
	BitmapFont* font = CreateBitmapFont(bitmapFontFilePathWithNoExtension);
	m_loadedFonts.push_back(font);
	return font;
}


BitmapFont* Renderer::CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	Texture* fontTexture = CreateOrGetTextureFromFile(bitmapFontFilePathWithNoExtension);
	BitmapFont* font	 = new BitmapFont(bitmapFontFilePathWithNoExtension, *fontTexture); // todo:??? need to use 'new' or it will be store on stack memory and disppear after the end of this function right?
	return font;
}

Shader* Renderer::CreateOrGetShader(char const* shaderPath, VertexType vertexType /* = VertexType::Vertex_PCU*/)
{
	// convert the c-style string to C++ style string
	std::string newShaderName;
	newShaderName = std::string(shaderPath);

	// check to see if the shader has been loaded before
	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		if (newShaderName == m_loadedShaders[shaderIndex]->m_config.m_shaderPath)
		{
			// if so, skip the step of shader creation
			// just set is as current shader
			return m_loadedShaders[shaderIndex];
		}
	}

	// if the name of the shader does not exist, create a new shader
	ShaderConfig newConfig;
	Shader* newShader = new Shader(newConfig);
	newShader = CreateShader(shaderPath, vertexType);
	return newShader;
}

Shader* Renderer::CreateShader(char const* shaderPath, char const* shaderSource, VertexType vertexType /* = VertexType::Vertex_PCU*/)
{
	// create a new shader config and a shader
	ShaderConfig config;
	config.m_shaderPath = shaderPath;
	Shader* newShader = new Shader(config); // need to "new" to keep it in heap memory, otherwise it will be deleted after the function is excuted

	// compile byteCode and create the shader's vertex shader
	std::vector<unsigned char> vertexShaderByteCode;
	if (!CompileShaderToByteCode(vertexShaderByteCode, "VertexShader", shaderSource, "VertexMain", "vs_5_0"))
	{
		ERROR_AND_DIE(Stringf("Could not compile vertex shader."));
	}
	else
	{
		HRESULT hr;
		hr = m_device->CreateVertexShader(
			vertexShaderByteCode.data(),
			vertexShaderByteCode.size(),
			NULL, &newShader->m_vertexShader
		);

		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE(Stringf("Could not create vertex shader."));
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------	
	// compile byteCode and create the shader's pixel shader
	std::vector<unsigned char> pixelShaderByteCode;
	if (!CompileShaderToByteCode(pixelShaderByteCode, "PixelShader", shaderSource, "PixelMain", "ps_5_0"))
	{
		ERROR_AND_DIE(Stringf("Could not compile pixel shader."));
	}
	else
	{
		//create pixel shader
		HRESULT hr;
		hr = m_device->CreatePixelShader(
			pixelShaderByteCode.data(),
			pixelShaderByteCode.size(),
			NULL, &newShader->m_pixelShader
		);

		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE(Stringf("Could not create pixel shader."));
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// create local array of input element descriptions that defines the vertex layout
	// based on the chosen vertex type, we have different input element description
	HRESULT hr;
	UINT numElements;
	switch (vertexType)
	{
	case VertexType::Vertex_PCU:
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		numElements = ARRAYSIZE(inputElementDesc);
		hr = m_device->CreateInputLayout(
			inputElementDesc, numElements,
			vertexShaderByteCode.data(),
			vertexShaderByteCode.size(),
			&newShader->m_inputLayoutForVertex);

		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE(Stringf("Could not create Vertex_PCUTBN layout."));
		}
	}break;
	case VertexType::Vertex_PCUTBN:
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		numElements = ARRAYSIZE(inputElementDesc);
		hr = m_device->CreateInputLayout(
			inputElementDesc, numElements,
			vertexShaderByteCode.data(),
			vertexShaderByteCode.size(),
			&newShader->m_inputLayoutForVertex);

		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE(Stringf("Could not create Vertex_PCUTBN layout."));
		}
	}break;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add this new shader to loaded shaders and return it
	m_loadedShaders.push_back(newShader);
	return newShader;
}

Shader* Renderer::CreateShader(char const* shaderPath, VertexType vertexType /* = VertexType::Vertex_PCU*/)
{
	// get the file path
	// if the we try to create the default shader, it is going to read from the renderer folder file
	// if we try to create new shader, it is going to read from the data/shaders folder
	std::string fileName = std::string(shaderPath);
	fileName.append(".hlsl");

	// read the file and get the shader source code 
	std::string shaderString;
	FileReadToString(shaderString, fileName);
	defaultShaderSource = shaderString.c_str();

	// create and return new shader
	Shader* newShader;
	newShader = CreateShader(shaderPath, defaultShaderSource, vertexType);
	m_currentShader = newShader;
	return newShader;
}

// outByteCode is a machine code that could read by machine, therefore needed to be compile by the DirectX 3D
bool Renderer::CompileShaderToByteCode(std::vector<unsigned char>&outByteCode, char const* name, char const* shaderSource, char const* entryPoint, char const* target)
{
	// Compile vertex shader
	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif
	ID3DBlob* shaderBlob = NULL;
	ID3DBlob* errorBlob = NULL;

	HRESULT hr;
	hr = D3DCompile(
		shaderSource, strlen(shaderSource),
		name, nullptr, nullptr,
		entryPoint, target, shaderFlags, 0,
		&shaderBlob, &errorBlob
	);

	if (SUCCEEDED(hr))
	{
		outByteCode.resize(shaderBlob->GetBufferSize());
		memcpy(
			outByteCode.data(),
			shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize()
		);
	}
	else
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)errorBlob->GetBufferPointer());
			return false;
		}

		// std::string nameOfShader;
		// nameOfShader = std::string(name);
		// customized the error handling to show which kind of shader is not compiling
		// ERROR_AND_DIE(Stringf("Could not compile %s shader", nameOfShader.c_str()));// this way is how to treat C++ style string
		// ERROR_AND_DIE(Stringf("Could not compile %s shader", name));// todo:??? why this also works
	}

	shaderBlob->Release();
	if (errorBlob != NULL)
	{
		errorBlob->Release();
		return false;	// return false when there is an error
	}
	return true;	// return true when the compile is successful
}

void Renderer::BindShader(Shader* shader)
{
	// if the new binding shader is nullptr, then use current shader
	if (shader == nullptr)
	{
		shader = m_defaultShader;
		m_currentShader = m_defaultShader;
	}
	else if (m_currentShader == shader)
	{
		return;
	}
	else
	{
		// if the shader do exist, update the current shader info
		m_currentShader = shader;
	}

	m_deviceContext->IASetInputLayout(shader->m_inputLayoutForVertex);
	m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
	m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);
}

VertexBuffer* Renderer::CreateVertexBuffer(const size_t size, size_t stride /*sizeof(Vertex_PCU)*/, ID3D11Device* device /*nullptr*/)
{
	// create a local D3D11_BUFFER_DESC variable
	// create vertex buffer
	if (!device)
	{
		device = m_device;
	}
	VertexBuffer* newBuffer = new VertexBuffer(size, stride, device);
	UINT vertexBufferSize = (UINT)(size * stride);
	D3D11_BUFFER_DESC bufferDesc = { 0 };// trying to initialize all the variable inside the struct as default
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = vertexBufferSize;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &newBuffer->m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer.");
	}
	
	return newBuffer;
}

IndexBuffer* Renderer::CreateIndexBuffer(const size_t size, size_t stride /*sizeof(int)*/, ID3D11Device* device /*nullptr*/)
{
	// create a local D3D11_BUFFER_DESC variable
	// create index buffer
	if (!device)
	{
		device = m_device;
	}
	IndexBuffer* newBuffer = new IndexBuffer(size, stride, device);
	UINT indexBufferSize = (UINT)(size * stride);
	D3D11_BUFFER_DESC bufferDesc = { 0 };// trying to initialize all the variable inside the struct as default
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = indexBufferSize;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &newBuffer->m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create index buffer.");
	}

	return newBuffer;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Renderer::CopyCPUToGPU(void const* data, size_t size, VertexBuffer*& vbo)
{
	// Check if the exiting immediate vertex buffer is large enough for the data being passed in
	if ((vbo->m_size * vbo->m_stride) < size)
	{
		size_t stride = vbo->m_stride;
		delete vbo;
		// recreate the vertex buffer so it is sufficiently large and not introduce memory leaks
		// todo:??? createVertexBuffer will size*stride again for the buffer
		vbo = CreateVertexBuffer(size, stride, m_device);
	}

	// copy the vertex buffer data from the CPU to GPU
	// copy vertices
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(vbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(vbo->m_buffer, 0);
}

void Renderer::CopyCPUToGPU(void const* data, size_t size, ConstantBuffer* cbo)
{
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(cbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(cbo->m_buffer, 0);
}

void Renderer::CopyCPUToGPU(void const* data, size_t size, IndexBuffer*& ibo)
{
	// Check if the exiting immediate vertex buffer is large enough for the data being passed in
	if (ibo->m_size < size)
	{
		delete ibo;
		ibo = CreateIndexBuffer(size);
	}

	// copy the Index buffer data from the CPU to GPU
	// copy vertices
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(ibo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(ibo->m_buffer, 0);
}

// pass value to Shader
void Renderer::BindConstantBuffer(int slot, ConstantBuffer* cbo)
{
	m_deviceContext->VSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->PSSetConstantBuffers(slot, 1, &cbo->m_buffer);
}

void Renderer::BindIndexBuffer(IndexBuffer* ibo, int indexOffset /*= 0*/)
{
	m_deviceContext->IASetIndexBuffer(ibo->m_buffer, DXGI_FORMAT_R32_UINT, indexOffset);
}

void Renderer::BindVertexBuffer(VertexBuffer* vbo, int vertexOffset /*= 0*/)
{
	UINT stride = vbo->m_stride;
	UINT startOffset = vertexOffset;
	m_deviceContext->IASetVertexBuffers(0, 1, &vbo->m_buffer, &stride, &startOffset);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Renderer::DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset /*= 0*/)
{
	BindVertexBuffer(vbo);
	SetStatesIfChanged();
	m_deviceContext->Draw(vertexCount, vertexOffset);
}

void Renderer::DrawVertexAndIndexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, int indexCount, 
	int indexOffset /*= 0*/, int vertexOffset /*= 0*/)
{
	BindVertexBuffer(vbo, vertexOffset);
	BindIndexBuffer(ibo, indexOffset);
	SetStatesIfChanged();
	m_deviceContext->DrawIndexed(indexCount, 0, 0);
}

Shader* Renderer::GetLoadedShader(char const* shaderName)
{
	// check to see if the shader has been loaded before
	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		if (shaderName == m_loadedShaders[shaderIndex]->m_config.m_shaderPath)
		{
			// if so, skip the step of shader creation
			// just set is as current shader			
			return m_loadedShaders[shaderIndex];
		}
	}

	return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

void Renderer::CreateCameraConstantBuffer()
{
	m_cameraCBO = CreateConstantBuffer(sizeof(CameraConstants));
}

void Renderer::CreateLightingConstantBuffer()
{
	m_lightingCBO = CreateConstantBuffer(sizeof(LightingConstants));
}

void Renderer::SetLightingConstants(Vec3 lightDirection, float sunIntensity, float ambientIntensity)
{
	LightingConstants lightingInfo;
	lightingInfo.SunDirection = lightDirection.GetNormalized();
	lightingInfo.SunIntensity = sunIntensity;
	lightingInfo.AmbientIntensity = ambientIntensity;

	CopyCPUToGPU(&lightingInfo, sizeof(LightingConstants), m_lightingCBO);
	BindConstantBuffer(k_lightingConstantsSlot, m_lightingCBO);
}

void Renderer::SetLightingConstants(LightingConstants newSetting)
{
	newSetting.SunDirection = newSetting.SunDirection.GetNormalized();
	CopyCPUToGPU(&newSetting, sizeof(LightingConstants), m_lightingCBO);
	BindConstantBuffer(k_lightingConstantsSlot, m_lightingCBO);
}

Vec2 Renderer::GetRenderWindowDimensions() const
{
	float width = (float)m_config.m_window->GetClientDimensions().x;
	float height = (float)m_config.m_window->GetClientDimensions().y;
	return Vec2(width, height);
}

// translate the 
AABB2 Renderer::GetCameraViewportForD3D11(Camera const camera) const
{
	float width = (float)m_config.m_window->GetClientDimensions().x;
	float height = (float)m_config.m_window->GetClientDimensions().y;

	AABB2 engineViewport = camera.GetNormalizedViewport();
	AABB2 D3D11Viewport;
	D3D11Viewport.m_mins.x = engineViewport.m_mins.x;
	D3D11Viewport.m_maxs.y = 1.f - engineViewport.m_mins.y;
	D3D11Viewport.m_maxs.x = engineViewport.m_maxs.x;
	D3D11Viewport.m_mins.y = 1.f - engineViewport.m_maxs.y;

	D3D11Viewport.m_mins.x *= width;
	D3D11Viewport.m_maxs.x *= width;
	D3D11Viewport.m_mins.y *= height;
	D3D11Viewport.m_maxs.y *= height;
	return D3D11Viewport;
}

void Renderer::CreateModelConstantBuffer()
{
	m_modelCBO = CreateConstantBuffer(sizeof(ModelConstants));
}

ConstantBuffer* Renderer::CreateConstantBuffer(const size_t size)
{
	// create a local D3D11_BUFFER_DESC variable
	// create vertex buffer
	ConstantBuffer* newBuffer = new ConstantBuffer(size);

	D3D11_BUFFER_DESC bufferDesc = { };
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = (UINT)size;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &newBuffer->m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer.");
	}

	return newBuffer;
}

void Renderer::SetModelConstants(const Mat44& modelMatrix /*= Mat44()*/, const Rgba8& modelColor /*= Rgba8::WHITE*/)
{
	ModelConstants modelInfo;
	modelInfo.ModelMatrix = modelMatrix;
	modelColor.GetAsFloats(modelInfo.ModelColor);

	CopyCPUToGPU(&modelInfo, sizeof(modelInfo), m_modelCBO);
	BindConstantBuffer(k_modelConstantsSlot, m_modelCBO);
}

void Renderer::SetModelConstantsTesting(const Mat44& modelMatrix /*= Mat44()*/, const Rgba8& modelColor /*= Rgba8::WHITE*/)
{
	ModelConstants modelInfo;
	modelInfo.ModelMatrix = modelMatrix;
	modelColor.GetAsFloats(modelInfo.ModelColor);

	CopyCPUToGPU(&modelInfo, sizeof(modelInfo), m_modelCBO);
	BindConstantBuffer(k_modelConstantsSlot, m_modelCBO);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------

void Renderer::CreateAllRasterizerStates()
{ 
	// set rasterizer state
	D3D11_RASTERIZER_DESC rasterizerDesc = {  };// trying to initialize all the variable inside the struct as default

	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.f;
	rasterizerDesc.SlopeScaledDepthBias = 0.f;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.AntialiasedLineEnable = true;
	HRESULT hr;

	// change fill and cull mode based on the rasterizer mode setting and create all the rasterizer state
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::SOLID_CULL_NONE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create WIREFRAME_CULL_NONE state.");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::WIREFRAME_CULL_NONE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create WIREFRAME_CULL_NONE state.");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::WIREFRAME_CULL_BACK)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create WIREFRAME_CULL_BACK state.");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::SOLID_CULL_BACK)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create SOLID_CULL_BACK state.");
	}

	m_deviceContext->RSSetState(m_rasterizerState[(int)(m_rasterizerMode)]);
}

void Renderer::SetStatesIfChanged()
{
	// check if the desired blend mode is the same as current blend state
	if (m_currentBlendState != m_blendStates[(int)m_desiredBlendMode])
	{
		m_currentBlendState = m_blendStates[(int)m_desiredBlendMode];

		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
		UINT sampleMask = 0xffffffff;
		m_deviceContext->OMSetBlendState(m_currentBlendState, blendFactor, sampleMask);
	}

	// set the depth mode if is changed
	if (m_desiredDepthMode != m_depthMode)
	{
		UINT stencilRef = 0xffffffff;
		m_deviceContext->OMSetDepthStencilState(m_depthStencilStates[(int)m_desiredDepthMode], stencilRef);
		m_depthMode = m_desiredDepthMode;
	}
	
	// sampler state
	if (m_currentSamplerState != m_samplerStates[(int)m_desiredSamplerMode])
	{
		m_deviceContext->PSSetSamplers(0, 1, &m_samplerStates[(int)m_desiredSamplerMode]);
		m_currentSamplerState = m_samplerStates[(int)m_desiredSamplerMode];
	}

	// rasterizer mode
	if (m_desiredRasterizerMode != m_rasterizerMode)
	{
		m_deviceContext->RSSetState(m_rasterizerState[(int)m_desiredRasterizerMode]);
		m_rasterizerMode = m_desiredRasterizerMode;
	}
}

void Renderer::CreateAllBlendStates()
{
	// create blend state for opaque rendering
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
	blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
	blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HRESULT hr;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::OPAQUE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::OPAQUE failed");
	}

	// create alpha state for opaque rendering
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ALPHA)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::ALPHA failed");
	}

	// create alpha state for additive rendering
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ADDITIVE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::ADDITIVE failed");
	}
}

// used when rendering in openGL environment
Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
	// Check if the load was successful
	GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name));
	GUARANTEE_OR_DIE(bytesPerTexel >= 3 && bytesPerTexel <= 4, Stringf("CreateTextureFromData failed for \"%s\" - unsupported BPP=%i (must be 3 or 4)", name, bytesPerTexel));
	GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0, Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name, dimensions.x, dimensions.y));

	Texture* newTexture = new Texture();
	newTexture->m_name = name; // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
	newTexture->m_dimensions = dimensions;

	//// Enable OpenGL texturing
	//glEnable(GL_TEXTURE_2D);

	//// Tell OpenGL that our pixel data is single-byte aligned
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//// Ask OpenGL for an unused texName (ID number) to use for this texture
	//glGenTextures(1, (GLuint*)&newTexture->m_textureID);

	//// Tell OpenGL to bind (set) this as the currently active texture
	//glBindTexture(GL_TEXTURE_2D, newTexture->m_textureID);

	//// Set texture clamp vs. wrap (repeat) default settings
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP or GL_REPEAT
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP or GL_REPEAT

	//// Set magnification (texel > pixel) and minification (texel < pixel) filters
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR

	//// Pick the appropriate OpenGL format (RGB or RGBA) for this texel data
	//GLenum bufferFormat = GL_RGBA; // the format our source pixel data is in; any of: GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, ...
	//if (bytesPerTexel == 3)
	//{
	//	bufferFormat = GL_RGB;
	//}
	//GLenum internalFormat = bufferFormat; // the format we want the texture to be on the card; technically allows us to translate into a different texture format as we upload to OpenGL

	//// Upload the image texel data (raw pixels bytes) to OpenGL under this textureID
	//glTexImage2D(			// Upload this pixel data to our new OpenGL texture
	//	GL_TEXTURE_2D,		// Creating this as a 2d texture
	//	0,					// Which mipmap level to use as the "root" (0 = the highest-quality, full-res image), if mipmaps are enabled
	//	internalFormat,		// Type of texel format we want OpenGL to use for this texture internally on the video card
	//	dimensions.x,		// Texel-width of image; for maximum compatibility, use 2^N + 2^B, where N is some integer in the range [3,11], and B is the border thickness [0,1]
	//	dimensions.y,		// Texel-height of image; for maximum compatibility, use 2^M + 2^B, where M is some integer in the range [3,11], and B is the border thickness [0,1]
	//	0,					// Border size, in texel (must be 0 or 1, recommend 0)
	//	bufferFormat,		// Pixel format describing the composition of the pixel data in buffer
	//	GL_UNSIGNED_BYTE,	// Pixel color components are unsigned bytes (one byte per color channel/component)
	//	texelData);		// Address of the actual pixel data bytes/buffer in system memory

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

void Renderer::BindTexture(Texture const* texture)
{
	// if the texture does not exist, bind it to default texture
	if (!texture)
	{
		//glEnable(GL_TEXTURE_2D);
		//glBindTexture(GL_TEXTURE_2D, texture->m_textureID);
		m_currentTexture = m_defaultTexture;
		m_deviceContext->PSSetShaderResources(0, 1, &m_currentTexture->m_shaderResourceView);
	}
	// if the binding texture do exist
	else 
	{
		// if the new binding texture is the same, no need to bind it again
		if (texture == m_currentTexture)
		{
			return;
		}
		else // new texture don't exist, switch to new texture
		{
			//glDisable(GL_TEXTURE_2D);
			m_currentTexture = texture;
			m_deviceContext->PSSetShaderResources(0, 1, &m_currentTexture->m_shaderResourceView);
		}
	}
}

void Renderer::CreateDefaultTexture()
{
	Image* defaultImage = new Image(IntVec2(2, 2), Rgba8::WHITE);
	m_defaultTexture = CreateTextureFromImage(*defaultImage);
	m_loadedTextures.push_back(const_cast<Texture*>(m_defaultTexture));
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//std::vector<std::string> Renderer::GetInstanceExtensions() const
//{
//	return { "XR_KHR_D3D11_enable" };
//}
//
//void Renderer::InitializeDeviceForOpenXR(XrInstance instance, XrSystemId systemId)
//{
//	PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR = nullptr;
//	CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR",
//		reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D11GraphicsRequirementsKHR)));
//
//	// Create the D3D11 device for the adapter associated with the system.
//	XrGraphicsRequirementsD3D11KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
//	CHECK_XRCMD(pfnGetD3D11GraphicsRequirementsKHR(instance, systemId, &graphicsRequirements));
//	const ComPtr<IDXGIAdapter1> adapter = GetAdapter(graphicsRequirements.adapterLuid);
//
//	// Create a list of feature levels which are both supported by the OpenXR runtime and this application.
//	std::vector<D3D_FEATURE_LEVEL> featureLevels = { D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
//													D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
//	featureLevels.erase(std::remove_if(featureLevels.begin(), featureLevels.end(),
//		[&](D3D_FEATURE_LEVEL fl) { return fl < graphicsRequirements.minFeatureLevel; }),
//		featureLevels.end());
//	CHECK_MSG(featureLevels.size() != 0, "Unsupported minimum feature level!");
//
//	InitializeD3D11DeviceForAdapter(adapter.Get(), featureLevels, m_device.ReleaseAndGetAddressOf(),
//		m_deviceContext.ReleaseAndGetAddressOf());
//
//	InitializeResources();
//
//	m_graphicsBinding.device = m_device.Get();
//}

