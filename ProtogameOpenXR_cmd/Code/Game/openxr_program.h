#pragma once
#include "Engine/Input/XRInputSystem.hpp"

struct Options;
struct IPlatformPlugin;
struct IGraphicsPlugin;

enum TextureID
{
	TESTUV,
	VICTORY_MENU,
	NUM_TEXTURES
};

class OpenXrProgram 
{
public:
    OpenXrProgram(const std::shared_ptr<Options>& options, const std::shared_ptr<IPlatformPlugin>& platformPlugin);

    ~OpenXrProgram();

    // Create an Instance and other basic instance-level initialization.
    void CreateInstance();

    // Select a System for the view configuration specified in the Options
    void InitializeSystem();

    // Initialize the graphics device for the selected system.
    void InitializeDevice();

    // Create a Session and other basic session-level initialization.
    void InitializeSession();

    // Create a Swapchain which requires coordinating with the graphics plugin to select the format, getting the system graphics
    // properties, getting the view configuration and grabbing the resulting swapchain images.
    void CreateSwapchains();

    // Process any events in the event queue.
    void PollEvents(bool* exitRenderLoop, bool* requestRestart);

    // Manage session lifecycle to track if RenderFrame should be called.
    bool IsSessionRunning() const;

    // Manage session state to track if input should be processed.
    bool IsSessionFocused() const;

    // Sample input actions and generate haptic feedback.
    void PollActions();

    // Create and submit a frame.
    void RenderFrame();

	void Startup();
	void RunFrame();
	void BeginFrame();
	void Update();

	// log info
	void LogViewConfigurations();
	void LogInstanceInfo();
	void LogActionSourceName(XrAction action, const std::string& actionName) const;
	void LogReferenceSpaces();
	void LogEnvironmentBlendMode(XrViewConfigurationType type);

	// input
	void InitializeActions();
	void InitializeGameStartActions();

	void CreateVisualizedSpaces();
	void CreateInstanceInternal();

	XrEventDataBaseHeader const* TryReadNextEvent();

	void HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent, bool* exitRenderLoop, bool* requestRestart);

	bool RenderLayer(XrTime predictedDisplayTime, std::vector<XrCompositionLayerProjectionView>& projectionLayerViews,
		XrCompositionLayerProjection& layer);

    // Get preferred blend mode based on the view configuration specified in the Options
    XrEnvironmentBlendMode GetPreferredBlendMode();

	// Function to create a quaternion from an axis and an angle
	XrQuaternionf createQuaternionFromAxisAngle(float x, float y, float z);


private:
	const std::shared_ptr<const Options> m_options;
	std::shared_ptr<IPlatformPlugin> m_platformPlugin;
	std::shared_ptr<IGraphicsPlugin> m_graphicsPlugin;
	XrInstance m_instance{ XR_NULL_HANDLE };
	XrSession m_session{ XR_NULL_HANDLE };
	XrSpace m_appSpace{ XR_NULL_HANDLE };
	XrSystemId m_systemId{ XR_NULL_SYSTEM_ID };

	// std::vector<XrViewConfigurationView> m_configViews;
	// std::vector<Swapchain> m_swapchains;
	// std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> m_swapchainImages;
	// std::vector<XrView> m_views;
	// int64_t m_colorSwapchainFormat{ -1 };

	// std::vector<XrSpace> m_visualizedSpaces;
	XrSpace m_worldSpace;
	XrSpace m_viewSpace;

	// Application's current lifecycle state according to the runtime
	XrSessionState m_sessionState{ XR_SESSION_STATE_UNKNOWN };
	bool m_sessionRunning{ false };

	XrEventDataBuffer m_eventDataBuffer;
	InputState m_input;

	const std::set<XrEnvironmentBlendMode> m_acceptableBlendModes;
};



