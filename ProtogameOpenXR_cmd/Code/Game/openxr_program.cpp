// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0
// Modified by Chengxiang Li

#include "Engine/core/Clock.hpp"
#include "Engine/core/StringUtils.hpp"
#include "ThirdParty/OpenXR/include/openxr/openxr.h"
#include "ThirdParty/OpenXR/src/common/xr_linear.h"
#include "pch.h"
#include "common.h"
#include "options.h"
#include "platformdata.h"
#include "platformplugin.h"
#include "Game/graphicsplugin_d3d11.hpp"
#include "Game/GameCommon.hpp"
#include "openxr_program.h"
#include "Game/Game.hpp"
#include "Game/HUD.hpp"
#include <array>
#include <cmath>
#include <set>

Game* g_theGame = nullptr;
XRRenderer* g_theXRRenderer = nullptr;
AudioSystem* g_theAudio = nullptr;
BitmapFont*	g_consoleFont = nullptr;
extern HUD* g_theHUD;

    #if !defined(XR_USE_PLATFORM_WIN32)
    #define strcpy_s(dest, source) strncpy((dest), (source), sizeof(dest))
    #endif

    inline std::string GetXrVersionString(XrVersion ver) {
        return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
    }

    namespace Math {
    namespace Pose {
        XrPosef Identity() 
        {
            XrPosef t{};
            t.orientation.w = 1;
            return t;
        }

        XrPosef Translation(const XrVector3f& translation) 
        {
            XrPosef t = Identity();
            t.position = translation;
            return t;
        }

        XrPosef RotateCCWAboutYAxis(float radians, XrVector3f translation) 
        {
            XrPosef t = Identity();
            t.orientation.x = 0.f;
            t.orientation.y = std::sin(radians * 0.5f);
            t.orientation.z = 0.f;
            t.orientation.w = std::cos(radians * 0.5f);
            t.position = translation;
            return t;
        }

		XrPosef RotateCCWAboutZAxis(XrVector3f translation, float z)
		{
			float halfAngle = 3.1415926f * 0.5f * z;			

			XrPosef t = Identity();
			t.orientation.x = 0.f;
			t.orientation.y = 0.f;
			t.orientation.z = std::sin(halfAngle);
			t.orientation.w = std::cos(halfAngle);
			t.position = translation;
			return t;
		}
    }  // namespace Pose
    }  // namespace Math

    inline XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(const std::string& referenceSpaceTypeStr) 
    {
        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Identity();
        if (EqualsIgnoreCase(referenceSpaceTypeStr, "View")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "ViewFront")) {
            // Render head-locked 2m in front of device.
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Translation({0.f, 0.f, -2.f}),
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Local")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Stage")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeft")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {-2.f, 0.f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRight")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {2.f, 0.f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeftRotated")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(3.14f / 3.f, {-2.f, 0.5f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRightRotated")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(-3.14f / 3.f, {2.f, 0.5f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else {
            throw std::invalid_argument(Fmt("Unknown reference space type '%s'", referenceSpaceTypeStr.c_str()));
        }
        return referenceSpaceCreateInfo;
    }


        OpenXrProgram::OpenXrProgram(const std::shared_ptr<Options>& options, const std::shared_ptr<IPlatformPlugin>& platformPlugin)
            : m_options(options),
              m_platformPlugin(platformPlugin),
              m_acceptableBlendModes{XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE,
                                     XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND} 
        {
            g_theXRRenderer = new XRRenderer(options, platformPlugin);
            g_theXRRenderer->UpdateOptions(options);
        }

        OpenXrProgram::~OpenXrProgram()
        {
            if (m_input.actionSet != XR_NULL_HANDLE) {
                for (auto hand : {Side::LEFT, Side::RIGHT}) {
                    xrDestroySpace(m_input.handSpace[hand]);
                }
                xrDestroyActionSet(m_input.actionSet);
            }

            for (Swapchain swapchain : g_theXRRenderer->m_swapchains) 
            {
                xrDestroySwapchain(swapchain.handle);
            }

            xrDestroySpace(m_worldSpace);

            xrDestroySpace(m_viewSpace);	    

            if (m_appSpace != XR_NULL_HANDLE) 
            {
                xrDestroySpace(m_appSpace);
            }

            if (m_session != XR_NULL_HANDLE) 
            {
                xrDestroySession(m_session);
            }

            if (m_instance != XR_NULL_HANDLE) 
            {
                xrDestroyInstance(m_instance);
            }
        }

        static void LogLayersAndExtensions() 
        {
            // Write out extension properties for a given layer.
            const auto logExtensions = [](const char* layerName, int indent = 0) {
                uint32_t instanceExtensionCount;
                CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));
                std::vector<XrExtensionProperties> extensions(instanceExtensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
                CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, (uint32_t)extensions.size(), &instanceExtensionCount,
                                                                   extensions.data()));

                const std::string indentStr(indent, ' ');
                Log::Write(Log::Level::Verbose, Fmt("%sAvailable Extensions: (%d)", indentStr.c_str(), instanceExtensionCount));
                for (const XrExtensionProperties& extension : extensions) {
                    Log::Write(Log::Level::Verbose, Fmt("%s  Name=%s SpecVersion=%d", indentStr.c_str(), extension.extensionName,
                                                        extension.extensionVersion));
                }
            };

            // Log non-layer extensions (layerName==nullptr).
            logExtensions(nullptr);

            // Log layers and any of their extensions.
            {
                uint32_t layerCount;
                CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));
                std::vector<XrApiLayerProperties> layers(layerCount, {XR_TYPE_API_LAYER_PROPERTIES});
                CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data()));

                Log::Write(Log::Level::Info, Fmt("Available Layers: (%d)", layerCount));
                for (const XrApiLayerProperties& layer : layers) {
                    Log::Write(Log::Level::Verbose,
                               Fmt("  Name=%s SpecVersion=%s LayerVersion=%d Description=%s", layer.layerName,
                                   GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description));
                    logExtensions(layer.layerName, 4);
                }
            }
        }

        void OpenXrProgram::LogInstanceInfo()
        {
            CHECK(m_instance != XR_NULL_HANDLE);

            XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
            CHECK_XRCMD(xrGetInstanceProperties(m_instance, &instanceProperties));

            Log::Write(Log::Level::Info, Fmt("Instance RuntimeName=%s RuntimeVersion=%s", instanceProperties.runtimeName,
                                             GetXrVersionString(instanceProperties.runtimeVersion).c_str()));
        }

        void OpenXrProgram::CreateInstanceInternal()
        {
            CHECK(m_instance == XR_NULL_HANDLE);

            // Create union of extensions required by platform and graphics plugins.
            std::vector<const char*> extensions;

            // Transform platform and graphics extension std::strings to C strings.
            const std::vector<std::string> platformExtensions = m_platformPlugin->GetInstanceExtensions();
            std::transform(platformExtensions.begin(), platformExtensions.end(), std::back_inserter(extensions),
                           [](const std::string& ext) { return ext.c_str(); });
            const std::vector<std::string> graphicsExtensions = g_theXRRenderer->GetInstanceExtensions();
            std::transform(graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(extensions),
                           [](const std::string& ext) { return ext.c_str(); });

            XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
            createInfo.next = m_platformPlugin->GetInstanceCreateExtension();
            createInfo.enabledExtensionCount = (uint32_t)extensions.size();
            createInfo.enabledExtensionNames = extensions.data();

            strncpy_s(createInfo.applicationInfo.applicationName, "HelloXR", 7);

            // Current version is 1.1.x, but hello_xr only requires 1.0.x
            createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;

            CHECK_XRCMD(xrCreateInstance(&createInfo, &m_instance));
        }

        void OpenXrProgram::CreateInstance()
        {
            LogLayersAndExtensions();

            CreateInstanceInternal();

            LogInstanceInfo();
        }

        void OpenXrProgram::LogViewConfigurations()
        {
            CHECK(m_instance != XR_NULL_HANDLE);
            CHECK(m_systemId != XR_NULL_SYSTEM_ID);

            uint32_t viewConfigTypeCount;
            CHECK_XRCMD(xrEnumerateViewConfigurations(m_instance, m_systemId, 0, &viewConfigTypeCount, nullptr));
            std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
            CHECK_XRCMD(xrEnumerateViewConfigurations(m_instance, m_systemId, viewConfigTypeCount, &viewConfigTypeCount,
                                                      viewConfigTypes.data()));
            CHECK((uint32_t)viewConfigTypes.size() == viewConfigTypeCount);

            Log::Write(Log::Level::Info, Fmt("Available View Configuration Types: (%d)", viewConfigTypeCount));
            for (XrViewConfigurationType viewConfigType : viewConfigTypes) {
                Log::Write(Log::Level::Verbose, Fmt("  View Configuration Type: %s %s", to_string(viewConfigType),
                                                    viewConfigType == m_options->Parsed.ViewConfigType ? "(Selected)" : ""));

                XrViewConfigurationProperties viewConfigProperties{XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
                CHECK_XRCMD(xrGetViewConfigurationProperties(m_instance, m_systemId, viewConfigType, &viewConfigProperties));

                Log::Write(Log::Level::Verbose,
                           Fmt("  View configuration FovMutable=%s", viewConfigProperties.fovMutable == XR_TRUE ? "True" : "False"));

                uint32_t viewCount;
                CHECK_XRCMD(xrEnumerateViewConfigurationViews(m_instance, m_systemId, viewConfigType, 0, &viewCount, nullptr));
                if (viewCount > 0) {
                    std::vector<XrViewConfigurationView> views(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
                    CHECK_XRCMD(
                        xrEnumerateViewConfigurationViews(m_instance, m_systemId, viewConfigType, viewCount, &viewCount, views.data()));

                    for (uint32_t i = 0; i < views.size(); i++) {
                        const XrViewConfigurationView& view = views[i];

                        Log::Write(Log::Level::Verbose, Fmt("    View [%d]: Recommended Width=%d Height=%d SampleCount=%d", i,
                                                            view.recommendedImageRectWidth, view.recommendedImageRectHeight,
                                                            view.recommendedSwapchainSampleCount));
                        Log::Write(Log::Level::Verbose,
                                   Fmt("    View [%d]:     Maximum Width=%d Height=%d SampleCount=%d", i, view.maxImageRectWidth,
                                       view.maxImageRectHeight, view.maxSwapchainSampleCount));
                    }
                } else {
                    Log::Write(Log::Level::Error, Fmt("Empty view configuration type"));
                }

                LogEnvironmentBlendMode(viewConfigType);
            }
        }

        void OpenXrProgram::LogEnvironmentBlendMode(XrViewConfigurationType type)
        {
            CHECK(m_instance != XR_NULL_HANDLE);
            CHECK(m_systemId != 0);

            uint32_t count;
            CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, type, 0, &count, nullptr));
            CHECK(count > 0);

            Log::Write(Log::Level::Info, Fmt("Available Environment Blend Mode count : (%d)", count));

            std::vector<XrEnvironmentBlendMode> blendModes(count);
            CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, type, count, &count, blendModes.data()));

            bool blendModeFound = false;
            for (XrEnvironmentBlendMode mode : blendModes) {
                const bool blendModeMatch = (mode == m_options->Parsed.EnvironmentBlendMode);
                Log::Write(Log::Level::Info,
                           Fmt("Environment Blend Mode (%s) : %s", to_string(mode), blendModeMatch ? "(Selected)" : ""));
                blendModeFound |= blendModeMatch;
            }
            CHECK(blendModeFound);
        }

        void OpenXrProgram::InitializeSystem()
        {
            CHECK(m_instance != XR_NULL_HANDLE);
            CHECK(m_systemId == XR_NULL_SYSTEM_ID);

            XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
            systemInfo.formFactor = m_options->Parsed.FormFactor;
            CHECK_XRCMD(xrGetSystem(m_instance, &systemInfo, &m_systemId));

            Log::Write(Log::Level::Verbose,
                       Fmt("Using system %d for form factor %s", m_systemId, to_string(m_options->Parsed.FormFactor)));
            CHECK(m_instance != XR_NULL_HANDLE);
            CHECK(m_systemId != XR_NULL_SYSTEM_ID);
        }

        void OpenXrProgram::InitializeDevice()
        {
            LogViewConfigurations();

            // The graphics API can initialize the graphics device now that the systemId and instance
            // handle are available.
            g_theXRRenderer->InitializeDevice(m_instance, m_systemId);
        }

        void OpenXrProgram::LogReferenceSpaces()
        {
            CHECK(m_session != XR_NULL_HANDLE);

            uint32_t spaceCount;
            CHECK_XRCMD(xrEnumerateReferenceSpaces(m_session, 0, &spaceCount, nullptr));
            std::vector<XrReferenceSpaceType> spaces(spaceCount);
            CHECK_XRCMD(xrEnumerateReferenceSpaces(m_session, spaceCount, &spaceCount, spaces.data()));

            Log::Write(Log::Level::Info, Fmt("Available reference spaces: %d", spaceCount));
            for (XrReferenceSpaceType space : spaces) 
            {
                Log::Write(Log::Level::Info, Fmt("  Name: %s", to_string(space)));
            }

        }

		void OpenXrProgram::InitializeGameStartActions()
		{
			// Create an action set.
			{
				XrActionSetCreateInfo actionSetInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
				strncpy_s(actionSetInfo.actionSetName, "gameStart", 8);
				strncpy_s(actionSetInfo.localizedActionSetName, "GameStart", 8);
				actionSetInfo.priority = 0;
				CHECK_XRCMD(xrCreateActionSet(m_instance, &actionSetInfo, &m_input.actionSet));
			}

			// Get the XrPath for the left and right hands - we will use them as subaction paths.
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left", &m_input.handSubactionPath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right", &m_input.handSubactionPath[Side::RIGHT]));

			// Create actions.
			{
				// Create an input action for squeeze both hands to start the game
				XrActionCreateInfo actionInfo{ XR_TYPE_ACTION_CREATE_INFO };
				actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
				strncpy_s(actionInfo.actionName, "start_game", 10);
				strncpy_s(actionInfo.localizedActionName, "Start Game", 10);
				actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
				actionInfo.subactionPaths = m_input.handSubactionPath.data();
				CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.grabAction));

				// Create an input action getting the left and right hand poses.
				actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
				strncpy_s(actionInfo.actionName, "hand_pose", 9);
				strncpy_s(actionInfo.localizedActionName, "Hand Pose", 9);
				actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
				actionInfo.subactionPaths = m_input.handSubactionPath.data();
				CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.poseAction));

				// Create output actions for vibrating the left and right controller.
				actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
				strncpy_s(actionInfo.actionName, "vibrate_hand", 12);
				strncpy_s(actionInfo.localizedActionName, "Vibrate Hand", 12);
				actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
				actionInfo.subactionPaths = m_input.handSubactionPath.data();
				CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.vibrateAction));

				// Create input actions for quitting the session using the left and right controller.
				// Since it doesn't matter which hand did this, we do not specify subaction paths for it.
				// We will just suggest bindings for both hands, where possible.
				actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
				strncpy_s(actionInfo.actionName, "quit_session", 16);
				strncpy_s(actionInfo.localizedActionName, "Quit Session", 16);
				actionInfo.countSubactionPaths = 0;
				actionInfo.subactionPaths = nullptr;
				CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.quitAction));
			}

			std::array<XrPath, Side::COUNT> selectPath;
			std::array<XrPath, Side::COUNT> squeezeValuePath;
			std::array<XrPath, Side::COUNT> squeezeForcePath;
			std::array<XrPath, Side::COUNT> squeezeClickPath;
			std::array<XrPath, Side::COUNT> posePath;
			std::array<XrPath, Side::COUNT> hapticPath;
			std::array<XrPath, Side::COUNT> menuClickPath;
			std::array<XrPath, Side::COUNT> bClickPath;
			std::array<XrPath, Side::COUNT> triggerValuePath;
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/select/click", &selectPath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/select/click", &selectPath[Side::RIGHT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/value", &squeezeValuePath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &squeezeValuePath[Side::RIGHT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/force", &squeezeForcePath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/force", &squeezeForcePath[Side::RIGHT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/click", &squeezeClickPath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/click", &squeezeClickPath[Side::RIGHT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &posePath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &posePath[Side::RIGHT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/output/haptic", &hapticPath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/output/haptic", &hapticPath[Side::RIGHT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/menu/click", &menuClickPath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/menu/click", &menuClickPath[Side::RIGHT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/b/click", &bClickPath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/b/click", &bClickPath[Side::RIGHT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &triggerValuePath[Side::LEFT]));
			CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &triggerValuePath[Side::RIGHT]));
			// Suggest bindings for KHR Simple.
			{
				XrPath khrSimpleInteractionProfilePath;
				CHECK_XRCMD(
					xrStringToPath(m_instance, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath));
				std::vector<XrActionSuggestedBinding> bindings{ {// Fall back to a click input for the grab action.
																{m_input.grabAction, selectPath[Side::LEFT]},
																{m_input.grabAction, selectPath[Side::RIGHT]},
																{m_input.poseAction, posePath[Side::LEFT]},
																{m_input.poseAction, posePath[Side::RIGHT]},
																{m_input.quitAction, menuClickPath[Side::LEFT]},
																{m_input.quitAction, menuClickPath[Side::RIGHT]},
																{m_input.vibrateAction, hapticPath[Side::LEFT]},
																{m_input.vibrateAction, hapticPath[Side::RIGHT]}} };
				XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
				suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
				suggestedBindings.suggestedBindings = bindings.data();
				suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
				CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
			}
			// Suggest bindings for the Oculus Touch.
			{
				XrPath oculusTouchInteractionProfilePath;
				CHECK_XRCMD(
					xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &oculusTouchInteractionProfilePath));
				std::vector<XrActionSuggestedBinding> bindings{ {{m_input.grabAction, squeezeValuePath[Side::LEFT]},
																{m_input.grabAction, squeezeValuePath[Side::RIGHT]},
																{m_input.poseAction, posePath[Side::LEFT]},
																{m_input.poseAction, posePath[Side::RIGHT]},
																{m_input.quitAction, menuClickPath[Side::LEFT]},
																{m_input.vibrateAction, hapticPath[Side::LEFT]},
																{m_input.vibrateAction, hapticPath[Side::RIGHT]}} };
				XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
				suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
				suggestedBindings.suggestedBindings = bindings.data();
				suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
				CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
			}
			// Suggest bindings for the Vive Controller.
			{
				XrPath viveControllerInteractionProfilePath;
				CHECK_XRCMD(
					xrStringToPath(m_instance, "/interaction_profiles/htc/vive_controller", &viveControllerInteractionProfilePath));
				std::vector<XrActionSuggestedBinding> bindings{ {{m_input.grabAction, triggerValuePath[Side::LEFT]},
																{m_input.grabAction, triggerValuePath[Side::RIGHT]},
																{m_input.poseAction, posePath[Side::LEFT]},
																{m_input.poseAction, posePath[Side::RIGHT]},
																{m_input.quitAction, menuClickPath[Side::LEFT]},
																{m_input.quitAction, menuClickPath[Side::RIGHT]},
																{m_input.vibrateAction, hapticPath[Side::LEFT]},
																{m_input.vibrateAction, hapticPath[Side::RIGHT]}} };
				XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
				suggestedBindings.interactionProfile = viveControllerInteractionProfilePath;
				suggestedBindings.suggestedBindings = bindings.data();
				suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
				CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
			}

			// Suggest bindings for the Valve Index Controller.
			{
				XrPath indexControllerInteractionProfilePath;
				CHECK_XRCMD(
					xrStringToPath(m_instance, "/interaction_profiles/valve/index_controller", &indexControllerInteractionProfilePath));
				std::vector<XrActionSuggestedBinding> bindings{ {{m_input.grabAction, squeezeForcePath[Side::LEFT]},
																{m_input.grabAction, squeezeForcePath[Side::RIGHT]},
																{m_input.poseAction, posePath[Side::LEFT]},
																{m_input.poseAction, posePath[Side::RIGHT]},
																{m_input.quitAction, bClickPath[Side::LEFT]},
																{m_input.quitAction, bClickPath[Side::RIGHT]},
																{m_input.vibrateAction, hapticPath[Side::LEFT]},
																{m_input.vibrateAction, hapticPath[Side::RIGHT]}} };
				XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
				suggestedBindings.interactionProfile = indexControllerInteractionProfilePath;
				suggestedBindings.suggestedBindings = bindings.data();
				suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
				CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
			}

			// Suggest bindings for the Microsoft Mixed Reality Motion Controller.
			{
				XrPath microsoftMixedRealityInteractionProfilePath;
				CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/microsoft/motion_controller",
					&microsoftMixedRealityInteractionProfilePath));
				std::vector<XrActionSuggestedBinding> bindings{ {{m_input.grabAction, squeezeClickPath[Side::LEFT]},
																{m_input.grabAction, squeezeClickPath[Side::RIGHT]},
																{m_input.poseAction, posePath[Side::LEFT]},
																{m_input.poseAction, posePath[Side::RIGHT]},
																{m_input.quitAction, menuClickPath[Side::LEFT]},
																{m_input.quitAction, menuClickPath[Side::RIGHT]},
																{m_input.vibrateAction, hapticPath[Side::LEFT]},
																{m_input.vibrateAction, hapticPath[Side::RIGHT]}} };
				XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
				suggestedBindings.interactionProfile = microsoftMixedRealityInteractionProfilePath;
				suggestedBindings.suggestedBindings = bindings.data();
				suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
				CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
			}

			XrActionSpaceCreateInfo actionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
			actionSpaceInfo.action = m_input.poseAction;
			actionSpaceInfo.poseInActionSpace = Math::Pose::Identity();
			actionSpaceInfo.subactionPath = m_input.handSubactionPath[Side::LEFT];
			CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.handSpace[Side::LEFT]));
			actionSpaceInfo.subactionPath = m_input.handSubactionPath[Side::RIGHT];
			CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.handSpace[Side::RIGHT]));

			XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
			attachInfo.countActionSets = 1;
			attachInfo.actionSets = &m_input.actionSet;
			CHECK_XRCMD(xrAttachSessionActionSets(m_session, &attachInfo));
		}

        void OpenXrProgram::InitializeActions()
        {
            // Create an action set.
            {
                XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
                strncpy_s(actionSetInfo.actionSetName, "gameplay", 8);
                strncpy_s(actionSetInfo.localizedActionSetName, "Gameplay", 8);
                actionSetInfo.priority = 0;
                CHECK_XRCMD(xrCreateActionSet(m_instance, &actionSetInfo, &m_input.actionSet));
            }

            // Get the XrPath for the left and right hands - we will use them as subaction paths.
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left", &m_input.handSubactionPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right", &m_input.handSubactionPath[Side::RIGHT]));

            // Create actions.
            {
                // Create an input action for grabbing objects with the left and right hands.
                XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
                actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
                strncpy_s(actionInfo.actionName, "grab_object", 11);
                strncpy_s(actionInfo.localizedActionName, "Grab Object", 11);
                actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
                actionInfo.subactionPaths = m_input.handSubactionPath.data();
                CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.grabAction));

                // Create an input action getting the left and right hand poses.
                actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
                strncpy_s(actionInfo.actionName, "hand_pose", 9);
                strncpy_s(actionInfo.localizedActionName, "Hand Pose", 9);
                actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
                actionInfo.subactionPaths = m_input.handSubactionPath.data();
                CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.poseAction));

                // Create output actions for vibrating the left and right controller.
                actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
                strncpy_s(actionInfo.actionName, "vibrate_hand", 12);
                strncpy_s(actionInfo.localizedActionName, "Vibrate Hand", 12);
                actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
                actionInfo.subactionPaths = m_input.handSubactionPath.data();
                CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.vibrateAction));

                // Create input actions for quitting the session using the left and right controller.
                // Since it doesn't matter which hand did this, we do not specify subaction paths for it.
                // We will just suggest bindings for both hands, where possible.
                actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
                strncpy_s(actionInfo.actionName, "quit_session", 16);
                strncpy_s(actionInfo.localizedActionName, "Quit Session", 16);
                actionInfo.countSubactionPaths = 0;
                actionInfo.subactionPaths = nullptr;
                CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.quitAction));
            }

            std::array<XrPath, Side::COUNT> selectPath;
            std::array<XrPath, Side::COUNT> squeezeValuePath;
            std::array<XrPath, Side::COUNT> squeezeForcePath;
            std::array<XrPath, Side::COUNT> squeezeClickPath;
            std::array<XrPath, Side::COUNT> posePath;
            std::array<XrPath, Side::COUNT> hapticPath;
            std::array<XrPath, Side::COUNT> menuClickPath;
            std::array<XrPath, Side::COUNT> bClickPath;
            std::array<XrPath, Side::COUNT> triggerValuePath;
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/select/click", &selectPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/select/click", &selectPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/value", &squeezeValuePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &squeezeValuePath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/force", &squeezeForcePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/force", &squeezeForcePath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/click", &squeezeClickPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/click", &squeezeClickPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &posePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &posePath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/output/haptic", &hapticPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/output/haptic", &hapticPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/menu/click", &menuClickPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/menu/click", &menuClickPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/b/click", &bClickPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/b/click", &bClickPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &triggerValuePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &triggerValuePath[Side::RIGHT]));
            // Suggest bindings for KHR Simple.
            {
                XrPath khrSimpleInteractionProfilePath;
                CHECK_XRCMD(
                    xrStringToPath(m_instance, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{{// Fall back to a click input for the grab action.
                                                                {m_input.grabAction, selectPath[Side::LEFT]},
                                                                {m_input.grabAction, selectPath[Side::RIGHT]},
                                                                {m_input.poseAction, posePath[Side::LEFT]},
                                                                {m_input.poseAction, posePath[Side::RIGHT]},
                                                                {m_input.quitAction, menuClickPath[Side::LEFT]},
                                                                {m_input.quitAction, menuClickPath[Side::RIGHT]},
                                                                {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                                {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
                CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
            }
            // Suggest bindings for the Oculus Touch.
            {
                XrPath oculusTouchInteractionProfilePath;
                CHECK_XRCMD(
                    xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &oculusTouchInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{{{m_input.grabAction, squeezeValuePath[Side::LEFT]},
                                                                {m_input.grabAction, squeezeValuePath[Side::RIGHT]},
                                                                {m_input.poseAction, posePath[Side::LEFT]},
                                                                {m_input.poseAction, posePath[Side::RIGHT]},
                                                                {m_input.quitAction, menuClickPath[Side::LEFT]},
                                                                {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                                {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
                CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
            }
            // Suggest bindings for the Vive Controller.
            {
                XrPath viveControllerInteractionProfilePath;
                CHECK_XRCMD(
                    xrStringToPath(m_instance, "/interaction_profiles/htc/vive_controller", &viveControllerInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{{{m_input.grabAction, triggerValuePath[Side::LEFT]},
                                                                {m_input.grabAction, triggerValuePath[Side::RIGHT]},
                                                                {m_input.poseAction, posePath[Side::LEFT]},
                                                                {m_input.poseAction, posePath[Side::RIGHT]},
                                                                {m_input.quitAction, menuClickPath[Side::LEFT]},
                                                                {m_input.quitAction, menuClickPath[Side::RIGHT]},
                                                                {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                                {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = viveControllerInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
                CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
            }

            // Suggest bindings for the Valve Index Controller.
            {
                XrPath indexControllerInteractionProfilePath;
                CHECK_XRCMD(
                    xrStringToPath(m_instance, "/interaction_profiles/valve/index_controller", &indexControllerInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{{{m_input.grabAction, squeezeForcePath[Side::LEFT]},
                                                                {m_input.grabAction, squeezeForcePath[Side::RIGHT]},
                                                                {m_input.poseAction, posePath[Side::LEFT]},
                                                                {m_input.poseAction, posePath[Side::RIGHT]},
                                                                {m_input.quitAction, bClickPath[Side::LEFT]},
                                                                {m_input.quitAction, bClickPath[Side::RIGHT]},
                                                                {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                                {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = indexControllerInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
                CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
            }

            // Suggest bindings for the Microsoft Mixed Reality Motion Controller.
            {
                XrPath microsoftMixedRealityInteractionProfilePath;
                CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/microsoft/motion_controller",
                                           &microsoftMixedRealityInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{{{m_input.grabAction, squeezeClickPath[Side::LEFT]},
                                                                {m_input.grabAction, squeezeClickPath[Side::RIGHT]},
                                                                {m_input.poseAction, posePath[Side::LEFT]},
                                                                {m_input.poseAction, posePath[Side::RIGHT]},
                                                                {m_input.quitAction, menuClickPath[Side::LEFT]},
                                                                {m_input.quitAction, menuClickPath[Side::RIGHT]},
                                                                {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                                {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = microsoftMixedRealityInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
                CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
            }
            XrActionSpaceCreateInfo actionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
            actionSpaceInfo.action = m_input.poseAction;
            actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
            actionSpaceInfo.subactionPath = m_input.handSubactionPath[Side::LEFT];
            CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.handSpace[Side::LEFT]));
            actionSpaceInfo.subactionPath = m_input.handSubactionPath[Side::RIGHT];
            CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.handSpace[Side::RIGHT]));

            XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
            attachInfo.countActionSets = 1;
            attachInfo.actionSets = &m_input.actionSet;
            CHECK_XRCMD(xrAttachSessionActionSets(m_session, &attachInfo));
        }

		void OpenXrProgram::CreateVisualizedSpaces()
        {
            CHECK(m_session != XR_NULL_HANDLE);

		    // Fill out an XrReferenceSpaceCreateInfo structure and create a reference XrSpace
            // specifying a Local space with an identity pose as the origin.
		    XrReferenceSpaceCreateInfo referenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
		    referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            referenceSpaceCI.poseInReferenceSpace = Math::Pose::RotateCCWAboutZAxis(Vec3(0.f, 0.f, -1.f).MakeXrVector3f(), -0.5f);
            // referenceSpaceCI.poseInReferenceSpace = Math::Pose::Identity();
            XrSpace space;
            XrResult res = xrCreateReferenceSpace(m_session, &referenceSpaceCI, &space);
		    if (XR_SUCCEEDED(res))
		    {
                m_worldSpace = space;
		    }
		    else
		    {
			    Log::Write(Log::Level::Error, "failed to create reference space");
		    }

			XrReferenceSpaceCreateInfo viewSpaceReferenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
			viewSpaceReferenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
			viewSpaceReferenceSpaceCI.poseInReferenceSpace = Math::Pose::Identity();
			XrSpace viewSpace;
			res = xrCreateReferenceSpace(m_session, &viewSpaceReferenceSpaceCI, &viewSpace);
			if (XR_SUCCEEDED(res))
			{
				m_viewSpace = viewSpace;
			}
			else
			{
				Log::Write(Log::Level::Error, "failed to create reference space");
			}
        }

        void OpenXrProgram::InitializeSession()
        {
            CHECK(m_instance != XR_NULL_HANDLE);
            CHECK(m_session == XR_NULL_HANDLE);

            {
                Log::Write(Log::Level::Verbose, Fmt("Creating session..."));

                XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
                createInfo.next = g_theXRRenderer->GetGraphicsBinding();
                createInfo.systemId = m_systemId;
                CHECK_XRCMD(xrCreateSession(m_instance, &createInfo, &m_session));
            }

            LogReferenceSpaces();
            InitializeActions();
            CreateVisualizedSpaces();

            { 
                XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(m_options->AppSpace);
                CHECK_XRCMD(xrCreateReferenceSpace(m_session, &referenceSpaceCreateInfo, &m_appSpace));
            }
        }

        //void OpenXrProgram::CreateSwapchains()
        //{
        //    CHECK(m_session != XR_NULL_HANDLE);
        //    CHECK(m_swapchains.empty());
        //    CHECK(m_configViews.empty());

        //    // Read graphics properties for preferred swapchain length and logging.
        //    XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
        //    CHECK_XRCMD(xrGetSystemProperties(m_instance, m_systemId, &systemProperties));

        //    // Log system properties.
        //    Log::Write(Log::Level::Info,
        //               Fmt("System Properties: Name=%s VendorId=%d", systemProperties.systemName, systemProperties.vendorId));
        //    Log::Write(Log::Level::Info, Fmt("System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d",
        //                                     systemProperties.graphicsProperties.maxSwapchainImageWidth,
        //                                     systemProperties.graphicsProperties.maxSwapchainImageHeight,
        //                                     systemProperties.graphicsProperties.maxLayerCount));
        //    Log::Write(Log::Level::Info, Fmt("System Tracking Properties: OrientationTracking=%s PositionTracking=%s",
        //                                     systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "True" : "False",
        //                                     systemProperties.trackingProperties.positionTracking == XR_TRUE ? "True" : "False"));

        //    // Note: No other view configurations exist at the time this code was written. If this
        //    // condition is not met, the project will need to be audited to see how support should be
        //    // added.
        //    CHECK_MSG(m_options->Parsed.ViewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        //              "Unsupported view configuration type");

        //    // Query and cache view configuration views.
        //    uint32_t viewCount;
        //    CHECK_XRCMD(
        //        xrEnumerateViewConfigurationViews(m_instance, m_systemId, m_options->Parsed.ViewConfigType, 0, &viewCount, nullptr));
        //    m_configViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        //    CHECK_XRCMD(xrEnumerateViewConfigurationViews(m_instance, m_systemId, m_options->Parsed.ViewConfigType, viewCount,
        //                                                  &viewCount, m_configViews.data()));

        //    // Create and cache view buffer for xrLocateViews later.
        //    m_views.resize(viewCount, {XR_TYPE_VIEW});

        //    // Create the swapchain and get the images.
        //    if (viewCount > 0) {
        //        // Select a swapchain format.
        //        uint32_t swapchainFormatCount;
        //        CHECK_XRCMD(xrEnumerateSwapchainFormats(m_session, 0, &swapchainFormatCount, nullptr));
        //        std::vector<int64_t> swapchainFormats(swapchainFormatCount);
        //        CHECK_XRCMD(xrEnumerateSwapchainFormats(m_session, (uint32_t)swapchainFormats.size(), &swapchainFormatCount,
        //                                                swapchainFormats.data()));
        //        CHECK(swapchainFormatCount == swapchainFormats.size());
        //        m_colorSwapchainFormat = g_theXRRenderer->SelectColorSwapchainFormat(swapchainFormats);

        //        // Print swapchain formats and the selected one.
        //        {
        //            std::string swapchainFormatsString;
        //            for (int64_t format : swapchainFormats) {
        //                const bool selected = format == m_colorSwapchainFormat;
        //                swapchainFormatsString += " ";
        //                if (selected) {
        //                    swapchainFormatsString += "[";
        //                }
        //                swapchainFormatsString += std::to_string(format);
        //                if (selected) {
        //                    swapchainFormatsString += "]";
        //                }
        //            }
        //            Log::Write(Log::Level::Verbose, Fmt("Swapchain Formats: %s", swapchainFormatsString.c_str()));
        //        }

        //        // Create a swapchain for each view.
        //        for (uint32_t i = 0; i < viewCount; i++) {
        //            const XrViewConfigurationView& vp = m_configViews[i];
        //            Log::Write(Log::Level::Info,
        //                       Fmt("Creating swapchain for view %d with dimensions Width=%d Height=%d SampleCount=%d", i,
        //                           vp.recommendedImageRectWidth, vp.recommendedImageRectHeight, vp.recommendedSwapchainSampleCount));

        //            // Create the swapchain.
        //            XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        //            swapchainCreateInfo.arraySize = 1;
        //            swapchainCreateInfo.format = m_colorSwapchainFormat;
        //            swapchainCreateInfo.width = vp.recommendedImageRectWidth;
        //            swapchainCreateInfo.height = vp.recommendedImageRectHeight;
        //            swapchainCreateInfo.mipCount = 1;
        //            swapchainCreateInfo.faceCount = 1;
        //            swapchainCreateInfo.sampleCount = g_theXRRenderer->GetSupportedSwapchainSampleCount(vp);
        //            swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        //            Swapchain swapchain;
        //            swapchain.width = swapchainCreateInfo.width;
        //            swapchain.height = swapchainCreateInfo.height;
        //            CHECK_XRCMD(xrCreateSwapchain(m_session, &swapchainCreateInfo, &swapchain.handle));

        //            m_swapchains.push_back(swapchain);

        //            uint32_t imageCount;
        //            CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr));
        //            // XXX This should really just return XrSwapchainImageBaseHeader*
        //            std::vector<XrSwapchainImageBaseHeader*> swapchainImages =
        //                g_theXRRenderer->AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);
        //            CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount, swapchainImages[0]));

        //            m_swapchainImages.insert(std::make_pair(swapchain.handle, std::move(swapchainImages)));
        //        }
        //    }
        //}

        void OpenXrProgram::Startup()
        {
			InitializeDevice();
			InitializeSession();
            g_theXRRenderer->CreateSwapchains(m_session, m_instance, m_systemId, m_options);

			AudioConfig audioConfig;
			g_theAudio = new AudioSystem(audioConfig);
			g_theAudio->Startup();

			g_theGame = new Game();
			g_theGame->Startup();
            g_theXRRenderer->Startup();

			g_consoleFont = g_theXRRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont.png");
        }

	    void OpenXrProgram::RunFrame()
	    {
            BeginFrame();
            Update();
		    RenderFrame();
	    }

        void OpenXrProgram::BeginFrame()
        {
            g_theAudio->BeginFrame();
        }

        void OpenXrProgram::Update()
        {
			Clock::GetSystemClock().TickSystemClock();

            PollActions();
            g_theGame->Update();
        }

		// Return event if one is available, otherwise return null.
        const XrEventDataBaseHeader* OpenXrProgram::TryReadNextEvent()
        {
            // It is sufficient to clear the just the XrEventDataBuffer header to
            // XR_TYPE_EVENT_DATA_BUFFER
            XrEventDataBaseHeader* baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&m_eventDataBuffer);
            *baseHeader = {XR_TYPE_EVENT_DATA_BUFFER};
            const XrResult xr = xrPollEvent(m_instance, &m_eventDataBuffer);
            if (xr == XR_SUCCESS) {
                if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
                    const XrEventDataEventsLost* const eventsLost = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
                    Log::Write(Log::Level::Warning, Fmt("%d events lost", eventsLost->lostEventCount));
                }

                return baseHeader;
            }
            if (xr == XR_EVENT_UNAVAILABLE) {
                return nullptr;
            }
            THROW_XR(xr, "xrPollEvent");
        }

        void OpenXrProgram::PollEvents(bool* exitRenderLoop, bool* requestRestart)
        {
            *exitRenderLoop = *requestRestart = false;

            // Process all pending messages.
            while (const XrEventDataBaseHeader* event = TryReadNextEvent()) {
                switch (event->type) {
                    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                        const auto& instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);
                        Log::Write(Log::Level::Warning, Fmt("XrEventDataInstanceLossPending by %lld", instanceLossPending.lossTime));
                        *exitRenderLoop = true;
                        *requestRestart = true;
                        return;
                    }
                    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                        auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
                        HandleSessionStateChangedEvent(sessionStateChangedEvent, exitRenderLoop, requestRestart);
                        break;
                    }
                    case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                        LogActionSourceName(m_input.grabAction, "Grab");
                        LogActionSourceName(m_input.quitAction, "Quit");
                        LogActionSourceName(m_input.poseAction, "Pose");
                        LogActionSourceName(m_input.vibrateAction, "Vibrate");
                        break;
                    case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                    default: {
                        Log::Write(Log::Level::Verbose, Fmt("Ignoring event type %d", event->type));
                        break;
                    }
                }
            }
        }

        void OpenXrProgram::HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent, bool* exitRenderLoop, bool* requestRestart)
        {
            const XrSessionState oldState = m_sessionState;
            m_sessionState = stateChangedEvent.state;

            Log::Write(Log::Level::Info, Fmt("XrEventDataSessionStateChanged: state %s->%s session=%lld time=%lld", to_string(oldState),
                                             to_string(m_sessionState), stateChangedEvent.session, stateChangedEvent.time));

            if ((stateChangedEvent.session != XR_NULL_HANDLE) && (stateChangedEvent.session != m_session)) {
                Log::Write(Log::Level::Error, "XrEventDataSessionStateChanged for unknown session");
                return;
            }

            switch (m_sessionState) {
                case XR_SESSION_STATE_READY: {
                    CHECK(m_session != XR_NULL_HANDLE);
                    XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                    sessionBeginInfo.primaryViewConfigurationType = m_options->Parsed.ViewConfigType;
                    CHECK_XRCMD(xrBeginSession(m_session, &sessionBeginInfo));
                    m_sessionRunning = true;
                    break;
                }
                case XR_SESSION_STATE_STOPPING: {
                    CHECK(m_session != XR_NULL_HANDLE);
                    m_sessionRunning = false;
                    CHECK_XRCMD(xrEndSession(m_session))
                    break;
                }
                case XR_SESSION_STATE_EXITING: {
                    *exitRenderLoop = true;
                    // Do not attempt to restart because user closed this session.
                    *requestRestart = false;
                    break;
                }
                case XR_SESSION_STATE_LOSS_PENDING: {
                    *exitRenderLoop = true;
                    // Poll for a new instance.
                    *requestRestart = true;
                    break;
                }
                default:
                    break;
            }
        }

        void OpenXrProgram::LogActionSourceName(XrAction action, const std::string& actionName) const {
            XrBoundSourcesForActionEnumerateInfo getInfo = {XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
            getInfo.action = action;
            uint32_t pathCount = 0;
            CHECK_XRCMD(xrEnumerateBoundSourcesForAction(m_session, &getInfo, 0, &pathCount, nullptr));
            std::vector<XrPath> paths(pathCount);
            CHECK_XRCMD(xrEnumerateBoundSourcesForAction(m_session, &getInfo, uint32_t(paths.size()), &pathCount, paths.data()));

            std::string sourceName;
            for (uint32_t i = 0; i < pathCount; ++i) {
                constexpr XrInputSourceLocalizedNameFlags all = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
                                                                XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
                                                                XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

                XrInputSourceLocalizedNameGetInfo nameInfo = {XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO};
                nameInfo.sourcePath = paths[i];
                nameInfo.whichComponents = all;

                uint32_t size = 0;
                CHECK_XRCMD(xrGetInputSourceLocalizedName(m_session, &nameInfo, 0, &size, nullptr));
                if (size < 1) {
                    continue;
                }
                std::vector<char> grabSource(size);
                CHECK_XRCMD(xrGetInputSourceLocalizedName(m_session, &nameInfo, uint32_t(grabSource.size()), &size, grabSource.data()));
                if (!sourceName.empty()) {
                    sourceName += " and ";
                }
                sourceName += "'";
                sourceName += std::string(grabSource.data(), size - 1);
                sourceName += "'";
            }

            Log::Write(Log::Level::Info,
                       Fmt("%s action is bound to %s", actionName.c_str(), ((!sourceName.empty()) ? sourceName.c_str() : "nothing")));
        }

		bool OpenXrProgram::IsSessionRunning() const { return m_sessionRunning; }

		bool OpenXrProgram::IsSessionFocused() const { return m_sessionState == XR_SESSION_STATE_FOCUSED; }

		void OpenXrProgram::PollActions()
        {
            m_input.handActive = {XR_FALSE, XR_FALSE};

            // Sync actions
            const XrActiveActionSet activeActionSet{m_input.actionSet, XR_NULL_PATH};
            XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
            syncInfo.countActiveActionSets = 1;
            syncInfo.activeActionSets = &activeActionSet;
            CHECK_XRCMD(xrSyncActions(m_session, &syncInfo));

            // Get pose and grab action state and start haptic vibrate when hand is 90% squeezed.
            for (auto hand : {Side::LEFT, Side::RIGHT}) 
            {
                XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                getInfo.action = m_input.grabAction;
                getInfo.subactionPath = m_input.handSubactionPath[hand];

                XrActionStateFloat grabValue{XR_TYPE_ACTION_STATE_FLOAT};
                CHECK_XRCMD(xrGetActionStateFloat(m_session, &getInfo, &grabValue));
                if (grabValue.isActive == XR_TRUE) 
                {
                    // update the grab value for both hands
                    g_theGame->m_hands[hand].m_grabValue = grabValue.currentState;

                    if (grabValue.currentState > 0.9f) {
                        XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
                        vibration.amplitude = 0.5;
                        vibration.duration = XR_MIN_HAPTIC_DURATION;
                        vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

                        XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
                        hapticActionInfo.action = m_input.vibrateAction;
                        hapticActionInfo.subactionPath = m_input.handSubactionPath[hand];
                        CHECK_XRCMD(xrApplyHapticFeedback(m_session, &hapticActionInfo, (XrHapticBaseHeader*)&vibration));
                    }
                }

                // update the pose status for both hands, if they are active or not
                getInfo.action = m_input.poseAction;
                XrActionStatePose poseState{XR_TYPE_ACTION_STATE_POSE};
                CHECK_XRCMD(xrGetActionStatePose(m_session, &getInfo, &poseState));
                m_input.handActive[hand] = poseState.isActive;
            }

            // There were no subaction paths specified for the quit action, because we don't care which hand did it.
            XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, m_input.quitAction, XR_NULL_PATH};
            XrActionStateBoolean quitValue{XR_TYPE_ACTION_STATE_BOOLEAN};
            CHECK_XRCMD(xrGetActionStateBoolean(m_session, &getInfo, &quitValue));
            if ((quitValue.isActive == XR_TRUE) && (quitValue.changedSinceLastSync == XR_TRUE) && (quitValue.currentState == XR_TRUE)) 
            {
                CHECK_XRCMD(xrRequestExitSession(m_session));
            }
        }

        void OpenXrProgram::RenderFrame()
        {
            CHECK(m_session != XR_NULL_HANDLE);

            XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
            XrFrameState frameState{XR_TYPE_FRAME_STATE};
            CHECK_XRCMD(xrWaitFrame(m_session, &frameWaitInfo, &frameState));

            XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
            CHECK_XRCMD(xrBeginFrame(m_session, &frameBeginInfo));

            std::vector<XrCompositionLayerBaseHeader*> layers;
            XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
            std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
            if (frameState.shouldRender == XR_TRUE) 
            {
                if (RenderLayer(frameState.predictedDisplayTime, projectionLayerViews, layer)) 
                {
                    layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
                }
            }

            XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
            frameEndInfo.displayTime = frameState.predictedDisplayTime;
            frameEndInfo.environmentBlendMode = m_options->Parsed.EnvironmentBlendMode;
            frameEndInfo.layerCount = (uint32_t)layers.size();
            frameEndInfo.layers = layers.data();
            CHECK_XRCMD(xrEndFrame(m_session, &frameEndInfo));
        }

        bool OpenXrProgram::RenderLayer(XrTime predictedDisplayTime, std::vector<XrCompositionLayerProjectionView>& projectionLayerViews,
                         XrCompositionLayerProjection& layer) 
        {
            XrResult res;

            XrViewState viewState{XR_TYPE_VIEW_STATE};
            uint32_t viewCapacityInput = (uint32_t)(g_theXRRenderer->m_views.size());
            uint32_t viewCountOutput;

            XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
            viewLocateInfo.viewConfigurationType = m_options->Parsed.ViewConfigType;
            viewLocateInfo.displayTime = predictedDisplayTime;
            viewLocateInfo.space = m_appSpace;

            res = xrLocateViews(m_session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, g_theXRRenderer->m_views.data());
            CHECK_XRRESULT(res, "xrLocateViews");
            if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
                (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) 
            {
                return false;  // There is no valid tracking poses for the views.
            }

            CHECK(viewCountOutput == viewCapacityInput);
            CHECK(viewCountOutput == g_theXRRenderer->m_configViews.size());
            CHECK(viewCountOutput == g_theXRRenderer->m_swapchains.size());

            projectionLayerViews.resize(viewCountOutput);

            if (res == XR_SUCCESS)
            {
				// // Get the original pose from the first view
				// XrPosef eyePose1 = g_theXRRenderer->m_views[0].pose;
				// XrPosef eyePose2 = g_theXRRenderer->m_views[1].pose;
				// 
				// XrQuaternionf viewQuaternion = (g_theXRRenderer->m_views[0].pose.orientation);
				// Vec3 pos = Vec3(g_theXRRenderer->m_views[0].pose.position);
				// Mat44 mat44;
				// XrMatrix4x4f mat = mat44.GetXrMatByMat();
				// XrMatrix4x4f_CreateFromQuaternion(&mat, &viewQuaternion);
				// 
				// Log::Write(Log::Level::Info, Stringf("eye0: m00 = %.02f, m01 = %.02f, m02 = %.02f, m03 = %.02f", mat.m[0], mat.m[1], mat.m[2], mat.m[3]));
				// Log::Write(Log::Level::Info, Stringf("eye0: m04 = %.02f, m05 = %.02f, m06 = %.02f, m07 = %.02f", mat.m[4], mat.m[5], mat.m[6], mat.m[7]));
				// Log::Write(Log::Level::Info, Stringf("eye0: m08 = %.02f, m09 = %.02f, m10 = %.02f, m11 = %.02f", mat.m[8], mat.m[9], mat.m[10], mat.m[11]));
				// Log::Write(Log::Level::Info, Stringf("eye0: m12 = %.02f, m13 = %.02f, m14 = %.02f, m15 = %.02f", mat.m[12], mat.m[13], mat.m[14], mat.m[15]));
				// Log::Write(Log::Level::Info, Stringf("eye0_Pos: x = %.02f, y = %.02f, z = %.02f", pos.x, pos.y, pos.z));
				// 
				// viewQuaternion = (g_theXRRenderer->m_views[1].pose.orientation);
				// pos = Vec3(g_theXRRenderer->m_views[1].pose.position);
				// XrMatrix4x4f_CreateFromQuaternion(&mat, &viewQuaternion);
				// 
				// Log::Write(Log::Level::Info, Stringf("eye1: m00 = %.02f, m01 = %.02f, m02 = %.02f, m03 = %.02f", mat.m[0], mat.m[1], mat.m[2], mat.m[3]));
				// Log::Write(Log::Level::Info, Stringf("eye1: m04 = %.02f, m05 = %.02f, m06 = %.02f, m07 = %.02f", mat.m[4], mat.m[5], mat.m[6], mat.m[7]));
				// Log::Write(Log::Level::Info, Stringf("eye1: m08 = %.02f, m09 = %.02f, m10 = %.02f, m11 = %.02f", mat.m[8], mat.m[9], mat.m[10], mat.m[11]));
				// Log::Write(Log::Level::Info, Stringf("eye1: m12 = %.02f, m13 = %.02f, m14 = %.02f, m15 = %.02f", mat.m[12], mat.m[13], mat.m[14], mat.m[15]));
				// Log::Write(Log::Level::Info, Stringf("eye1_Pos: x = %.02f, y = %.02f, z = %.02f", pos.x, pos.y, pos.z));

            }
            // AdjustStageSpaceAccordingToPlayerPos(predictedDisplayTime);

            // get the location information for the world space I created
		    XrSpaceLocation localSpaceLocation{ XR_TYPE_SPACE_LOCATION };
		    res = xrLocateSpace(m_worldSpace, m_appSpace, predictedDisplayTime, &localSpaceLocation); 
		    CHECK_XRRESULT(res, "xrLocateSpace");
		    if (XR_UNQUALIFIED_SUCCESS(res))
		    {
                // for the ground, render a 2m x 2m cube.
			    if ((localSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
				    (localSpaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
			    {
                    g_theGame->UpdateReferenceWorldSpacePoseForCubes(localSpaceLocation.pose);
			    }          
		    }
		    else
		    {
			    Log::Write(Log::Level::Verbose, Fmt("Unable to locate a visualized reference space in app space: %d", res));
		    }

            //----------------------------------------------------------------------------------------------------------------------------------------------------
            xrDestroySpace(m_viewSpace);

            // create new view space
			XrReferenceSpaceCreateInfo viewSpaceReferenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
			viewSpaceReferenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
			viewSpaceReferenceSpaceCI.poseInReferenceSpace = Math::Pose::Identity();
			XrSpace viewSpace;
			res = xrCreateReferenceSpace(m_session, &viewSpaceReferenceSpaceCI, &viewSpace);
			if (XR_SUCCEEDED(res))
			{
				m_viewSpace = viewSpace;
			}
			else
			{
				Log::Write(Log::Level::Error, "failed to create reference space");
			}

            // locate view space
			XrSpaceLocation viewSpaceLocation{ XR_TYPE_SPACE_LOCATION };
			res = xrLocateSpace(m_viewSpace, m_appSpace, predictedDisplayTime, &viewSpaceLocation);
			CHECK_XRRESULT(res, "xrLocateSpace");
			if (XR_UNQUALIFIED_SUCCESS(res))
			{
				// for the ground, render a 2m x 2m cube.
				if ((viewSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
					(viewSpaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
				{
                    g_theHUD->UpdateHUDViewSpacePose(viewSpaceLocation.pose);
					// XrQuaternionf viewQuaternion = (viewSpaceLocation.pose.orientation);
					Vec3 pos = Vec3(viewSpaceLocation.pose.position);
					// Mat44 mat44;
					// XrMatrix4x4f mat = mat44.GetXrMatByMat();
					// XrMatrix4x4f_CreateFromQuaternion(&mat, &viewQuaternion);
					// 
					// Log::Write(Log::Level::Info, Stringf("viewSpace: m00 = %.02f, m01 = %.02f, m02 = %.02f, m03 = %.02f", mat.m[0], mat.m[1], mat.m[2], mat.m[3]));
					// Log::Write(Log::Level::Info, Stringf("viewSpace: m04 = %.02f, m05 = %.02f, m06 = %.02f, m07 = %.02f", mat.m[4], mat.m[5], mat.m[6], mat.m[7]));
					// Log::Write(Log::Level::Info, Stringf("viewSpace: m08 = %.02f, m09 = %.02f, m10 = %.02f, m11 = %.02f", mat.m[8], mat.m[9], mat.m[10], mat.m[11]));
					// Log::Write(Log::Level::Info, Stringf("viewSpace: m12 = %.02f, m13 = %.02f, m14 = %.02f, m15 = %.02f", mat.m[12], mat.m[13], mat.m[14], mat.m[15]));
					// Log::Write(Log::Level::Info, Stringf("viewSpace_Pos: x = %.02f, y = %.02f, z = %.02f", pos.x, pos.y, pos.z));
				}
			}
			else
			{
				Log::Write(Log::Level::Verbose, Fmt("Unable to locate a visualized reference space in app space: %d", res));
			}


            // Update the space locate information for the player hand class. Only render the hand when it is active
            // true when the application has focus.
            for (auto hand : {Side::LEFT, Side::RIGHT}) 
            {
                XrSpaceLocation handSpaceLocation{XR_TYPE_SPACE_LOCATION};
                res = xrLocateSpace(m_input.handSpace[hand], m_appSpace, predictedDisplayTime, &handSpaceLocation);
                CHECK_XRRESULT(res, "xrLocateSpace");
                if (XR_UNQUALIFIED_SUCCESS(res)) 
                {
                    if ((handSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                        (handSpaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) 
                    {
						g_theGame->m_hands[hand].m_isActive = true;

                        g_theGame->UpdateReferenceHandSpacePoses(handSpaceLocation.pose, hand);
                    }
                } 
                else 
                {
                    // Tracking loss is expected when the hand is not active so only log a message
                    // update the information for the hand class
					if (m_input.handActive[hand] == XR_TRUE) 
                    {
                        g_theGame->m_hands[hand].m_isActive = false;

						const char* handName[] = { "left", "right" };
						Log::Write(Log::Level::Verbose, Fmt("Unable to locate %s hand action space in app space: %d", handName[hand], res));
					}
                }
            }

            //----------------------------------------------------------------------------------------------------------------------------------------------------
            // Render view to the appropriate part of the swapchain image.
            for (uint32_t i = 0; i < viewCountOutput; i++) 
            {
                // Each view has a separate swapchain which is acquired, rendered to, and released.
                const Swapchain viewSwapchain = g_theXRRenderer->m_swapchains[i];

                XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

                uint32_t swapchainImageIndex;
                CHECK_XRCMD(xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex));

                XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                waitInfo.timeout = XR_INFINITE_DURATION;
                CHECK_XRCMD(xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo));

                projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
                projectionLayerViews[i].pose = g_theXRRenderer->m_views[i].pose;
                projectionLayerViews[i].fov = g_theXRRenderer->m_views[i].fov;
                projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
                projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
                projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

                const XrSwapchainImageBaseHeader* const swapchainImage = g_theXRRenderer->m_swapchainImages[viewSwapchain.handle][swapchainImageIndex];
                g_theXRRenderer->BeginCamera(projectionLayerViews[i], swapchainImage, g_theXRRenderer->m_colorSwapchainFormat);

				g_theGame->Render();

                XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                CHECK_XRCMD(xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo));
            }

            layer.space = m_appSpace;
            layer.layerFlags =
                m_options->Parsed.EnvironmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND
                    ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT
                    : 0;
            layer.viewCount = (uint32_t)projectionLayerViews.size();
            layer.views = projectionLayerViews.data();
            return true;
        }

		XrEnvironmentBlendMode OpenXrProgram::GetPreferredBlendMode()
		{
			uint32_t count;
			CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, m_options->Parsed.ViewConfigType, 0, &count, nullptr));
			CHECK(count > 0);

			std::vector<XrEnvironmentBlendMode> blendModes(count);
			CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, m_options->Parsed.ViewConfigType, count, &count,
				blendModes.data()));
			for (const auto& blendMode : blendModes) {
				if (m_acceptableBlendModes.count(blendMode)) return blendMode;
			}
			THROW("No acceptable blend mode returned from the xrEnumerateEnvironmentBlendModes");
		}

		XrQuaternionf OpenXrProgram::createQuaternionFromAxisAngle(float x, float y, float z)
		{
			float halfAngle = 3.1415926f * 0.5f;
			float sinHalfAngle = sin(halfAngle);
            XrQuaternionf q;
			q.x = x * sinHalfAngle;
			q.y = y * sinHalfAngle;
			q.z = z * sinHalfAngle;
			q.w = cos(halfAngle);
			return q;
		}
